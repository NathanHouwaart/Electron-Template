#pragma once

#include "../card.h"
#include <cstdint>
#include <vector>
#include <cstring>
#include <iostream>

// Forward declaration
namespace NFC_Controller {
    namespace Cpp {
        class PN532_chip;
    }
}

enum class DesfireCommand : uint8_t {
    // Authentication
    AUTHENTICATE = 0x0A,
    AUTHENTICATE_ISO = 0x1A,
    AUTHENTICATE_AES = 0xAA,
    AUTHENTICATE_EV2_FIRST = 0x71,
    AUTHENTICATE_EV2_NONFIRST = 0x77,
    
    // PICC Management
    GET_VERSION = 0x60,
    GET_CARD_UID = 0x51,
    FORMAT_PICC = 0xFC,
    
    // Application Management
    CREATE_APPLICATION = 0xCA,
    DELETE_APPLICATION = 0xDA,
    SELECT_APPLICATION = 0x5A,
    GET_APPLICATION_IDS = 0x6A,
    
    // File Management
    CREATE_STD_DATA_FILE = 0xCD,
    DELETE_FILE = 0xDF,
    GET_FILE_IDS = 0x6F,
    
    // Data Operations
    READ_DATA = 0xBD,
    WRITE_DATA = 0x3D,
    GET_VALUE = 0x6C,
    
    // Transaction
    COMMIT_TRANSACTION = 0xC7,
    ABORT_TRANSACTION = 0xA7,
    
    // Utility
    ADDITIONAL_FRAME = 0xAF
};

// Use packed structures for proper memory layout matching DESFire response format
#if defined(_MSC_VER)
#pragma pack(push, 1)
#endif

// DESFire version structure - matches the card's GetVersion response format
struct DesfireVersionInfo
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

// Verify the packed structure size matches expected DESFire response (7+7+14 = 28 bytes)
static_assert(sizeof(DesfireVersionInfo) == 28, "DesfireVersionInfo must be 28 bytes (packed)");

// Helper to output DESFire version info
std::ostream& operator<<(std::ostream& os, const DesfireVersionInfo& v);

class MifareDesfireCard : public Card {
public:
    MifareDesfireCard(TargetInfo id, NFC_Controller::Cpp::PN532_chip* nfc = nullptr)
        : Card(std::move(id)), nfc_(nfc), versionRetrieved_(false) {
        std::memset(&versionInfo_, 0, sizeof(versionInfo_));
    }

    virtual std::string prettyType() const override {
        return "MIFARE DESFire";
    }

    virtual void selectApplication(uint32_t aid);
    virtual void readData(uint8_t fileNo, std::vector<uint8_t>& outData) {}
    virtual void writeData(uint8_t fileNo, const std::vector<uint8_t>& data) {}

    virtual void authenticate();
    virtual void authenticateAES(uint8_t keyNo, const std::array<uint8_t, 16>& RndB);
    
    // Get version information from the card via InDataExchange
    virtual bool getVersion(DesfireVersionInfo& versionInfo);
    
    // Get the DESFire variant type (0 = unknown, 1 = EV1, 2 = EV2, 3 = EV3)
    virtual uint8_t getDesfireVariant();

protected:
    NFC_Controller::Cpp::PN532_chip* nfc_;
    DesfireVersionInfo versionInfo_;
    bool versionRetrieved_;
    
    // Helper to parse version response frames
    bool parseVersionFrame(const uint8_t* data, uint8_t responseSize, uint8_t frameIndex);
};

