#include "../../Headers/Cards/Desfire.h"
#include "../../Headers/pn532.h"
#include "../../../AddonLog.h"
#include <iostream>
#include <iomanip>
#include "../../Headers/Commands/inDataExchange.h"
#include "aes.hpp"
#include "cppdes/des3.h"
#include "cppdes/des3cbc.h"
#include <algorithm>
#include <random>

namespace {
std::array<uint8_t,8> rotateLeft(std::array<uint8_t,8> value) {
    std::array<uint8_t,8> out{};
    // Rotate left by 1: [a,b,c,d,e,f,g,h] -> [b,c,d,e,f,g,h,a]
    std::rotate_copy(value.begin(), value.begin() + 1, value.end(), out.begin());
    return out;
}
std::array<uint8_t,8> rotateRight(std::array<uint8_t,8> value) {
    std::array<uint8_t,8> out{};
    // Rotate right by 1: [a,b,c,d,e,f,g,h] -> [h,a,b,c,d,e,f,g]
    std::rotate_copy(value.begin(), value.end() - 1, value.end(), out.begin());
    return out;
}

// CRC16 calculation for DESFire (ISO 14443-3 Type A)
// Polynomial: 0x8005, Initial value: 0x6363
// This is used for ChangeKey command payload integrity
uint16_t calculateCRC16(const uint8_t* data, size_t length) {
    uint16_t crc = 0x6363;  // ISO/IEC 14443-3 Type A preset

    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint16_t>(data[i]);
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x0001) ? (crc >> 1) ^ 0x8408 : (crc >> 1);
        }
    }

    return crc;
}

} // namespace

// Helper to output DESFire version info (inspired by desfire.cpp)
std::ostream &operator<<(std::ostream &os, const DesfireVersionInfo &v)
{
    os << "=== DESFire Version Info ===\n";
    os << "Hardware Information:\n";
    os << "  Vendor: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(v.hardwareInfo.hwVendorID) << std::dec << "\n";
    os << "  Type: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(v.hardwareInfo.hwType);
    os << " SubType: 0x" << std::setw(2) << std::setfill('0') << static_cast<int>(v.hardwareInfo.hwSubType) << std::dec << "\n";
    os << "  Version: " << static_cast<int>(v.hardwareInfo.hwMajorVersion) << "." << static_cast<int>(v.hardwareInfo.hwMinorVersion) << "\n";
    os << "  Storage: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(v.hardwareInfo.hwStorageSize) << std::dec << "\n";
    os << "  Protocol: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(v.hardwareInfo.hwProtocolType) << std::dec << "\n";

    os << "\nSoftware Information:\n";
    os << "  Vendor: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(v.softwareInfo.swVendorID) << std::dec << "\n";
    os << "  Type: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(v.softwareInfo.swType);
    os << " SubType: 0x" << std::setw(2) << std::setfill('0') << static_cast<int>(v.softwareInfo.swSubType) << std::dec << "\n";
    os << "  Version: " << static_cast<int>(v.softwareInfo.swMajorVersion) << "." << static_cast<int>(v.softwareInfo.swMinorVersion) << "\n";
    os << "  Storage: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(v.softwareInfo.swStorageSize) << std::dec << "\n";
    os << "  Protocol: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(v.softwareInfo.swProtocolType) << std::dec << "\n";

    os << "\nManufacturing Information:\n";
    os << "  UID: ";
    for (int i = 0; i < 7; i++)
        os << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(v.manufacturingInfo.uid[i]) << (i < 6 ? ":" : "");
    os << std::dec << "\n";

    os << "  Batch: ";
    for (int i = 0; i < 5; i++)
        os << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(v.manufacturingInfo.batchNumber[i]) << " ";
    os << std::dec << "\n";
    os << "  Production: Week " << static_cast<int>(v.manufacturingInfo.productionWeek);
    os << " Year " << static_cast<int>(v.manufacturingInfo.productionYear) << "\n";

    return os;
}

bool MifareDesfireCard::parseVersionFrame(const uint8_t *data, uint8_t responseSize, uint8_t frameIndex)
{
    if (!data)
    {
        Log("parseVersionFrame: Data pointer is null\n");
        return false;
    }

    Log("Parsing DESFire GetVersion frame " + std::to_string(frameIndex) + " (size: " + std::to_string(responseSize) + " bytes)\n");

    // Log raw data for debugging
    std::cout << "  Raw data: ";
    for (uint8_t i = 0; i < responseSize; i++)
    {
        std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]) << " ";
    }
    std::cout << std::dec << std::endl;

    switch (frameIndex)
    {
    case 0: // Hardware info (7 bytes)
        if (responseSize < sizeof(versionInfo_.hardwareInfo))
        {
            Log("ERROR: Not enough data for hardware info (need " + std::to_string(sizeof(versionInfo_.hardwareInfo)) +
                " bytes, got " + std::to_string(responseSize) + ")\n");
            return false;
        }
        std::memcpy(&versionInfo_.hardwareInfo, data, sizeof(versionInfo_.hardwareInfo));
        break;

    case 1: // Software info (7 bytes)
        if (responseSize < sizeof(versionInfo_.softwareInfo))
        {
            Log("ERROR: Not enough data for software info (need " + std::to_string(sizeof(versionInfo_.softwareInfo)) +
                " bytes, got " + std::to_string(responseSize) + ")\n");
            return false;
        }
        std::memcpy(&versionInfo_.softwareInfo, data, sizeof(versionInfo_.softwareInfo));
        break;

    case 2: // Manufacturing info (14 bytes)
        if (responseSize < sizeof(versionInfo_.manufacturingInfo))
        {
            Log("ERROR: Not enough data for manufacturing info (need " + std::to_string(sizeof(versionInfo_.manufacturingInfo)) +
                " bytes, got " + std::to_string(responseSize) + ")\n");
            return false;
        }
        std::memcpy(&versionInfo_.manufacturingInfo, data, sizeof(versionInfo_.manufacturingInfo));
        break;

    default:
        Log("ERROR: Invalid frame index: " + std::to_string(frameIndex) + "\n");
        return false;
    }

    return true;
}

void MifareDesfireCard::selectApplication(uint32_t aid)
{
    Log("Executing DESFire SelectApplication command via InDataExchange on AID " + std::to_string(aid) + "\n");

    uint8_t cmd[] = {
        0x90,             // CLA
        0x5A,             // INS = SelectApplication
        0x00,             // P1
        0x00,             // P2
        0x03,             // Lc = length of AID
        static_cast<uint8_t>((aid >> 16) & 0xFF), // AID byte 1
        static_cast<uint8_t>((aid >> 8) & 0xFF),  // AID byte 2
        static_cast<uint8_t>(aid & 0xFF),         // AID byte 3
        0x00              // Le
    };

    using namespace NFC_Controller::Cpp;

    auto command = InDataExchangeCommand(
        InDataExchangeCommand::Options{
            .payload = std::vector<uint8_t>(cmd, cmd + sizeof(cmd)),
            .responseTimeoutMs = 2000});

    auto result = nfc_->executeCommand(command);

    if (result.status != pn532Response::statusCode::OK)
    {
        Log("ERROR: selectApplication failed with status: " + std::to_string(static_cast<int>(result.status)) + "\n");
        return;
    }

    if (!command.isSuccess()){
        Log("ERROR: selectApplication command returned failure status\n");
        return;
    }

    Log("selectApplication succeeded on AID " + std::to_string(aid) + "\n");

    return;
}

