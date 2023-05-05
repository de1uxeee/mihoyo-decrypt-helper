#include <iostream>
#include <Windows.h>
#include <fstream>
#include <vector>

typedef uintptr_t(__cdecl* decryptMetadata_t) (char originalMetadata[], size_t length);
decryptMetadata_t decryptMetadata;

typedef uintptr_t(__cdecl* getStringFromIndex_t) (char decryptedMetadata[], size_t index);
getStringFromIndex_t getStringFromIndex;

typedef uintptr_t(__cdecl* getStringLiteralFromIndex_t) (char decryptedMetadata[], size_t index, int32_t* length);
getStringLiteralFromIndex_t getStringLiteralFromIndex;

uintptr_t decryptMetadataOffset = 0xC9B10;
uintptr_t getStringFromIndexOffset = 0x51710;
uintptr_t getStringLiteralFromIndexOffset = 0x51910;

int main(int argc, char* argv[])
{
    if (argc < 1)
        return -1;

    if (const HMODULE unityDll = LoadLibraryA("UnityPlayer.dll"); unityDll)
    {
        decryptMetadata = reinterpret_cast<decryptMetadata_t>(reinterpret_cast<uintptr_t>(unityDll) + decryptMetadataOffset);

        std::ifstream file("global-metadata.dat", std::ios_base::binary);
        std::vector<char> buff(std::istreambuf_iterator(file), {});
        size_t len{ buff.size() };
        file.close();

        uintptr_t decryptPtr = decryptMetadata(&buff[0], len);

        char* decryptedMetadata = new char[len];
        memcpy(decryptedMetadata, (PVOID)decryptPtr, len);

        std::ofstream decryptedFilePart("decryptedPartly.dat", std::ios_base::binary);
        decryptedFilePart.write(decryptedMetadata, len);
        decryptedFilePart.close();

        int32_t xorKeyHonkai[] = { 0x3F, 0x73, 0xA8, 0x5A, 0x8, 0x32, 0xA, 0x33, 0x3C, 0xFA, 0x8D, 0x4E, 0x8B, 0xC, 0x77, 0x45 };

        int32_t xorKeyStarRail[] = { 0xF4, 0xB9, 0x54, 0x50, 0x85, 0x21, 0xB4, 0x14, 0x6C, 0x2F, 0xF1, 0xC2, 0x88, 0x9C, 0x79, 0xC4 };

        size_t step = (len >> 14) << 6;
        printf_s("step 0x%llX\n", step);

        std::ofstream bytes("bytesForHXD.dat", std::ios_base::binary);
        for (size_t pos = 0; pos < len; pos += step)
        {
            for (int32_t i = 0; i < 0x10; i++)
                bytes << decryptedMetadata[pos + i];

            for (size_t b = 0; b < 0x10; b++)
                decryptedMetadata[pos + b] ^= xorKeyHonkai[b];
        }
        bytes.close();

        std::ofstream decryptedFile("decrypted.dat", std::ios_base::binary);
        decryptedFile.write(decryptedMetadata, len);
        decryptedFile.close();

        getStringFromIndex = reinterpret_cast<getStringFromIndex_t>(reinterpret_cast<uintptr_t>(unityDll) + getStringFromIndexOffset);

        size_t numberOfSymbols{};
        std::ofstream stringsFile("strings.txt", std::ios_base::binary);

        for (int32_t i = 0; i < 0x10000; i++)
        {
            const uintptr_t stringPtr = getStringFromIndex(decryptedMetadata, numberOfSymbols);
            stringsFile << reinterpret_cast<const char*>(stringPtr) << '\n';
            numberOfSymbols += strlen(reinterpret_cast<const char*>(stringPtr)) + 0x1;
        }

        stringsFile.close();

        getStringLiteralFromIndex = reinterpret_cast<getStringLiteralFromIndex_t>(reinterpret_cast<uintptr_t>(unityDll) + getStringLiteralFromIndexOffset);

        std::ofstream stringLiteralsFile("stringLiterals.txt", std::ios_base::binary);

        for (int32_t i = 0; i < 0x10000; i++)
        {
            int32_t literalLength{};
            uintptr_t decryptedBytesPtr = getStringLiteralFromIndex(decryptedMetadata, i, &literalLength);
            stringLiteralsFile << reinterpret_cast<const char*>(decryptedBytesPtr) << '\n';
        }

        stringLiteralsFile.close();

        delete[] decryptedMetadata;
        FreeLibrary(unityDll);

        system("pause");
    }

    return 0;
}

