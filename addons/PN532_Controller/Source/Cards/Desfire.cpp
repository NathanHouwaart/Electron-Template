#include "../../Headers/Cards/Desfire.h"
#include "../../Headers/nfc.h"
#include "../../../AddonLog.h"
#include <iostream>
#include <iomanip>

// Helper to output DESFire version info (inspired by desfire.cpp)
std::ostream& operator<<(std::ostream& os, const DesfireVersionInfo& v)
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

bool MifareDesfireCard::parseVersionFrame(const uint8_t* data, uint8_t responseSize, uint8_t frameIndex)
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

bool MifareDesfireCard::getVersion(DesfireVersionInfo& versionInfo)
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
        0x90, 0x60, 0x00, 0x00, 0x00
    };

    uint8_t frameIndex = 0;
    uint8_t responseSize = 0;
    uint8_t responseBuffer[255] = {0};

    // DESFire GetVersion returns data in 3 frames (hardware, software, manufacturing)
    // Each response ends with status bytes (0x91 0xAF for "more data" or 0x91 0x00 for "done")
    do {
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
