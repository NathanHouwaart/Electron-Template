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
    } else {
        std::cout << "❌ AUTHENTICATION FAILED! RndA mismatch!" << std::endl;
        Log("ERROR: Authentication failed - RndA verification mismatch\n");
        return;
    }

    return;
}

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