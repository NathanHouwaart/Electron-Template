#include <cstring>
#include "../Headers/desfire.h"
#include <iostream>
#include <iomanip>
#include "../Headers/hex.h"

using namespace NFC_Controller::Cpp;

namespace Desfire
{

    std::ostream &operator<<(std::ostream &os, const DesfireVersion &v)
    {
        os << "=== DESFire Version Info ===\n";
        os << "Hardware Information:\n";
        os << "HW: vendor= " << hex0x(v.hardwareInfo.hwVendorID) << std::endl;
        os << "HW: type= " << hex0x(v.hardwareInfo.hwType) << 
           " - HW: subType= " << hex0x(v.hardwareInfo.hwSubType) << std::endl;
        os << "HW: version= " << hex0x(v.hardwareInfo.hwMajorVersion) << 
           " - HW: minorVersion= " << hex0x(v.hardwareInfo.hwMinorVersion) << std::endl;
        os << "HW: storage= " << hex0x(v.hardwareInfo.hwStorageSize) << std::endl;
        os << "HW: protocol= " << hex0x(v.hardwareInfo.hwProtocolType) << std::endl;

        os << std::endl;

        os << "Software Information:\n";
        os << "SW: vendor= " << hex0x(v.softwareInfo.swVendorID) << std::endl;
        os << "SW: type= " << hex0x(v.softwareInfo.swType) << 
           " - SW: subType= " << hex0x(v.softwareInfo.swSubType) << std::endl;
        os << "SW: version= " << hex0x(v.softwareInfo.swMajorVersion) << 
           " - SW: minorVersion= " << hex0x(v.softwareInfo.swMinorVersion) << std::endl;
        os << "SW: storage= " << hex0x(v.softwareInfo.swStorageSize) << std::endl;
        os << "SW: protocol= " << hex0x(v.softwareInfo.swProtocolType) << std::endl;
        
        os << std::endl;

        os << "Manufacturing Information:\n";
        os << "UID: ";
        for (auto b : v.manufacturingInfo.uid)
            os << hex0x(b) << ' ';
        os << "\n";

        os << "Batch Number: ";
        for (auto b : v.manufacturingInfo.batchNumber)
            os << hex0x(b) << ' ';
        os << "\n";
        os << "Production Week: " << hex0x(v.manufacturingInfo.productionWeek) << std::endl;
        os << "Production Year: " << hex0x(v.manufacturingInfo.productionYear) << std::endl;

        return os;
    }

    bool desfireParseGetVersionFrame(DesfireVersion &versionStruct, const uint8_t *data, uint8_t frameIndex)
    {
        // Check for null pointer
        if (!data)
        {
            std::cout << "Data pointer is null." << std::endl;
            return false;
        }

        // Extract frame length
        uint8_t frameLength = data[0];

        switch (frameIndex)
        {
        case 0: // Hardware info
            if (frameLength < sizeof(versionStruct.hardwareInfo))
            {
                std::cout << "Not enough data for hardware info." << std::endl;
                return false; // Not enough data for hardware info
            }
            memcpy(&versionStruct.hardwareInfo, &data[5], sizeof(versionStruct.hardwareInfo));
            break;

        case 1: // Software info
            if (frameLength < sizeof(versionStruct.softwareInfo))
            {
                return false; // Not enough data for software info
            }
            memcpy(&versionStruct.softwareInfo, &data[5], sizeof(versionStruct.softwareInfo));
            break;

        case 2: // Manufacturing info
            if (frameLength < sizeof(versionStruct.manufacturingInfo))
            {
                return false; // Not enough data for manufacturing info
            }
            memcpy(&versionStruct.manufacturingInfo, &data[5], sizeof(versionStruct.manufacturingInfo));
            break;

        default:
            return false;
        }

        return true;
    }


    DesfireCard::DesfireCard(
        NFC_Controller::Cpp::NFC & nfc
    ) : 
        _nfc(nfc)
    {

    }

    // Implement other member functions here
    statusCode DesfireCard::getVersion()
    {
        struct DesfireVersion versionStruct;

        uint8_t command [] = {
            0x90, 0x60, 0x00, 0x00, 0x00
        };

        uint8_t finalBuffer[255] = {0};
        uint8_t finalBufferSize = 0;

        uint8_t frameIndex = 0;
        
        uint8_t responseSize = 0;
        uint8_t responseBuffer[255] = {0};

        do {
            // Reset response buffer
            responseSize = 0;
            memset(responseBuffer, 0, sizeof(responseBuffer));

            // Send command and get response
            auto result = _nfc.initDataExchange(command, sizeof(command), responseBuffer, responseSize);
            if (result != statusCode::pn532StatusOK)
            {
                return result;
            }
            
            std::cout << "Response frame " << int(frameIndex) << ": ";
            for (uint8_t i = 0; i < responseSize; i++)
            {
                std::cout << "0x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(responseBuffer[i]) << " ";
            }
            std::cout << std::endl;

            // Parse response frame
            if (!desfireParseGetVersionFrame(versionStruct, responseBuffer, frameIndex))
            {
                std::cout << "Failed to parse response frame " << int(frameIndex) << std::endl;
                return statusCode::pn532StatusWrongCommand;
            }

            if (frameIndex == 0)
            {
                command[1] = 0xAF; // Change INS byte to 0xAF for subsequent frames
            }
            frameIndex++;

        } while (responseSize > 0 && responseBuffer[responseSize - 4] == 0x91 && responseBuffer[responseSize - 3] == 0xAF);

        std::cout << versionStruct << std::endl;

        return statusCode::pn532StatusOK;
    }

} // namespace Desfire