void MifareDesfireCard::authenticate(){
    using namespace NFC_Controller::Cpp;
    
    Log("Executing DESFire Authenticate command via InDataExchange\n");
    
    this->selectApplication(0x000000); // Select PICC/master application first

    // Step 1 – start legacy 3DES authentication
    uint8_t cmd[] = { 0x90, 0x0A, 0x00, 0x00, 0x01, 0x00, 0x00 };
    auto command = InDataExchangeCommand(
        InDataExchangeCommand::Options{
            .payload = std::vector<uint8_t>(cmd, cmd + sizeof(cmd)),
            .responseTimeoutMs = 2000}
    );
    auto result = nfc_->executeCommand(command);

    if (result.status != pn532Response::statusCode::OK)
    {
        Log("ERROR: authenticate failed with status: " + std::to_string(static_cast<int>(result.status)) + "\n");
        return;
    }

    std::cout << "authenticate response: ";
    for (auto byte : result.responsePayload)
    {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::dec << std::endl;
    

    // The card must reply with 8 encrypted bytes followed by 91 AF
    if (result.responsePayload.size() < 10 ||
        result.responsePayload[result.responsePayload.size() - 2] != 0x91 ||
        result.responsePayload[result.responsePayload.size() - 1] != 0xAF) {
        Log("ERROR: unexpected DESFire auth response\n");
        return;
    }

    std::array<uint8_t,8> encRndB{};
    std::copy_n(result.responsePayload.begin(), 8, encRndB.begin());
    
    // Step 2 – decrypt RndB using 2-key 3DES ECB
    // Factory default PICC master key: two zero 64-bit DES keys
    ui64 key1 = 0x0000000000000000ULL;  // First 8 bytes (all zeros)
    ui64 key2 = 0x0000000000000000ULL;  // Second 8 bytes (all zeros)
    
    // For 2-key 3DES: K1 and K3 are the same
    DES3 des3(key1, key2, key1);
    
    std::cout << "Encrypted RndB bytes: ";
    for (auto byte : encRndB) {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;
    
    // Convert 8-byte array to ui64 (big-endian: first byte is MSB)
    ui64 encRndB_u64 = 0;
    for (int i = 0; i < 8; i++) {
        encRndB_u64 = (encRndB_u64 << 8) | encRndB[i];
    }
    
    std::cout << "encRndB_u64: 0x" << std::hex << std::setw(16) << std::setfill('0') << encRndB_u64 << std::dec << std::endl;
    
    // Decrypt the challenge
    ui64 rndB_u64 = des3.decrypt(encRndB_u64);
    
    std::cout << "rndB_u64: 0x" << std::hex << std::setw(16) << std::setfill('0') << rndB_u64 << std::dec << std::endl;
    
    // Convert back to byte array (big-endian: MSB first)
    std::array<uint8_t,8> rndB{};
    for (int i = 7; i >= 0; i--) {
        rndB[i] = static_cast<uint8_t>(rndB_u64 & 0xFF);
        rndB_u64 >>= 8;
    }
    
    std::cout << "Decrypted RndB: ";
    for (auto byte : rndB) {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;

    // Step 3 – generate our own random RndA (8 bytes)
    std::array<uint8_t,8> rndA{};
    {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint16_t> dis(0, 255);
        for (auto& byte : rndA) {
            byte = static_cast<uint8_t>(dis(gen));
        }
    }
    
    std::cout << "Generated RndA: ";
    for (auto byte : rndA) {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;
    
    // Step 4 – rotate RndB left by 1 byte
    std::cout << "Rotating RndB left by 1 byte\n";
    std::array<uint8_t,8> rndB_rotated = rotateLeft(rndB);
    
    std::cout << "RndB rotated left: ";
    for (auto byte : rndB_rotated) {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;
    
    // Step 5 – concatenate RndA || rotated(RndB) = 16 bytes
    std::array<uint8_t,16> hostChallenge{};
    std::copy(rndA.begin(), rndA.end(), hostChallenge.begin());
    std::copy(rndB_rotated.begin(), rndB_rotated.end(), hostChallenge.begin() + 8);
    
    std::cout << "Host challenge (RndA || RndB'): ";
    for (auto byte : hostChallenge) {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;
    
    // Step 6 – encrypt the 16-byte challenge using 2-key 3DES CBC (two 8-byte blocks, IV=0)
    std::array<uint8_t,16> encHostChallenge{};
    
    // Create 3DES CBC cipher with IV = 0
    ui64 iv = 0x0000000000000000ULL;
    DES3CBC des3cbc(key1, key2, key1, iv);
    
    // Encrypt first 8 bytes (RndA)
    ui64 block1_u64 = 0;
    for (int i = 0; i < 8; i++) {
        block1_u64 = (block1_u64 << 8) | hostChallenge[i];
    }
    ui64 encBlock1_u64 = des3cbc.encrypt(block1_u64);
    for (int i = 7; i >= 0; i--) {
        encHostChallenge[i] = static_cast<uint8_t>(encBlock1_u64 & 0xFF);
        encBlock1_u64 >>= 8;
    }
    
    // Encrypt second 8 bytes (rotated RndB) - CBC mode chains automatically
    ui64 block2_u64 = 0;
    for (int i = 0; i < 8; i++) {
        block2_u64 = (block2_u64 << 8) | hostChallenge[8 + i];
    }
    ui64 encBlock2_u64 = des3cbc.encrypt(block2_u64);
    for (int i = 7; i >= 0; i--) {
        encHostChallenge[8 + i] = static_cast<uint8_t>(encBlock2_u64 & 0xFF);
        encBlock2_u64 >>= 8;
    }
    
    std::cout << "Encrypted host challenge: ";
    for (auto byte : encHostChallenge) {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;

    // Step 7 – send encrypted challenge to card via Additional Frame (0xAF)
    std::vector<uint8_t> afCmd;
    afCmd.push_back(0x90);  // CLA
    afCmd.push_back(0xAF);  // INS = Additional Frame
    afCmd.push_back(0x00);  // P1
    afCmd.push_back(0x00);  // P2
    afCmd.push_back(0x10);  // Lc = 16 bytes
    // Add the 16 encrypted bytes
    afCmd.insert(afCmd.end(), encHostChallenge.begin(), encHostChallenge.end());
    afCmd.push_back(0x00);  // Le
    
    auto afCommand = InDataExchangeCommand(
        InDataExchangeCommand::Options{
            .payload = afCmd,
            .responseTimeoutMs = 2000}
    );
    auto afResult = nfc_->executeCommand(afCommand);

    if (afResult.status != pn532Response::statusCode::OK)
    {
        Log("ERROR: Additional Frame failed with status: " + std::to_string(static_cast<int>(afResult.status)) + "\n");
        return;
    }

    std::cout << "Card verification response: ";
    for (auto byte : afResult.responsePayload)
    {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::dec << std::endl;
    
    // The card must reply with 8 encrypted bytes followed by 0x91 0x00 (success)
    if (afResult.responsePayload.size() < 10 ||
        afResult.responsePayload[afResult.responsePayload.size() - 2] != 0x91 ||
        afResult.responsePayload[afResult.responsePayload.size() - 1] != 0x00) {
        Log("ERROR: unexpected card verification response\n");
        return;
    }
    
    std::array<uint8_t,8> encRndA_rotated{};
    std::copy_n(afResult.responsePayload.begin(), 8, encRndA_rotated.begin());
    
    // Step 8 – decrypt the card's response (rotated RndA)
    ui64 encRndA_rotated_u64 = 0;
    for (int i = 0; i < 8; i++) {
        encRndA_rotated_u64 = (encRndA_rotated_u64 << 8) | encRndA_rotated[i];
    }
    
    ui64 rndA_rotated_u64 = des3.decrypt(encRndA_rotated_u64);
    
    std::array<uint8_t,8> rndA_rotated{};
    for (int i = 7; i >= 0; i--) {
        rndA_rotated[i] = static_cast<uint8_t>(rndA_rotated_u64 & 0xFF);
        rndA_rotated_u64 >>= 8;
    }
    
    std::cout << "Decrypted rotated RndA from card: ";
    for (auto byte : rndA_rotated) {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;
    
    // Step 9 – rotate right to get original RndA and verify
    std::array<uint8_t,8> rndA_verified = rotateRight(rndA_rotated);
    
    std::cout << "RndA after rotating right: ";
    for (auto byte : rndA_verified) {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;
    
    // Verify it matches our original RndA
    if (rndA == rndA_verified) {
        std::cout << "✅ AUTHENTICATION SUCCESSFUL! RndA matches!" << std::endl;
        Log("DESFire authentication succeeded\n");
        
        // Store session data for ChangeKey (and other authenticated commands)
        sessionValid_ = true;
        currentKeyType_ = DesfireKeyType::DES3_2KEY;
        currentKeyNo_ = 0;  // Legacy authenticate always uses key 0
        
        // Copy RndA and RndB to session storage (extend to 16 bytes for uniform storage)
        std::copy(rndA.begin(), rndA.end(), sessionRndA_.begin());
        std::fill(sessionRndA_.begin() + 8, sessionRndA_.end(), 0);
        
        std::copy(rndB.begin(), rndB.end(), sessionRndB_.begin());
        std::fill(sessionRndB_.begin() + 8, sessionRndB_.end(), 0);
        
        // Store current key (factory default all-zero 2-key 3DES)
        currentKey_.fill(0);
        
        std::cout << "Session established: 3DES with factory default key" << std::endl;
    } else {
        std::cout << "❌ AUTHENTICATION FAILED! RndA mismatch!" << std::endl;
        Log("ERROR: Authentication failed - RndA verification mismatch\n");
        sessionValid_ = false;
        return;
    }

    return;
}

template<DesfireKeyType KeyType>
bool MifareDesfireCard::authenticate(uint8_t keyNo, const std::array<uint8_t, DesfireKeyTraits<KeyType>::KeySize>& key) {
    using namespace NFC_Controller::Cpp;
    
    constexpr size_t keySize = DesfireKeyTraits<KeyType>::KeySize;
    constexpr size_t rndSize = (KeyType == DesfireKeyType::AES) ? 16 : 8;
    constexpr uint8_t insCode = (KeyType == DesfireKeyType::AES) ? 0xAA : 0x0A;
    
    std::cout << "\n===============================================\n";
    std::cout << " Authenticate: Key #" << int(keyNo) << " (" 
              << DesfireKeyTraits<KeyType>::Name << ", " << keySize << " bytes)\n";
    std::cout << "===============================================\n";
    
    Log("Executing DESFire Authenticate command via InDataExchange\n");
    
    selectApplication(0x000000); // Select PICC/master application first

    // Step 1: Send authentication command (0x0A for DES/3DES, 0xAA for AES)
    uint8_t cmd[] = { 0x90, insCode, 0x00, 0x00, 0x01, keyNo, 0x00 };
    auto command = InDataExchangeCommand(
        InDataExchangeCommand::Options{
            .payload = std::vector<uint8_t>(cmd, cmd + sizeof(cmd)),
            .responseTimeoutMs = 2000}
    );
    auto result = nfc_->executeCommand(command);

    if (result.status != pn532Response::statusCode::OK)
    {
        Log("ERROR: authenticate failed with status: " + std::to_string(static_cast<int>(result.status)) + "\n");
        std::cout << "❌ Authentication failed\n";
        return false;
    }

    std::cout << "authenticate response: ";
    for (auto byte : result.responsePayload)
    {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::dec << std::endl;
    
    // The card must reply with rndSize encrypted bytes followed by 91 AF
    if (result.responsePayload.size() < rndSize + 2 ||
        result.responsePayload[result.responsePayload.size() - 2] != 0x91 ||
        result.responsePayload[result.responsePayload.size() - 1] != 0xAF) {
        Log("ERROR: unexpected DESFire auth response\n");
        std::cout << "❌ Unexpected response format\n";
        return false;
    }

    // Step 2: Decrypt RndB
    std::array<uint8_t, rndSize> encRndB{};
    std::copy_n(result.responsePayload.begin(), rndSize, encRndB.begin());
    
    std::cout << "Encrypted RndB bytes: ";
    for (auto byte : encRndB) {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;
    
    std::array<uint8_t, rndSize> rndB{};
    
    if constexpr (KeyType == DesfireKeyType::AES) {
        // AES decryption (16-byte block)
        struct AES_ctx ctx;
        uint8_t iv[16] = {0};
        AES_init_ctx_iv(&ctx, key.data(), iv);
        
        std::copy(encRndB.begin(), encRndB.end(), rndB.begin());
        AES_CBC_decrypt_buffer(&ctx, rndB.data(), rndB.size());
        
    } else {
        // 3DES decryption (8-byte block)
        ui64 key1_u64 = 0, key2_u64 = 0, key3_u64 = 0;
        
        // Extract key components based on key size
        for (int i = 0; i < 8; i++) {
            key1_u64 = (key1_u64 << 8) | key[i];
        }
        
        if constexpr (keySize >= 16) {
            for (int i = 0; i < 8; i++) {
                key2_u64 = (key2_u64 << 8) | key[8 + i];
            }
        } else {
            key2_u64 = key1_u64;  // Single DES uses same key
        }
        
        if constexpr (keySize >= 24) {
            for (int i = 0; i < 8; i++) {
                key3_u64 = (key3_u64 << 8) | key[16 + i];
            }
        } else {
            key3_u64 = key1_u64;  // 2-key 3DES: K3 = K1
        }
        
        DES3 des3(key1_u64, key2_u64, key3_u64);
        
        // Convert RndB to ui64 (big-endian)
        ui64 encRndB_u64 = 0;
        for (int i = 0; i < 8; i++) {
            encRndB_u64 = (encRndB_u64 << 8) | encRndB[i];
        }
        
        // Decrypt
        ui64 rndB_u64 = des3.decrypt(encRndB_u64);
        
        // Convert back to byte array
        for (int i = 7; i >= 0; i--) {
            rndB[i] = static_cast<uint8_t>(rndB_u64 & 0xFF);
            rndB_u64 >>= 8;
        }
    }
    
    std::cout << "Decrypted RndB: ";
    for (auto byte : rndB) {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;

    // Step 3: Generate RndA
    std::array<uint8_t, rndSize> rndA{};
    {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint16_t> dis(0, 255);
        for (auto& byte : rndA) {
            byte = static_cast<uint8_t>(dis(gen));
        }
    }
    
    std::cout << "Generated RndA: ";
    for (auto byte : rndA) {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;
    
    // Step 4: Rotate RndB left by 1 byte
    std::array<uint8_t, rndSize> rndB_rotated{};
    if constexpr (rndSize == 8) {
        rndB_rotated = rotateLeft(rndB);
    } else {
        // AES: 16-byte rotation
        std::rotate_copy(rndB.begin(), rndB.begin() + 1, rndB.end(), rndB_rotated.begin());
    }
    
    std::cout << "RndB rotated left: ";
    for (auto byte : rndB_rotated) {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;
    
    // Step 5: Build host challenge: RndA || RndB'
    std::array<uint8_t, rndSize * 2> hostChallenge{};
    std::copy(rndA.begin(), rndA.end(), hostChallenge.begin());
    std::copy(rndB_rotated.begin(), rndB_rotated.end(), hostChallenge.begin() + rndSize);
    
    std::cout << "Host challenge (RndA || RndB'): ";
    for (auto byte : hostChallenge) {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;
    
    // Step 6: Encrypt host challenge
    std::array<uint8_t, rndSize * 2> encHostChallenge{};
    
    if constexpr (KeyType == DesfireKeyType::AES) {
        // AES encryption
        struct AES_ctx ctx;
        uint8_t iv[16] = {0};
        AES_init_ctx_iv(&ctx, key.data(), iv);
        
        std::copy(hostChallenge.begin(), hostChallenge.end(), encHostChallenge.begin());
        AES_CBC_encrypt_buffer(&ctx, encHostChallenge.data(), encHostChallenge.size());
        
    } else {
        // 3DES CBC encryption
        ui64 key1_u64 = 0, key2_u64 = 0, key3_u64 = 0;
        for (int i = 0; i < 8; i++) key1_u64 = (key1_u64 << 8) | key[i];
        if constexpr (keySize >= 16) {
            for (int i = 0; i < 8; i++) key2_u64 = (key2_u64 << 8) | key[8 + i];
        } else {
            key2_u64 = key1_u64;
        }
        key3_u64 = key1_u64;  // For 2-key 3DES
        
        DES3CBC des3cbc(key1_u64, key2_u64, key3_u64, 0x0000000000000000ULL);
        
        // Encrypt two 8-byte blocks
        for (size_t blockIdx = 0; blockIdx < 2; blockIdx++) {
            ui64 block_u64 = 0;
            for (size_t i = 0; i < 8; i++) {
                block_u64 = (block_u64 << 8) | hostChallenge[blockIdx * 8 + i];
            }
            
            ui64 encBlock_u64 = des3cbc.encrypt(block_u64);
            
            for (int i = 7; i >= 0; i--) {
                encHostChallenge[blockIdx * 8 + i] = static_cast<uint8_t>(encBlock_u64 & 0xFF);
                encBlock_u64 >>= 8;
            }
        }
    }
    
    std::cout << "Encrypted host challenge: ";
    for (auto byte : encHostChallenge) {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;

    // Step 7: Send encrypted challenge via Additional Frame
    std::vector<uint8_t> afCmd;
    afCmd.push_back(0x90);  // CLA
    afCmd.push_back(0xAF);  // INS = Additional Frame
    afCmd.push_back(0x00);  // P1
    afCmd.push_back(0x00);  // P2
    afCmd.push_back(rndSize * 2);  // Lc
    afCmd.insert(afCmd.end(), encHostChallenge.begin(), encHostChallenge.end());
    afCmd.push_back(0x00);  // Le
    
    auto afCommand = InDataExchangeCommand(
        InDataExchangeCommand::Options{
            .payload = afCmd,
            .responseTimeoutMs = 2000}
    );
    auto afResult = nfc_->executeCommand(afCommand);

    if (afResult.status != pn532Response::statusCode::OK)
    {
        Log("ERROR: Additional Frame failed with status: " + std::to_string(static_cast<int>(afResult.status)) + "\n");
        std::cout << "❌ Challenge response failed\n";
        return false;
    }

    std::cout << "Card verification response: ";
    for (auto byte : afResult.responsePayload)
    {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::dec << std::endl;
    
    // Card must reply with rndSize encrypted bytes followed by 0x91 0x00 (success)
    if (afResult.responsePayload.size() < rndSize + 2 ||
        afResult.responsePayload[afResult.responsePayload.size() - 2] != 0x91 ||
        afResult.responsePayload[afResult.responsePayload.size() - 1] != 0x00) {
        Log("ERROR: unexpected card verification response\n");
        std::cout << "❌ Verification failed\n";
        return false;
    }
    
    // Step 8: Decrypt and verify RndA'
    std::array<uint8_t, rndSize> encRndA_rotated{};
    std::copy_n(afResult.responsePayload.begin(), rndSize, encRndA_rotated.begin());
    
    std::array<uint8_t, rndSize> rndA_rotated{};
    
    if constexpr (KeyType == DesfireKeyType::AES) {
        // AES decryption
        struct AES_ctx ctx;
        uint8_t iv[16] = {0};
        AES_init_ctx_iv(&ctx, key.data(), iv);
        
        std::copy(encRndA_rotated.begin(), encRndA_rotated.end(), rndA_rotated.begin());
        AES_CBC_decrypt_buffer(&ctx, rndA_rotated.data(), rndA_rotated.size());
        
    } else {
        // 3DES decryption
        ui64 key1_u64 = 0, key2_u64 = 0, key3_u64 = 0;
        for (int i = 0; i < 8; i++) key1_u64 = (key1_u64 << 8) | key[i];
        if constexpr (keySize >= 16) {
            for (int i = 0; i < 8; i++) key2_u64 = (key2_u64 << 8) | key[8 + i];
        } else {
            key2_u64 = key1_u64;
        }
        key3_u64 = key1_u64;
        
        DES3 des3(key1_u64, key2_u64, key3_u64);
        
        ui64 encRndA_rotated_u64 = 0;
        for (int i = 0; i < 8; i++) {
            encRndA_rotated_u64 = (encRndA_rotated_u64 << 8) | encRndA_rotated[i];
        }
        
        ui64 rndA_rotated_u64 = des3.decrypt(encRndA_rotated_u64);
        
        for (int i = 7; i >= 0; i--) {
            rndA_rotated[i] = static_cast<uint8_t>(rndA_rotated_u64 & 0xFF);
            rndA_rotated_u64 >>= 8;
        }
    }
    
    std::cout << "Decrypted rotated RndA from card: ";
    for (auto byte : rndA_rotated) {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;
    
    // Step 9: Rotate right and verify
    std::array<uint8_t, rndSize> rndA_verified{};
    if constexpr (rndSize == 8) {
        rndA_verified = rotateRight(rndA_rotated);
    } else {
        // AES: 16-byte rotation
        std::rotate_copy(rndA_rotated.begin(), rndA_rotated.end() - 1, rndA_rotated.end(), rndA_verified.begin());
    }
    
    std::cout << "RndA after rotating right: ";
    for (auto byte : rndA_verified) {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;
    
    // Verify RndA matches
    if (rndA == rndA_verified) {
        std::cout << "✅ AUTHENTICATION SUCCESSFUL! RndA matches!" << std::endl;
        Log("DESFire authentication succeeded\n");
        
        // Store session data
        sessionValid_ = true;
        currentKeyType_ = KeyType;
        currentKeyNo_ = keyNo;
        
        // Copy RndA and RndB to session storage (pad to 16 bytes for uniform storage)
        std::copy(rndA.begin(), rndA.end(), sessionRndA_.begin());
        if constexpr (rndSize < 16) {
            std::fill(sessionRndA_.begin() + rndSize, sessionRndA_.end(), 0);
        }
        
        std::copy(rndB.begin(), rndB.end(), sessionRndB_.begin());
        if constexpr (rndSize < 16) {
            std::fill(sessionRndB_.begin() + rndSize, sessionRndB_.end(), 0);
        }
        
        // Store current key (pad to 24 bytes for uniform storage)
        std::copy(key.begin(), key.end(), currentKey_.begin());
        if constexpr (keySize < 24) {
            std::fill(currentKey_.begin() + keySize, currentKey_.end(), 0);
        }
        
        std::cout << "Session established: " << DesfireKeyTraits<KeyType>::Name << "\n";
        return true;
        
    } else {
        std::cout << "❌ AUTHENTICATION FAILED! RndA mismatch!" << std::endl;
        Log("ERROR: Authentication failed - RndA verification mismatch\n");
        sessionValid_ = false;
        return false;
    }
}

// Explicit template instantiations for supported key types
template bool MifareDesfireCard::authenticate<DesfireKeyType::DES>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::DES>::KeySize>&);
template bool MifareDesfireCard::authenticate<DesfireKeyType::DES3_2KEY>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::DES3_2KEY>::KeySize>&);
template bool MifareDesfireCard::authenticate<DesfireKeyType::DES3_3KEY>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::DES3_3KEY>::KeySize>&);
template bool MifareDesfireCard::authenticate<DesfireKeyType::AES>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::AES>::KeySize>&);

template<DesfireKeyType NewKeyType>
bool MifareDesfireCard::changeKey(uint8_t keyNo, const std::array<uint8_t, DesfireKeyTraits<NewKeyType>::KeySize>& newKey) {
    constexpr size_t newKeySize = DesfireKeyTraits<NewKeyType>::KeySize;
    
    std::cout << "\n===============================================\n";
    std::cout << " ChangeKey: Slot " << int(keyNo) << " -> " 
              << DesfireKeyTraits<NewKeyType>::Name << " (" << newKeySize << " bytes)\n";
    std::cout << "===============================================\n";
    
    // === STEP 1: Validate Session ===
    if (!sessionValid_) {
        Log("ERROR: No active authenticated session! Must authenticate first.\n");
        std::cout << "❌ Error: Must authenticate before calling ChangeKey\n";
        return false;
    }
    
    std::cout << "✓ Session valid (current: " << keyAlgoToString(currentKeyType_) << ")\n";
    
    // === STEP 2: XOR new key with old key ===
    std::cout << "\n--- Step 2: XOR new key with current key ---\n";
    
    std::vector<uint8_t> xorKeyData(newKeySize);
    for (size_t i = 0; i < newKeySize; i++) {
        xorKeyData[i] = newKey[i] ^ currentKey_[i];
    }
    
    std::cout << "XOR'd key data: ";
    for (auto b : xorKeyData) std::cout << Hex0x(b) << " ";
    std::cout << "\n";
    
    // === STEP 3: Build CRC data and calculate CRC16 ===
    std::cout << "\n--- Step 3: Calculate CRC16 ---\n";
    
    // CRC calculation differs based on whether we're changing the currently authenticated key:
    // - Changing different key: CRC over [0xC4, keyNo, xorKeyData...]
    // - Changing same key: CRC over [xorKeyData...] ONLY (no command byte, no keyNo)
    bool changingSameKey = (keyNo == currentKeyNo_);
    
    std::vector<uint8_t> crcData;
    
    if (changingSameKey) {
        // Same key: CRC only over XOR'd data
        crcData.insert(crcData.end(), xorKeyData.begin(), xorKeyData.end());
    } else {
        // Different key: CRC over command + keyNo + XOR'd data
        crcData.push_back(0xC4);
        crcData.push_back(keyNo);
        crcData.insert(crcData.end(), xorKeyData.begin(), xorKeyData.end());
    }
    
    uint16_t crc16 = calculateCRC16(crcData.data(), crcData.size());
    
    std::cout << "CRC16 = 0x" << std::hex << std::setw(4) << std::setfill('0') 
              << crc16 << std::dec << " (changing " << (changingSameKey ? "same" : "different") << " key)\n";
    
    // === STEP 4: Build payload (xorKeyData + CRC16 in little-endian) ===
    std::cout << "\n--- Step 4: Build payload ---\n";
    
    // Payload structure:
    // - Same key: [xorKeyData] || [CRC16] (no keyVersion)
    // - Different key: [xorKeyData] || [CRC16] (keyVersion sent unencrypted after)
    
    std::vector<uint8_t> payload = xorKeyData;
    payload.push_back(static_cast<uint8_t>(crc16 & 0xFF));        // CRC LSB
    payload.push_back(static_cast<uint8_t>((crc16 >> 8) & 0xFF)); // CRC MSB
    
    std::cout << "Payload (before encryption): ";
    for (auto b : payload) std::cout << Hex0x(b) << " ";
    std::cout << " (" << payload.size() << " bytes)\n";
    
    // === STEP 5: Derive session key ===
    std::cout << "\n--- Step 5: Derive session key ---\n";
    
    std::vector<uint8_t> sessionKey;
    
    if (currentKeyType_ == DesfireKeyType::DES3_2KEY || currentKeyType_ == DesfireKeyType::DES) {
        // 3DES session key: RndA[0..3] || RndB[0..3] || RndA[4..7] || RndB[4..7]
        sessionKey.resize(16);
        std::copy(sessionRndA_.begin(), sessionRndA_.begin() + 4, sessionKey.begin());
        std::copy(sessionRndB_.begin(), sessionRndB_.begin() + 4, sessionKey.begin() + 4);
        std::copy(sessionRndA_.begin() + 4, sessionRndA_.begin() + 8, sessionKey.begin() + 8);
        std::copy(sessionRndB_.begin() + 4, sessionRndB_.begin() + 8, sessionKey.begin() + 12);
        
        std::cout << "3DES session key derived: ";
        for (auto b : sessionKey) std::cout << Hex0x(b) << " ";
        std::cout << "\n";
        
    } else if (currentKeyType_ == DesfireKeyType::AES) {
        // AES session key: RndA[0..3] || RndB[0..3] || RndA[12..15] || RndB[12..15]
        sessionKey.resize(16);
        std::copy(sessionRndA_.begin(), sessionRndA_.begin() + 4, sessionKey.begin());
        std::copy(sessionRndB_.begin(), sessionRndB_.begin() + 4, sessionKey.begin() + 4);
        std::copy(sessionRndA_.begin() + 12, sessionRndA_.begin() + 16, sessionKey.begin() + 8);
        std::copy(sessionRndB_.begin() + 12, sessionRndB_.begin() + 16, sessionKey.begin() + 12);
        
        std::cout << "AES session key derived: ";
        for (auto b : sessionKey) std::cout << Hex0x(b) << " ";
        std::cout << "\n";
        
    } else {
        Log("ERROR: Unknown current key type for session key derivation\n");
        return false;
    }
    
    // === STEP 6: Encrypt payload with session key ===
    std::cout << "\n--- Step 6: Encrypt payload ---\n";
    
    std::vector<uint8_t> encryptedPayload;
    
    if (currentKeyType_ == DesfireKeyType::DES3_2KEY || currentKeyType_ == DesfireKeyType::DES) {
        // Use 3DES CBC with IV=0
        ui64 key1_u64 = 0, key2_u64 = 0;
        for (int i = 0; i < 8; i++) {
            key1_u64 = (key1_u64 << 8) | sessionKey[i];
            key2_u64 = (key2_u64 << 8) | sessionKey[8 + i];
        }
        
        DES3CBC des3cbc(key1_u64, key2_u64, key1_u64, 0x0000000000000000ULL);
        
        // Pad payload to multiple of 8 bytes (3DES block size) with zeros
        size_t paddedSize = ((payload.size() + 7) / 8) * 8;
        std::vector<uint8_t> paddedPayload = payload;
        paddedPayload.resize(paddedSize, 0x00);  // Pad with zeros
        
        std::cout << "Padded payload: ";
        for (auto b : paddedPayload) std::cout << Hex0x(b) << " ";
        std::cout << " (" << paddedPayload.size() << " bytes)\n";
        
        // Encrypt in 8-byte blocks
        encryptedPayload.resize(paddedSize);
        for (size_t blockIdx = 0; blockIdx < paddedPayload.size(); blockIdx += 8) {
            ui64 block = 0;
            for (size_t i = 0; i < 8; i++) {
                block = (block << 8) | paddedPayload[blockIdx + i];
            }
            
            ui64 encBlock = des3cbc.encrypt(block);
            
            for (int i = 7; i >= 0; i--) {
                encryptedPayload[blockIdx + i] = static_cast<uint8_t>(encBlock & 0xFF);
                encBlock >>= 8;
            }
        }
        
    } else if (currentKeyType_ == DesfireKeyType::AES) {
        // Use AES-128 CBC with IV=0
        struct AES_ctx ctx;
        uint8_t iv[16] = {0};
        AES_init_ctx_iv(&ctx, sessionKey.data(), iv);
        
        // Pad payload to multiple of 16 bytes
        size_t paddedSize = ((payload.size() + 15) / 16) * 16;
        encryptedPayload.resize(paddedSize);
        std::copy(payload.begin(), payload.end(), encryptedPayload.begin());
        std::fill(encryptedPayload.begin() + payload.size(), encryptedPayload.end(), 0);
        
        AES_CBC_encrypt_buffer(&ctx, encryptedPayload.data(), encryptedPayload.size());
    }
    
    std::cout << "Encrypted payload: ";
    for (auto b : encryptedPayload) std::cout << Hex0x(b) << " ";
    std::cout << " (" << encryptedPayload.size() << " bytes)\n";
    
    // === STEP 7: Build ChangeKey APDU ===
    std::cout << "\n--- Step 7: Build ChangeKey APDU ---\n";
    
    // Calculate keyVersion byte (only used for different-key changes)
    uint8_t keyVersion = makeKeyVersion(NewKeyType, 0);  // revision 0
    
    std::vector<uint8_t> apdu;
    apdu.push_back(0x90);                              // CLA
    apdu.push_back(0xC4);                              // INS = ChangeKey
    apdu.push_back(0x00);                              // P1
    apdu.push_back(0x00);                              // P2
    
    // Lc calculation differs based on same vs different key:
    // - Same key: Lc = 1 (keyNo) + encryptedPayload.size() (no keyVersion)
    // - Different key: Lc = 1 (keyNo) + encryptedPayload.size() + 1 (keyVersion unencrypted)
    if (changingSameKey) {
        apdu.push_back(encryptedPayload.size() + 1);   // Lc = keyNo + encrypted (no keyVersion)
    } else {
        apdu.push_back(encryptedPayload.size() + 2);   // Lc = keyNo + encrypted + keyVersion
    }
    
    apdu.push_back(keyNo);                             // Key number
    apdu.insert(apdu.end(), encryptedPayload.begin(), encryptedPayload.end());
    
    // For different-key changes, keyVersion is sent unencrypted after the cryptogram
    if (!changingSameKey) {
        apdu.push_back(keyVersion);                    // Key version (unencrypted)
    }
    
    apdu.push_back(0x00);                              // Le
    
    std::cout << "APDU: ";
    for (auto b : apdu) std::cout << Hex0x(b) << " ";
    std::cout << "\n";
    
    if (changingSameKey) {
        std::cout << "  CLA=0x90, INS=0xC4, keyNo=" << int(keyNo) << " (same-key, no keyVersion)\n";
    } else {
        std::cout << "  CLA=0x90, INS=0xC4, keyNo=" << int(keyNo) 
                  << ", keyVersion=0x" << std::hex << int(keyVersion) << std::dec << "\n";
    }
    
    // === STEP 8: Send command ===
    std::cout << "\n--- Step 8: Send ChangeKey command ---\n";
    
    using namespace NFC_Controller::Cpp;

    InDataExchangeCommand::Options opts;
    opts.payload = apdu;
    opts.responseTimeoutMs = 2000;
    auto command = InDataExchangeCommand(opts);
    
    auto result = nfc_->executeCommand(command);
    
    if (result.status != pn532Response::statusCode::OK) {
        Log("ERROR: ChangeKey failed with PN532 status: " + std::to_string(static_cast<int>(result.status)) + "\n");
        std::cout << "❌ PN532 communication error\n";
        return false;
    }
    
    std::cout << "Card response: ";
    for (auto byte : result.responsePayload) {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << "\n";
    
    // === STEP 9: Parse response ===
    if (result.responsePayload.size() < 2) {
        Log("ERROR: ChangeKey response too short\n");
        std::cout << "❌ Invalid response length\n";
        return false;
    }
    
    uint8_t sw1 = result.responsePayload[result.responsePayload.size() - 2];
    uint8_t sw2 = result.responsePayload[result.responsePayload.size() - 1];
    
    if (sw1 == 0x91 && sw2 == 0x00) {
        std::cout << "\n!! ChangeKey SUCCESS! Key slot " << int(keyNo) 
                  << " now uses " << DesfireKeyTraits<NewKeyType>::Name << "\n";
        
        std::cout << "New Key: ";
        for (auto b : newKey) std::cout << Hex0x(b) << " ";
        std::cout << "\n";

        Log("ChangeKey succeeded for key #" + std::to_string(keyNo) + "\n");
        
        // Update stored current key and type
        std::copy(newKey.begin(), newKey.end(), currentKey_.begin());
        currentKeyType_ = NewKeyType;
        
        // Invalidate session (DESFire spec: session ends after ChangeKey)
        sessionValid_ = false;
        std::cout << " Session invalidated (re-authenticate required)\n";
        
        return true;
        
    } else if (sw1 == 0x91 && sw2 == 0xAE) {
        Log("ERROR: ChangeKey authentication error (wrong CRC or session key)\n");
        std::cout << "❌ Authentication error (0x91 0xAE): check CRC/session key\n";
        sessionValid_ = false;
        return false;
        
    } else {
        Log("ERROR: ChangeKey failed with status 0x" + std::to_string(sw1) + " 0x" + std::to_string(sw2) + "\n");
        std::cout << "❌ Card error: SW1=0x" << std::hex << int(sw1) 
                  << " SW2=0x" << int(sw2) << std::dec << "\n";
        sessionValid_ = false;
        return false;
    }
    return false;
}

// Explicit template instantiations for supported key types
template bool MifareDesfireCard::changeKey<DesfireKeyType::DES>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::DES>::KeySize>&);
template bool MifareDesfireCard::changeKey<DesfireKeyType::DES3_2KEY>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::DES3_2KEY>::KeySize>&);
template bool MifareDesfireCard::changeKey<DesfireKeyType::DES3_3KEY>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::DES3_3KEY>::KeySize>&);
template bool MifareDesfireCard::changeKey<DesfireKeyType::AES>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::AES>::KeySize>&);

void MifareDesfireCard::authenticateAES(uint8_t keyNo, const std::array<uint8_t, 16> &RndB)
{
    Log("Executing DESFire AuthenticateAES command via InDataExchange on keyNo " + std::to_string(keyNo) + "\n");

    this->selectApplication(0x000000); // Select PICC/master application first
    uint8_t cmd[] = {0x90, 0xAA, keyNo, 0x00, 0x01, keyNo, 0x00};

    using namespace NFC_Controller::Cpp;

    auto res = getDesfireFullResponse(std::vector<uint8_t>(cmd, cmd + sizeof(cmd)));
    std::cout << "authenticateAES response: ";
    for (auto byte : res)
    {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::dec << std::endl;


    // auto command = InDataExchangeCommand(
    //     InDataExchangeCommand::Options{
    //         .payload = std::vector<uint8_t>(cmd, cmd + sizeof(cmd)),
    //         .responseTimeoutMs = 2000});

    // auto result = nfc_->executeCommand(command);

    // if (result.status != pn532Response::statusCode::OK)
    // {
    //     Log("ERROR: authenticateAES failed with status: " + std::to_string(static_cast<int>(result.status)) + "\n");
    //     return;
    // }

    // std::cout << "authenticateAES response: ";
    // for (auto byte : result.responsePayload)
    // {
    //     std::cout << "0x" << Hex0x(byte) << " ";
    // }
    // std::cout << std::dec << std::endl;

    return;
}

bool MifareDesfireCard::getVersion(DesfireVersionInfo &versionInfo)
{
    if (!nfc_)
    {
        Log("ERROR: NFC interface not available for getVersion\n");
        return false;
    }

    // Return cached version if already retrieved
    if (versionRetrieved_)
    {
        Log("Returning cached DESFire version info\n");
        versionInfo = versionInfo_;
        return true;
    }

    Log("Executing DESFire GetVersion command via InDataExchange\n");

    // DESFire GetVersion command in ISO 7816-4 wrapped format
    // CLA=0x90 (proprietary), INS=0x60 (GetVersion), P1=0x00, P2=0x00, Le=0x00
    uint8_t command[] = {
        0x90, 0x60, 0x00, 0x00, 0x00};

    uint8_t frameIndex = 0;
    uint8_t responseSize = 0;
    uint8_t responseBuffer[255] = {0};

    // DESFire GetVersion returns data in 3 frames (hardware, software, manufacturing)
    // Each response ends with status bytes (0x91 0xAF for "more data" or 0x91 0x00 for "done")
    do
    {
        // Reset response buffer for each frame
        responseSize = 0;
        std::memset(responseBuffer, 0, sizeof(responseBuffer));

        // Send command and get response via InDataExchange (PN532 command 0x40)
        auto result = nfc_->initDataExchange(command, sizeof(command), responseBuffer, responseSize);
        if (result != NFC_Controller::Cpp::statusCode::pn532StatusOK)
        {
            Log("ERROR: initDataExchange failed with status: " + std::to_string(static_cast<int>(result)) + "\n");
            return false;
        }

        if (responseSize < 2)
        {
            Log("ERROR: Response too short (got " + std::to_string(responseSize) + " bytes)\n");
            return false;
        }

        std::cout << "GetVersion response frame " << int(frameIndex) << " (" << int(responseSize) << " bytes): ";
        for (uint8_t i = 0; i < responseSize; i++)
        {
            std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(responseBuffer[i]) << " ";
        }
        std::cout << std::dec << std::endl;

        // Check status bytes (last 2 bytes of response)
        uint8_t statusByte1 = responseBuffer[responseSize - 2];
        uint8_t statusByte2 = responseBuffer[responseSize - 1];

        // Parse the data portion (everything except last 2 status bytes)
        if (!parseVersionFrame(responseBuffer, responseSize - 2, frameIndex))
        {
            Log("ERROR: Failed to parse response frame " + std::to_string(frameIndex) + "\n");
            return false;
        }

        // For subsequent frames, send continuation command (0xAF = Additional Frame)
        if (frameIndex == 0)
        {
            command[1] = 0xAF; // Change INS from 0x60 to 0xAF
        }

        frameIndex++;

        // Check if more frames are coming (status = 0x91 0xAF)
        // Status 0x91 0x00 or 0x90 0x00 indicates success/completion
        if (!(statusByte1 == 0x91 && statusByte2 == 0xAF))
        {
            Log("DESFire GetVersion completed after " + std::to_string(frameIndex) + " frames\n");
            break;
        }

    } while (frameIndex < 3); // Safety limit: max 3 frames expected

    if (frameIndex != 3)
    {
        Log("WARNING: Expected 3 frames but got " + std::to_string(frameIndex) + "\n");
    }

    versionRetrieved_ = true;
    versionInfo = versionInfo_;

    // Output full version information
    std::cout << versionInfo_ << std::endl;

    return true;
}

// Returns true on success and fills 'version'
bool MifareDesfireCard::getKeyVersion(uint8_t keyNo, uint8_t &version)
{
    using namespace NFC_Controller::Cpp;

    // Build DESFire APDU: GetKeyVersion for keyNo
    uint8_t apdu[] = { 0x90, 0x64, 0x00, 0x00, 0x01, keyNo, 0x00 };

    auto command = InDataExchangeCommand(
        InDataExchangeCommand::Options{
            .payload = std::vector<uint8_t>(apdu, apdu + sizeof(apdu)),
            .responseTimeoutMs = 2000
    });

    auto result = nfc_->executeCommand(command);

    if (result.status != pn532Response::statusCode::OK) {
        Log("ERROR: getKeyVersion PN532 transport failure: " + std::to_string(static_cast<int>(result.status)) + "\n");
        return false;
    }

    // Now check card-level status using the InDataExchange parser helpers if available
    // If you have access to the command instance after execution, use command.getStatus/getResponseData
    // But result.responsePayload contains the data bytes returned by the card
    if (result.responsePayload.empty()) {
        Log("ERROR: getKeyVersion returned empty payload\n");
        return false;
    }

    // Last two bytes of the card's low-level payload are status bytes; your InDataExchangeCommand
    // already strips the transport status byte and leaves card data in responsePayload.
    // In practice responsePayload should contain the keyVersion byte (single byte).
    version = result.responsePayload[0];

    std::cout << "GetKeyVersion(keyNo=" << int(keyNo) << ") = 0x" << std::hex << int(version) << std::dec << "\n";

    return true;
}

uint8_t MifareDesfireCard::getDesfireVariant()
{
    if (!versionRetrieved_)
    {
        DesfireVersionInfo versionInfo;
        if (!getVersion(versionInfo))
        {
            Log("ERROR: Failed to retrieve DESFire version\n");
            return 0; // Unknown
        }
    }

    // Determine variant based on software major version
    // DESFire EV1: SW version 1.x (0x01)
    // DESFire EV2: SW version 2.x (0x02)
    // DESFire EV3: SW version 3.x (0x03)
    uint8_t swMajor = versionInfo_.softwareInfo.swMajorVersion;

    if (swMajor == 0x01)
    {
        Log("Identified as DESFire EV1 (SW version 1.x)\n");
        return 1;
    }
    else if (swMajor == 0x02)
    {
        Log("Identified as DESFire EV2 (SW version 2.x)\n");
        return 2;
    }
    else if (swMajor == 0x03)
    {
        Log("Identified as DESFire EV3 (SW version 3.x)\n");
        return 3;
    }

    Log("WARNING: Unknown DESFire variant (SW Major version: 0x" +
        std::to_string(swMajor) + ")\n");
    return 0;
}


std::vector<uint8_t> MifareDesfireCard::getDesfireFullResponse(std::vector<uint8_t> initialApdu, int maxFrames)
{
    std::vector<uint8_t> aggregated;
    std::vector<uint8_t> apdu = initialApdu;

    int frame = 0;
    uint8_t statusByte1 = 0;    // Bytes to indicate more frames
    uint8_t statusByte2 = 0;    // 

    do{
        std::vector<uint8_t> responseBuffer;
        using namespace NFC_Controller::Cpp;

        auto command = InDataExchangeCommand(
            InDataExchangeCommand::Options{
                .payload = apdu,
                .responseTimeoutMs = 2000});

        auto result = nfc_->executeCommand(command);

        if (result.status != pn532Response::statusCode::OK)
        {
            Log("ERROR: getDesfireFullResponse initDataExchange failed with status: " + std::to_string(static_cast<int>(result.status)) + "\n");
            return {};
        }

        responseBuffer = result.responsePayload;

        if (responseBuffer.size() < 2)
        {
            Log("ERROR: getDesfireFullResponse Response too short (got " + std::to_string(responseBuffer.size()) + " bytes)\n");
            return {};
        }

        // Extract status bytes
        statusByte1 = responseBuffer[responseBuffer.size() - 2];
        statusByte2 = responseBuffer[responseBuffer.size() - 1];

        // Append data portion (excluding status bytes)
        aggregated.insert(aggregated.end(), responseBuffer.begin(), responseBuffer.end() - 2);

        // Prepare next APDU for continuation if needed
        if (statusByte1 == 0x91 && statusByte2 == 0xAF)
        {
            apdu = {0x90, 0xAF, 0x00, 0x00, 0x00}; // Continuation command
        }

        frame++;
    } while (statusByte1 == 0x91 && statusByte2 == 0xAF && frame < maxFrames);

        // Accept final success pairs 0x91/0x00 or 0x90/0x00
    if (!((statusByte1 == 0x91 && statusByte2 == 0x00) || (statusByte1 == 0x90 && statusByte2 == 0x00))) {
        Log("DESFire returned error status\n");
        return {};
    }

    return aggregated;
}