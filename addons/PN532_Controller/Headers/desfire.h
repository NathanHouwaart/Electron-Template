#pragma once

#include "pn532.h"
#include "declarations.h"
#include <iostream>
#include <iomanip>

using namespace NFC_Controller::Cpp;

namespace Desfire
{

// Put the pack pragmas around the struct for MSVC, and also keep the GCC attribute for
// compilers that support it. This combination is clear and portable.
#if defined(_MSC_VER)
#pragma pack(push, 1)
#endif

    struct DesfireVersion
    {
        struct HardwareInfo
        {
            uint8_t hwVendorID;
            uint8_t hwType;
            uint8_t hwSubType;
            uint8_t hwMajorVersion;
            uint8_t hwMinorVersion;
            uint8_t hwStorageSize;
            uint8_t hwProtocolType;
        } hardwareInfo;

        struct SoftwareInfo
        {
            uint8_t swVendorID;
            uint8_t swType;
            uint8_t swSubType;
            uint8_t swMajorVersion;
            uint8_t swMinorVersion;
            uint8_t swStorageSize;
            uint8_t swProtocolType;
        } softwareInfo;

        struct ManufacturingInfo
        {
            uint8_t uid[7];
            uint8_t batchNumber[5];
            uint8_t productionWeek;
            uint8_t productionYear;
        } manufacturingInfo;

    }
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((packed))
#endif
    ;

#if defined(_MSC_VER)
#pragma pack(pop)
#endif

    static constexpr auto x = sizeof(DesfireVersion);
    static_assert(sizeof(DesfireVersion) == 28, "unexpected packing for DesfireVersion");

#include <iostream>
#include <iomanip>

    std::ostream &operator<<(std::ostream &os, const DesfireVersion &v);

    bool desfireParseGetVersionFrame(DesfireVersion &versionStruct, const uint8_t *data, uint8_t frameIndex);

    class DesfireCard
    {

    public:
        DesfireCard(NFC_Controller::Cpp::NFC &nfc);
        statusCode authenticateAES(const uint8_t *key, const uint8_t keyLength);
        statusCode getVersion();
        statusCode getCardUID(uint8_t *uid, uint8_t &uidLength);
        statusCode selectApplication(uint8_t appID);
        statusCode createApplication(uint8_t appID, uint8_t fileCount);
        statusCode deleteApplication(uint8_t appID);
        statusCode formatPICC();
        statusCode getApplicationIDs(uint8_t *appIDs, uint8_t &appIDCount);
        statusCode getFileIDs(uint8_t *fileIDs, uint8_t &fileIDCount);
        statusCode createStdDataFile(uint8_t fileID, uint16_t fileSize);
        statusCode writeData(uint8_t fileID, const uint8_t *data, uint16_t dataSize);
        statusCode readData(uint8_t fileID, uint8_t *data, uint16_t &dataSize);

    private:
        NFC_Controller::Cpp::NFC &_nfc;
    };
} // namespace Desfire
