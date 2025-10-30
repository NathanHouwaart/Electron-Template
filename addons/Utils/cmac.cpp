#include <vector>
#include <array>
#include <string>
#include <cstdint>
#include <cstring>


#include "cppdes/des.h"
#include "cppdes/descbc.h"
#include "cppdes/des3.h"
#include "cppdes/des3cbc.h"

#include "../PN532_Controller/Source/desfireKey.h"

template<typename T>
void printHexArray(const T& arr, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        printf("%02X ", arr[i]);
    }
    printf("\n");
}

template<typename T>
void printHex(const T& value) {
    size_t size = sizeof(T) / sizeof(uint8_t);
    const uint8_t* bytePtr = reinterpret_cast<const uint8_t*>(&value);
    for (size_t i = 0; i < size; ++i) {
        printf("%02X ", bytePtr[size - 1 - i]);
    }
    printf("\n");
}


bool GenerateCmacSubkeys(uint8_t blockSize, const std::array<uint8_t, 16> & sessionKey, std::array<uint8_t, 16>& Cmac1, std::array<uint8_t, 16>& Cmac2){

    uint8_t CmacR = (blockSize == 8) ? 0x1B : 0x87;
    std::array<uint8_t, 16> data = {0};

    // Initialize DES key
    uint64_t sessionKeyVar = 0;
    
    // Copy sessionKey bytes into sessionKeyVar in big-endian order
    for (size_t i = 0; i < blockSize; ++i) {
        sessionKeyVar = (sessionKeyVar << 8) | sessionKey[i];
    }

    uint64_t dataVar = 0;
    for (size_t i = 0; i < blockSize; ++i) {
        dataVar = (dataVar << 8) | data[i];
    }

    printHex(sessionKeyVar);

    DESCBC desCbc(sessionKeyVar, 0); // IV is zero
    auto encrypted = desCbc.encrypt(dataVar);
   
    printHexArray(data, blockSize);

    printHex(encrypted);

    auto cmac1Var = encrypted << 1;
    printHex(cmac1Var);
    if(cmac1Var & (0x80 << ((blockSize - 1) * 8))){
        cmac1Var ^= CmacR;
    }
    printHex(cmac1Var);

    auto cmac2var = cmac1Var << 1;
    printHex(cmac2var);
    if(cmac1Var & (0x80 << ((blockSize - 1) * 8))){
        cmac2var ^= CmacR;
    }
    printHex(cmac2var);

    return true;
}
    

int main(){
    std::array<uint8_t, 16> mu8_Cmac1;
    std::array<uint8_t, 16> mu8_Cmac2;

    auto sessionKey = std::to_array<uint8_t>({
        0x48, 0xEC, 0x62, 0xDE, 0x3A, 0x3C, 0xFE, 0x62
    });

    DesfireKey<DesfireKeyType::DES> key(sessionKey, 0x00);
    key.printKey();

    std::array<uint8_t, 8> dataToEncrypt = {
        0x00
    };

    std::array<uint8_t, 8> encryptedData = {0};

    // pass both spans as two arguments (no comma operator)
    bool ok = key.encrypt(
        std::span<const uint8_t>(dataToEncrypt.data(), dataToEncrypt.size()),
        std::span<uint8_t>(encryptedData.data(), encryptedData.size())
    );

    auto iv = key.getIV();
    std::cout << "IV after encryption: ";
    for(auto byte : iv){
        std::cout << std::hex << std::uppercase << static_cast<int>(byte) << " ";
    }
    std::cout << std::dec << std::endl;

    auto [cmac1, cmac2] = key.getCmacSubkeys();

    printHexArray(cmac1, cmac1.size());
    printHexArray(cmac2, cmac1.size());


    if(ok){
        std::cout << "Encryption successful. Encrypted data: ";
        for(auto byte : encryptedData){
            std::cout << std::hex << std::uppercase << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl;
    } else {
        std::cout << "Encryption failed." << std::endl;
    }

    return 0;
}
