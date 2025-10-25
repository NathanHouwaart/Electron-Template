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

namespace
{
    std::array<uint8_t, 8> rotateLeft(std::array<uint8_t, 8> value)
    {
        std::array<uint8_t, 8> out{};
        // Rotate left by 1: [a,b,c,d,e,f,g,h] -> [b,c,d,e,f,g,h,a]
        std::rotate_copy(value.begin(), value.begin() + 1, value.end(), out.begin());
        return out;
    }
    std::array<uint8_t, 8> rotateRight(std::array<uint8_t, 8> value)
    {
        std::array<uint8_t, 8> out{};
        // Rotate right by 1: [a,b,c,d,e,f,g,h] -> [h,a,b,c,d,e,f,g]
        std::rotate_copy(value.begin(), value.end() - 1, value.end(), out.begin());
        return out;
    }

    // CRC16 calculation for DESFire (ISO 14443-3 Type A)
    // Polynomial: 0x8005, Initial value: 0x6363
    // This is used for ChangeKey command payload integrity
    uint16_t calculateCRC16(const uint8_t *data, size_t length)
    {
        uint16_t crc = 0x6363; // ISO/IEC 14443-3 Type A preset

        for (size_t i = 0; i < length; ++i)
        {
            crc ^= static_cast<uint16_t>(data[i]);
            for (int bit = 0; bit < 8; ++bit)
            {
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
        0x90,                                     // CLA
        0x5A,                                     // INS = SelectApplication
        0x00,                                     // P1
        0x00,                                     // P2
        0x03,                                     // Lc = length of AID
        static_cast<uint8_t>((aid >> 16) & 0xFF), // AID byte 1
        static_cast<uint8_t>((aid >> 8) & 0xFF),  // AID byte 2
        static_cast<uint8_t>(aid & 0xFF),         // AID byte 3
        0x00                                      // Le
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

    if (!command.isSuccess())
    {
        Log("ERROR: selectApplication command returned failure status\n");
        return;
    }

    Log("selectApplication succeeded on AID " + std::to_string(aid) + "\n");

    return;
}

template <DesfireKeyType KeyType>
bool MifareDesfireCard::authenticate(uint8_t keyNo, const std::array<uint8_t, DesfireKeyTraits<KeyType>::KeySize> &key)
{
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
    uint8_t cmd[] = {0x90, insCode, 0x00, 0x00, 0x01, keyNo, 0x00};
    auto command = InDataExchangeCommand(
        InDataExchangeCommand::Options{
            .payload = std::vector<uint8_t>(cmd, cmd + sizeof(cmd)),
            .responseTimeoutMs = 2000});
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
        result.responsePayload[result.responsePayload.size() - 1] != 0xAF)
    {
        Log("ERROR: unexpected DESFire auth response\n");
        std::cout << "❌ Unexpected response format\n";
        return false;
    }

    // Step 2: Decrypt RndB
    std::array<uint8_t, rndSize> encRndB{};
    std::copy_n(result.responsePayload.begin(), rndSize, encRndB.begin());

    std::cout << "Encrypted RndB bytes: ";
    for (auto byte : encRndB)
    {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;

    std::array<uint8_t, rndSize> rndB{};

    if constexpr (KeyType == DesfireKeyType::AES)
    {
        // AES decryption (16-byte block)
        struct AES_ctx ctx;
        uint8_t iv[16] = {0};
        AES_init_ctx_iv(&ctx, key.data(), iv);

        std::copy(encRndB.begin(), encRndB.end(), rndB.begin());
        AES_CBC_decrypt_buffer(&ctx, rndB.data(), rndB.size());
    }
    else
    {
        // 3DES decryption (8-byte block)
        ui64 key1_u64 = 0, key2_u64 = 0, key3_u64 = 0;

        // Extract key components based on key size
        for (int i = 0; i < 8; i++)
        {
            key1_u64 = (key1_u64 << 8) | key[i];
        }

        if constexpr (keySize >= 16)
        {
            for (int i = 0; i < 8; i++)
            {
                key2_u64 = (key2_u64 << 8) | key[8 + i];
            }
        }
        else
        {
            key2_u64 = key1_u64; // Single DES uses same key
        }

        if constexpr (keySize >= 24)
        {
            for (int i = 0; i < 8; i++)
            {
                key3_u64 = (key3_u64 << 8) | key[16 + i];
            }
        }
        else
        {
            key3_u64 = key1_u64; // 2-key 3DES: K3 = K1
        }

        DES3 des3(key1_u64, key2_u64, key3_u64);

        // Convert RndB to ui64 (big-endian)
        ui64 encRndB_u64 = 0;
        for (int i = 0; i < 8; i++)
        {
            encRndB_u64 = (encRndB_u64 << 8) | encRndB[i];
        }

        // Decrypt
        ui64 rndB_u64 = des3.decrypt(encRndB_u64);

        // Convert back to byte array
        for (int i = 7; i >= 0; i--)
        {
            rndB[i] = static_cast<uint8_t>(rndB_u64 & 0xFF);
            rndB_u64 >>= 8;
        }
    }

    std::cout << "Decrypted RndB: ";
    for (auto byte : rndB)
    {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;

    // Step 3: Generate RndA
    std::array<uint8_t, rndSize> rndA{};
    {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint16_t> dis(0, 255);
        for (auto &byte : rndA)
        {
            byte = static_cast<uint8_t>(dis(gen));
        }
    }

    std::cout << "Generated RndA: ";
    for (auto byte : rndA)
    {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;

    // Step 4: Rotate RndB left by 1 byte
    std::array<uint8_t, rndSize> rndB_rotated{};
    if constexpr (rndSize == 8)
    {
        rndB_rotated = rotateLeft(rndB);
    }
    else
    {
        // AES: 16-byte rotation
        std::rotate_copy(rndB.begin(), rndB.begin() + 1, rndB.end(), rndB_rotated.begin());
    }

    std::cout << "RndB rotated left: ";
    for (auto byte : rndB_rotated)
    {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;

    // Step 5: Build host challenge: RndA || RndB'
    std::array<uint8_t, rndSize * 2> hostChallenge{};
    std::copy(rndA.begin(), rndA.end(), hostChallenge.begin());
    std::copy(rndB_rotated.begin(), rndB_rotated.end(), hostChallenge.begin() + rndSize);

    std::cout << "Host challenge (RndA || RndB'): ";
    for (auto byte : hostChallenge)
    {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;

    // Step 6: Encrypt host challenge
    std::array<uint8_t, rndSize * 2> encHostChallenge{};

    if constexpr (KeyType == DesfireKeyType::AES)
    {
        // AES encryption
        struct AES_ctx ctx;
        uint8_t iv[16] = {0};
        AES_init_ctx_iv(&ctx, key.data(), iv);

        std::copy(hostChallenge.begin(), hostChallenge.end(), encHostChallenge.begin());
        AES_CBC_encrypt_buffer(&ctx, encHostChallenge.data(), encHostChallenge.size());
    }
    else
    {
        // 3DES CBC encryption
        ui64 key1_u64 = 0, key2_u64 = 0, key3_u64 = 0;
        for (int i = 0; i < 8; i++)
            key1_u64 = (key1_u64 << 8) | key[i];
        if constexpr (keySize >= 16)
        {
            for (int i = 0; i < 8; i++)
                key2_u64 = (key2_u64 << 8) | key[8 + i];
        }
        else
        {
            key2_u64 = key1_u64;
        }
        key3_u64 = key1_u64; // For 2-key 3DES

        DES3CBC des3cbc(key1_u64, key2_u64, key3_u64, 0x0000000000000000ULL);

        // Encrypt two 8-byte blocks
        for (size_t blockIdx = 0; blockIdx < 2; blockIdx++)
        {
            ui64 block_u64 = 0;
            for (size_t i = 0; i < 8; i++)
            {
                block_u64 = (block_u64 << 8) | hostChallenge[blockIdx * 8 + i];
            }

            ui64 encBlock_u64 = des3cbc.encrypt(block_u64);

            for (int i = 7; i >= 0; i--)
            {
                encHostChallenge[blockIdx * 8 + i] = static_cast<uint8_t>(encBlock_u64 & 0xFF);
                encBlock_u64 >>= 8;
            }
        }
    }

    std::cout << "Encrypted host challenge: ";
    for (auto byte : encHostChallenge)
    {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;

    // Step 7: Send encrypted challenge via Additional Frame
    std::vector<uint8_t> afCmd;
    afCmd.push_back(0x90);        // CLA
    afCmd.push_back(0xAF);        // INS = Additional Frame
    afCmd.push_back(0x00);        // P1
    afCmd.push_back(0x00);        // P2
    afCmd.push_back(rndSize * 2); // Lc
    afCmd.insert(afCmd.end(), encHostChallenge.begin(), encHostChallenge.end());
    afCmd.push_back(0x00); // Le

    auto afCommand = InDataExchangeCommand(
        InDataExchangeCommand::Options{
            .payload = afCmd,
            .responseTimeoutMs = 2000});
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
        afResult.responsePayload[afResult.responsePayload.size() - 1] != 0x00)
    {
        Log("ERROR: unexpected card verification response\n");
        std::cout << "❌ Verification failed\n";
        return false;
    }

    // Step 8: Decrypt and verify RndA'
    std::array<uint8_t, rndSize> encRndA_rotated{};
    std::copy_n(afResult.responsePayload.begin(), rndSize, encRndA_rotated.begin());

    std::array<uint8_t, rndSize> rndA_rotated{};

    if constexpr (KeyType == DesfireKeyType::AES)
    {
        // AES decryption
        struct AES_ctx ctx;
        uint8_t iv[16] = {0};
        AES_init_ctx_iv(&ctx, key.data(), iv);

        std::copy(encRndA_rotated.begin(), encRndA_rotated.end(), rndA_rotated.begin());
        AES_CBC_decrypt_buffer(&ctx, rndA_rotated.data(), rndA_rotated.size());
    }
    else
    {
        // 3DES decryption
        ui64 key1_u64 = 0, key2_u64 = 0, key3_u64 = 0;
        for (int i = 0; i < 8; i++)
            key1_u64 = (key1_u64 << 8) | key[i];
        if constexpr (keySize >= 16)
        {
            for (int i = 0; i < 8; i++)
                key2_u64 = (key2_u64 << 8) | key[8 + i];
        }
        else
        {
            key2_u64 = key1_u64;
        }
        key3_u64 = key1_u64;

        DES3 des3(key1_u64, key2_u64, key3_u64);

        ui64 encRndA_rotated_u64 = 0;
        for (int i = 0; i < 8; i++)
        {
            encRndA_rotated_u64 = (encRndA_rotated_u64 << 8) | encRndA_rotated[i];
        }

        ui64 rndA_rotated_u64 = des3.decrypt(encRndA_rotated_u64);

        for (int i = 7; i >= 0; i--)
        {
            rndA_rotated[i] = static_cast<uint8_t>(rndA_rotated_u64 & 0xFF);
            rndA_rotated_u64 >>= 8;
        }
    }

    std::cout << "Decrypted rotated RndA from card: ";
    for (auto byte : rndA_rotated)
    {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;

    // Step 9: Rotate right and verify
    std::array<uint8_t, rndSize> rndA_verified{};
    if constexpr (rndSize == 8)
    {
        rndA_verified = rotateRight(rndA_rotated);
    }
    else
    {
        // AES: 16-byte rotation
        std::rotate_copy(rndA_rotated.begin(), rndA_rotated.end() - 1, rndA_rotated.end(), rndA_verified.begin());
    }

    std::cout << "RndA after rotating right: ";
    for (auto byte : rndA_verified)
    {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;

    // Verify RndA matches
    if (rndA == rndA_verified)
    {
        std::cout << "✅ AUTHENTICATION SUCCESSFUL! RndA matches!" << std::endl;
        Log("DESFire authentication succeeded\n");

        // Store session data
        sessionValid_ = true;
        currentKeyType_ = KeyType;
        currentKeyNo_ = keyNo;

        // Copy RndA and RndB to session storage (pad to 16 bytes for uniform storage)
        std::copy(rndA.begin(), rndA.end(), sessionRndA_.begin());
        if constexpr (rndSize < 16)
        {
            std::fill(sessionRndA_.begin() + rndSize, sessionRndA_.end(), 0);
        }

        std::copy(rndB.begin(), rndB.end(), sessionRndB_.begin());
        if constexpr (rndSize < 16)
        {
            std::fill(sessionRndB_.begin() + rndSize, sessionRndB_.end(), 0);
        }

        std::cout << "Stored RndA: ";
        for (auto byte : sessionRndA_)
        {
            std::cout << Hex0x(byte) << " ";
        }
        std::cout << std::endl;

        std::cout << "Stored RndB: ";
        for (auto byte : sessionRndB_)
        {
            std::cout << Hex0x(byte) << " ";
        }
        std::cout << std::endl;

    // Calculate and store session key
    // Ensure we start from an empty vector, reserve 16 bytes capacity
    sessionKey_.clear();
    sessionKey_.reserve(16);
    // sessionKey = rnda[0..3] || rndb[0..3] || rnda[4..7] || rndb[4..7]
    sessionKey_.insert(sessionKey_.end(), sessionRndA_.begin(), sessionRndA_.begin() + 4);
    sessionKey_.insert(sessionKey_.end(), sessionRndB_.begin(), sessionRndB_.begin() + 4);
    // Only insert bytes 4..7 (4 bytes) rather than to .end() which would add padded zeros
    sessionKey_.insert(sessionKey_.end(), sessionRndA_.begin() + 4, sessionRndA_.begin() + 8);
    sessionKey_.insert(sessionKey_.end(), sessionRndB_.begin() + 4, sessionRndB_.begin() + 8);

        std::cout << "Derived session key: ";
        for (auto byte : sessionKey_)
        {
            std::cout << Hex0x(byte) << " ";
        }
        std::cout << std::dec << std::endl;

        // Store current key (pad to 24 bytes for uniform storage)
        std::copy(key.begin(), key.end(), currentKey_.begin());
        if constexpr (keySize < 24)
        {
            std::fill(currentKey_.begin() + keySize, currentKey_.end(), 0);
        }

        std::cout << "Session established: " << DesfireKeyTraits<KeyType>::Name << "\n";
        return true;
    }
    else
    {
        std::cout << "❌ AUTHENTICATION FAILED! RndA mismatch!" << std::endl;
        Log("ERROR: Authentication failed - RndA verification mismatch\n");
        sessionValid_ = false;
        return false;
    }
}

// Explicit template instantiations for supported key types
template bool MifareDesfireCard::authenticate<DesfireKeyType::DES>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::DES>::KeySize> &);
template bool MifareDesfireCard::authenticate<DesfireKeyType::DES3_2KEY>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::DES3_2KEY>::KeySize> &);
template bool MifareDesfireCard::authenticate<DesfireKeyType::DES3_3KEY>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::DES3_3KEY>::KeySize> &);
template bool MifareDesfireCard::authenticate<DesfireKeyType::AES>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::AES>::KeySize> &);

template <DesfireKeyType NewKeyType>
bool MifareDesfireCard::changeKey(uint8_t keyNo, const std::array<uint8_t, DesfireKeyTraits<NewKeyType>::KeySize> &newKey)
{
    constexpr size_t newKeySize = DesfireKeyTraits<NewKeyType>::KeySize;

    std::cout << "\n===============================================\n";
    std::cout << " ChangeKey: Slot " << int(keyNo) << " -> "
              << DesfireKeyTraits<NewKeyType>::Name << " (" << newKeySize << " bytes)\n";
    std::cout << "===============================================\n";

    // === STEP 1: Validate Session ===
    std::cout << "--- Step 1: Valudate session ---\n";
    if (!sessionValid_)
    {
        Log("ERROR: No active authenticated session! Must authenticate first.\n");
        std::cout << "❌ Error: Must authenticate before calling ChangeKey\n";
        return false;
    }

    std::cout << "✓ Session valid (current: " << keyAlgoToString(currentKeyType_) << ")\n";

    // === STEP 2: Build ChangeKey command payload ===
    std::cout << "--- Step 2: Build ChangeKey command payload ---\n";
    std::array<uint8_t, 8> iv = {0}; // Initial IV = 0s

    
    

    return true;
}

// Explicit template instantiations for supported key types
template bool MifareDesfireCard::changeKey<DesfireKeyType::DES>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::DES>::KeySize> &);
template bool MifareDesfireCard::changeKey<DesfireKeyType::DES3_2KEY>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::DES3_2KEY>::KeySize> &);
template bool MifareDesfireCard::changeKey<DesfireKeyType::DES3_3KEY>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::DES3_3KEY>::KeySize> &);
template bool MifareDesfireCard::changeKey<DesfireKeyType::AES>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::AES>::KeySize> &);

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
    uint8_t apdu[] = {0x90, 0x64, 0x00, 0x00, 0x01, keyNo, 0x00};

    auto command = InDataExchangeCommand(
        InDataExchangeCommand::Options{
            .payload = std::vector<uint8_t>(apdu, apdu + sizeof(apdu)),
            .responseTimeoutMs = 2000});

    auto result = nfc_->executeCommand(command);

    if (result.status != pn532Response::statusCode::OK)
    {
        Log("ERROR: getKeyVersion PN532 transport failure: " + std::to_string(static_cast<int>(result.status)) + "\n");
        return false;
    }

    // Now check card-level status using the InDataExchange parser helpers if available
    // If you have access to the command instance after execution, use command.getStatus/getResponseData
    // But result.responsePayload contains the data bytes returned by the card
    if (result.responsePayload.empty())
    {
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
    uint8_t statusByte1 = 0; // Bytes to indicate more frames
    uint8_t statusByte2 = 0; //

    do
    {
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
    if (!((statusByte1 == 0x91 && statusByte2 == 0x00) || (statusByte1 == 0x90 && statusByte2 == 0x00)))
    {
        Log("DESFire returned error status\n");
        return {};
    }

    return aggregated;
}