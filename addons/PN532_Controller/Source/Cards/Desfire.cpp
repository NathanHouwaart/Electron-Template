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
#include "../../Headers/Utils/KeyUtils.h"
#include "../desfireKey.h"
#include <span>
#include "etl/vector.h"

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

// Returns number of bytes written to `response`, or -1 on error.
bool MifareDesfireCard::executeCommand(const etl::ivector<uint8_t> &command, etl::ivector<uint8_t> &response)
{
    // Placeholder for command execution logic
    std::cout << "Executing command with DesfireKey\n";

    using namespace NFC_Controller::Cpp;
    InDataExchangeCommand inDataExchangeCmd(
        InDataExchangeCommand::Options{
            .payload = std::vector<uint8_t>(command.begin(), command.end()),
            .responseTimeoutMs = 2000});
    auto result = nfc_->executeCommand(inDataExchangeCmd);

    if (result.status != pn532Response::statusCode::OK)
    {
        Log("ERROR: ExecuteCommand failed with status: " + std::to_string(static_cast<int>(result.status)) + "\n");
        std::cout << "❌ ExecuteCommand failed\n";
        return false;
    }

    std::cout << "execute command response: ";
    for (auto byte : result.responsePayload)
    {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::dec << std::endl;

    const auto n = static_cast<int>(result.responsePayload.size());
    std::cout << "Response size: " << n << " bytes\n";
    std::cout << "Destination capacity: " << response.capacity() << " bytes\n";

    if (n > static_cast<int>(response.capacity()))
    {
        Log("ERROR: Response buffer too small\n");
        std::cout << "❌ Response buffer too small\n";
        return false;
    }

    response.assign(result.responsePayload.begin(), result.responsePayload.end());

    return true;
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
using namespace NFC_Controller::Cpp;

template <DesfireKeyType KeyType>
bool MifareDesfireCard::authenticate(uint8_t keyNo, const std::array<uint8_t, DesfireKeyTraits<KeyType>::keySize> &key)
{
    using namespace NFC_Controller::Cpp;

    DesfireKey<KeyType> desfireKey = DesfireKey<KeyType>(key, 0);

    constexpr size_t blockSize  = DesfireKeyTraits<KeyType>::blockSize;
    constexpr size_t keySize    = DesfireKeyTraits<KeyType>::keySize;

    constexpr size_t rndSize = (KeyType == DesfireKeyType::AES) ? 16 : 8;
    constexpr uint8_t insCode = (KeyType == DesfireKeyType::AES) ? 0xAA : 0x0A;

    std::cout << "\n===============================================\n";
    std::cout << " Authenticate: Key #" << int(keyNo) << " ("
              << DesfireKeyTraits<KeyType>::Name << ", " << keySize << " bytes)\n";
    std::cout << "===============================================\n";

    Log("Executing DESFire Authenticate command via InDataExchange\n");

    selectApplication(0x000000); // Select PICC/master application first

    // Step 1: Send authentication command (0x0A for DES/3DES, 0xAA for AES)
    etl::vector<uint8_t, 7> command = {0x90, insCode, 0x00, 0x00, 0x01, keyNo, 0x00};
    etl::vector<uint8_t, 256> response = {0};

    if (!this->executeCommand(command, response))
    {
        Log("ERROR: authenticate failed to execute command\n");
        return false;
    }

    for (size_t i = 0; i < response.size(); ++i)
    {
        std::cout << Hex0x(response[i]) << " ";
    }
    std::cout << std::dec << std::endl;

    // The card must reply with blockSize encrypted bytes followed by 91 AF
    std::cout << "Checking for " << blockSize + 2 << " bytes (RndB + status)\n";
    if (response.size() < blockSize + 2 ||
        response[response.size() - 2] != 0x91 ||
        response[response.size() - 1] != 0xAF)
    {
        Log("ERROR: unexpected DESFire auth response\n");
        std::cout << "❌ Unexpected response format\n";
        return false;
    }

    // Step 2: Decrypt RndB
    std::array<uint8_t, blockSize>  encryptedRndB = {0};
    std::array<uint8_t, rndSize>    rndB = {0};

    std::copy_n(response.begin(), blockSize, encryptedRndB.begin());

    std::cout << "Encrypted RndB bytes: ";
    for (auto byte : encryptedRndB)
    {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;

    desfireKey.resetIV(); // Reset IV after decryption
    desfireKey.decrypt(std::span<const uint8_t>(encryptedRndB.data(), rndSize), std::span<uint8_t>(rndB.data(), rndSize));

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

    desfireKey.resetIV(); // Reset IV before encryption
    desfireKey.encrypt(std::span<const uint8_t>(hostChallenge.data(), hostChallenge.size()),
                       std::span<uint8_t>(encHostChallenge.data(), encHostChallenge.size()));

    std::cout << "Encrypted host challenge: ";
    for (auto byte : encHostChallenge)
    {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::endl;

    // Step 7: Send encrypted challenge via Additional Frame
    etl::vector<uint8_t, 256> afCmd = {
        0x90,                               // CLA
        0xAF,                               // INS = Additional Frame
        0x00,                               // P1
        0x00,                               // P2
        static_cast<uint8_t>(rndSize * 2)   // Lc
    };
    afCmd.insert(afCmd.end(), encHostChallenge.begin(), encHostChallenge.end());
    afCmd.push_back(0x00);                  // Le

    if(!executeCommand(afCmd, response)){
        Log("ERROR: Additional Frame command failed\n");
        return false;
    }

    std::cout << "Card verification response: ";
    for (auto byte : response)
    {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::dec << std::endl;

    // Card must reply with rndSize encrypted bytes followed by 0x91 0x00 (success)
    if (response.size() < rndSize + 2 ||
        response[response.size() - 2] != 0x91 ||
        response[response.size() - 1] != 0x00)
    {
        Log("ERROR: unexpected card verification response\n");
        std::cout << "❌ Verification failed\n";
        return false;
    }

    // Step 8: Decrypt and verify RndA'
    std::array<uint8_t, rndSize> encRndA_rotated{};
    std::copy_n(response.begin(), rndSize, encRndA_rotated.begin());

    std::array<uint8_t, rndSize> rndA_rotated{};

    desfireKey.resetIV(); // Reset IV before decryption
    desfireKey.decrypt(std::span<const uint8_t>(encRndA_rotated.data(), rndSize),
                       std::span<uint8_t>(rndA_rotated.data(), rndSize));

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
            std::cout << Hex0x(byte) << ", ";
        }
        std::cout << std::endl;

        std::cout << "Stored RndB: ";
        for (auto byte : sessionRndB_)
        {
            std::cout << Hex0x(byte) << ", ";
        }
        std::cout << std::endl;

        sessionKey_.resize(16);
        sessionKey_Versioned.resize(16);

        std::fill(sessionKey_.begin(), sessionKey_.end(), 0);
        std::fill(sessionKey_Versioned.begin(), sessionKey_Versioned.end(), 0);

        DeriveSessionKey(sessionRndA_.data(), sessionRndB_.data(), currentKey_.data(), sessionKey_.data(), sessionKey_Versioned.data());

        std::cout << "Session key: ";
        for (auto byte : sessionKey_)
        {
            std::cout << Hex0x(byte) << ", ";
        }
        std::cout << std::dec << std::endl;

        // Store current key (pad to 24 bytes for uniform storage)
        std::copy(key.begin(), key.end(), currentKey_.begin());
        if constexpr (keySize < 24)
        {
            std::fill(currentKey_.begin() + keySize, currentKey_.end(), 0);
        }
        std::array<uint8_t, DesfireKeyTraits<KeyType>::keySize> sessionKey_Array{};
        std::copy(sessionKey_.begin(), sessionKey_.end(), sessionKey_Array.begin());
        sessionKeyObj.emplace<DesfireKey<KeyType>>(sessionKey_Array, /*version*/0);

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
template bool MifareDesfireCard::authenticate<DesfireKeyType::DES>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::DES>::keySize> &);
template bool MifareDesfireCard::authenticate<DesfireKeyType::DES3_2KEY>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::DES3_2KEY>::keySize> &);
template bool MifareDesfireCard::authenticate<DesfireKeyType::DES3_3KEY>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::DES3_3KEY>::keySize> &);
template bool MifareDesfireCard::authenticate<DesfireKeyType::AES>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::AES>::keySize> &);

template <DesfireKeyType NewKeyType>
bool MifareDesfireCard::changeKey(uint8_t keyNo, const std::array<uint8_t, DesfireKeyTraits<NewKeyType>::keySize> &newKey)
{
    constexpr size_t newKeySize = DesfireKeyTraits<NewKeyType>::keySize;

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
    std::array<uint8_t, 8> iv = {0}; // Initial IV = 0s

    // === STEP 2: CALCULATE CRC32 OF NEW KEY ===
    std::cout << "--- Step 2: Calculate CRC32 of new key ---\n";
    // Calculate crc over [ 0xC4, KeyNo, NewKey ]
    std::vector<uint8_t> crcCryptoBuf{0xC4, keyNo};
    crcCryptoBuf.insert(crcCryptoBuf.end(), newKey.begin(), newKey.end());
    uint32_t crc = 0xFFFFFFFF;
    crc = CalcCrc32(crcCryptoBuf.data(), crcCryptoBuf.size(), crc);
    crc ^= 0xFFFFFFFF;
    std::cout << "Calculated CRC32: 0x" << std::hex << std::setw(8) << std::setfill('0') << crc << std::dec << "\n";

    // uint32_t crckey = 0xFFFFFFFF;
    // crckey = CalcCrc32(newKey.data(), newKey.size(), crckey);
    // crckey ^= 0xFFFFFFFF;
    uint16_t crckey = calculateCRC16(newKey.data(), newKey.size());
    std::cout << "Calculated Key CRC32: 0x" << std::hex << std::setw(8) << std::setfill('0') << crckey << std::dec << "\n";

    // === STEP 3: Create raw Crypto Payload ===
    std::cout << "--- Step 3: Create cryptogram (not encrypted) ---\n";
    // Append New Key, CRC Crypto, CRC NewKey
    std::vector<uint8_t> cryptogram = {};
    for (int i = 0; i < 16; i++)
        cryptogram.push_back(newKey[i]);
    // for (int i = 0; i < 4; i++)
    //     cryptogram.push_back((crc >> (8 * i)) & 0xFF);
    for (int i = 0; i < 2; i++)
        cryptogram.push_back((crckey >> (8 * i)) & 0xFF);
    for (int i = 0; i < 6; i++)
        cryptogram.push_back(0x00); // padding

    std::cout << "Raw cryptogram (before encryption): ";
    for (auto byte : cryptogram)
    {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::dec << std::endl;

    // === STEP 4: ENCRYPT CRYPTOGRAM ===
    std::cout << "--- Step 4: Encrypt cryptogram ---\n";

    // Encrypt cryptogram with session key using the same call as in main.cpp
    // cryptogram contains 24 bytes: NewKey(16) || CRC32(4) || padding(4)
    if (sessionKey_.size() < 16)
    {
        Log("ERROR: session key too small for ChangeKey encryption\n");
        std::cout << "❌ Error: session key not available for encryption\n";
        return false;
    }

    // Encrypt cryptogram (24 bytes) using external cppdes DES3CBC in CBC mode
    // Build 3DES keys (2-key 3DES: K3 = K1)
    uint64_t key1_u64 = 0, key2_u64 = 0, key3_u64 = 0;
    // sessionKey_ holds the derived session key (at least 16 bytes expected)
    for (int i = 0; i < 8; i++)
        key1_u64 = (key1_u64 << 8) | static_cast<uint8_t>(sessionKey_[i]);
    if (sessionKey_.size() >= 16)
    {
        for (int i = 0; i < 8; i++)
            key2_u64 = (key2_u64 << 8) | static_cast<uint8_t>(sessionKey_[8 + i]);
    }
    else
    {
        key2_u64 = key1_u64;
    }
    key3_u64 = key1_u64; // For 2-key 3DES

    // Use DES3 primitive with per-block decrypt chaining to match legacy CryptDataCBC(CBC_SEND, KEY_DECIPHER)
    DES3 des3(key1_u64, key2_u64, key3_u64);

    uint8_t u8_Cryptogram_enc_ex[40] = {0};

    // --- Variant A: seed chaining with sessionEncRndB_ (as implemented) ---
    // ui64 last_block_A = 0;
    // for (int i = 0; i < 8; i++)
    // {
    //     last_block_A = (last_block_A << 8) | static_cast<uint8_t>(sessionEncRndB_[i]);
    // }

    // std::cout << "Using IV (sessionEncRndB_ first 8 bytes) for Variant A: ";
    // for (int i = 0; i < 8; i++)
    // {
    //     std::cout << Hex0x(sessionEncRndB_[i]) << " ";
    // }
    // std::cout << std::endl;

    // uint8_t variantA[24] = {0};
    // for (size_t blockIdx = 0; blockIdx < 3; blockIdx++)
    // {
    //     ui64 plain_u64 = 0;
    //     for (size_t i = 0; i < 8; i++)
    //         plain_u64 = (plain_u64 << 8) | static_cast<uint8_t>(cryptogram[blockIdx * 8 + i]);

    //     std::cout << "VariantA Plain block[" << blockIdx << "]: ";
    //     for (size_t i = 0; i < 8; i++)
    //         std::cout << Hex0x(cryptogram[blockIdx * 8 + i]) << " ";
    //     std::cout << std::endl;

    //     ui64 xored = plain_u64 ^ last_block_A;
    //     std::cout << "VariantA XOR (plain ^ last_block): 0x" << std::hex << xored << std::dec << std::endl;

    //     ui64 out_u64 = des3.decrypt(xored);

    //     ui64 tmp = out_u64;
    //     for (int i = 7; i >= 0; i--)
    //     {
    //         variantA[blockIdx * 8 + i] = static_cast<uint8_t>(tmp & 0xFF);
    //         tmp >>= 8;
    //     }

    //     std::cout << "VariantA Cipher block[" << blockIdx << "]: ";
    //     for (size_t i = 0; i < 8; i++)
    //         std::cout << Hex0x(variantA[blockIdx * 8 + i]) << " ";
    //     std::cout << std::endl;

    //     last_block_A = out_u64;
    // }

    // --- Variant B: seed chaining with ZERO IV ---
    ui64 last_block_B = 0x0000000000000000ULL;
    std::cout << "Using IV (zero) for Variant B" << std::endl;

    uint8_t variantB[24] = {0};
    for (size_t blockIdx = 0; blockIdx < 3; blockIdx++)
    {
        ui64 plain_u64 = 0;
        for (size_t i = 0; i < 8; i++)
            plain_u64 = (plain_u64 << 8) | static_cast<uint8_t>(cryptogram[blockIdx * 8 + i]);

        std::cout << "VariantB Plain block[" << blockIdx << "]: ";
        for (size_t i = 0; i < 8; i++)
            std::cout << Hex0x(cryptogram[blockIdx * 8 + i]) << " ";
        std::cout << std::endl;

        ui64 xored = plain_u64 ^ last_block_B;
        std::cout << "VariantB XOR (plain ^ last_block): 0x" << std::hex << xored << std::dec << std::endl;

        ui64 out_u64 = des3.decrypt(xored);

        ui64 tmp = out_u64;
        for (int i = 7; i >= 0; i--)
        {
            variantB[blockIdx * 8 + i] = static_cast<uint8_t>(tmp & 0xFF);
            tmp >>= 8;
        }

        std::cout << "VariantB Cipher block[" << blockIdx << "]: ";
        for (size_t i = 0; i < 8; i++)
            std::cout << Hex0x(variantB[blockIdx * 8 + i]) << " ";
        std::cout << std::endl;

        last_block_B = out_u64;
    }

    uint8_t cryptogram_own[24] = {0};
    auto & key = std::get<DesfireKey<NewKeyType>>(sessionKeyObj);
    key.resetIV();
    key.decrypt(std::span<const uint8_t>(cryptogram.data(), cryptogram.size()),
                std::span<uint8_t>(cryptogram_own, 24),
                true); // true = DECRYPT for ChangeKey

    // Print summaries
    // std::cout << "* CryptogrEnc VariantA (encRndB IV): ";
    // for (int i = 0; i < 24; i++)
    //     std::cout << Hex0x(variantA[i]) << " ";
    // std::cout << std::endl;

    std::cout << "* CryptogrEnc VariantB (zero IV): ";
    for (int i = 0; i < 24; i++)
        std::cout << Hex0x(variantB[i]) << " ";
    std::cout << std::endl;

    std::cout << "* CryptogrEnc OWN VariantB (zero IV): ";
    for (int i = 0; i < 24; i++)
        std::cout << Hex0x(cryptogram_own[i]) << " ";
    std::cout << std::endl;

    // Choose VariantB (zero IV) for sending — this is easier to test; if it doesn't match, we can switch.
    std::memcpy(u8_Cryptogram_enc_ex, variantB, 24);

    std::cout << "* CryptogrEnc: ";
    for (size_t i = 0; i < 24; i++)
    {
        std::cout << Hex0x(u8_Cryptogram_enc_ex[i]) << " ";
    }
    std::cout << std::endl;

    // Replace clear cryptogram bytes with the encrypted bytes for following steps
    cryptogram.clear();
    cryptogram.insert(cryptogram.end(), u8_Cryptogram_enc_ex, u8_Cryptogram_enc_ex + 24);

    // === STEP 5: SEND ChangeKey COMMAND ===
    std::cout << "--- Step 5: Send ChangeKey command ---\n";
    // Build ChangeKey command

    std::vector<uint8_t> cmd;
    cmd.push_back(0x90); // CLA
    cmd.push_back(0xC4); // INS = ChangeKey
    cmd.push_back(0x00); // P1
    cmd.push_back(0x00); // P2
    // Per DESFire ChangeKey APDU format the data field is: <KeyNo> || <encrypted-cryptogram(24)>
    // So Lc = 1 + cryptogram.size()
    cmd.push_back(static_cast<uint8_t>(cryptogram.size() + 1)); // Lc
    cmd.push_back(keyNo);                                       // first data byte = KeyNo
    cmd.insert(cmd.end(), cryptogram.begin(), cryptogram.end());
    cmd.push_back(0x00); // Le

    std::cout << "ChangeKey APDU: ";
    for (size_t i = 0; i < cmd.size(); i++)
    {
        std::cout << Hex0x(cmd[i]) << " ";
    }
    std::cout << std::dec << std::endl;

    using namespace NFC_Controller::Cpp;
    auto command = InDataExchangeCommand(
        InDataExchangeCommand::Options{
            .payload = cmd,
            .responseTimeoutMs = 2000});

    auto result = nfc_->executeCommand(command);
    if (result.status != pn532Response::statusCode::OK)
    {
        Log("ERROR: ChangeKey failed with status: " + std::to_string(static_cast<int>(result.status)) + "\n");
        std::cout << "❌ ChangeKey command failed\n";
        return false;
    }

    std::cout << "ChangeKey response: ";
    for (auto byte : result.responsePayload)
    {
        std::cout << Hex0x(byte) << " ";
    }
    std::cout << std::dec << std::endl;

    // Check for success status 0x91 0x00
    if (result.responsePayload.size() < 2 ||
        result.responsePayload[result.responsePayload.size() - 2] != 0x91 ||
        result.responsePayload[result.responsePayload.size() - 1] != 0x00)
    {
        Log("ERROR: ChangeKey command returned failure status\n");
        std::cout << "❌ ChangeKey failed (card returned error)\n";
        return false;
    }

    std::cout << "✅ ChangeKey successful!\n";
    return true;
}

// Explicit template instantiations for supported key types
template bool MifareDesfireCard::changeKey<DesfireKeyType::DES>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::DES>::keySize> &);
template bool MifareDesfireCard::changeKey<DesfireKeyType::DES3_2KEY>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::DES3_2KEY>::keySize> &);
template bool MifareDesfireCard::changeKey<DesfireKeyType::DES3_3KEY>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::DES3_3KEY>::keySize> &);
template bool MifareDesfireCard::changeKey<DesfireKeyType::AES>(uint8_t, const std::array<uint8_t, DesfireKeyTraits<DesfireKeyType::AES>::keySize> &);

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