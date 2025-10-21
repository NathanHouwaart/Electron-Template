#pragma once

#include "../card.h"
#include "../declarations.h"
#include <cstdint>
#include <vector>
#include <cstring>
#include <iostream>

// Forward declaration
namespace NFC_Controller {
    namespace Cpp {
        class NFC;
    }
}

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
    virtual std::string prettyType() const override {
        return "MIFARE DESFire";
    }

    MifareDesfireCard(TargetInfo id, NFC_Controller::Cpp::NFC* nfc = nullptr)
        : Card(std::move(id)), nfc_(nfc), versionRetrieved_(false) {
        std::memset(&versionInfo_, 0, sizeof(versionInfo_));
    }

    virtual void selectApplication(uint32_t aid) {}
    virtual void readData(uint8_t fileNo, std::vector<uint8_t>& outData) {}
    virtual void writeData(uint8_t fileNo, const std::vector<uint8_t>& data) {}
    
    // Get version information from the card via InDataExchange
    virtual bool getVersion(DesfireVersionInfo& versionInfo);
    
    // Get the DESFire variant type (0 = unknown, 1 = EV1, 2 = EV2, 3 = EV3)
    virtual uint8_t getDesfireVariant();

protected:
    NFC_Controller::Cpp::NFC* nfc_;
    DesfireVersionInfo versionInfo_;
    bool versionRetrieved_;
    
    // Helper to parse version response frames
    bool parseVersionFrame(const uint8_t* data, uint8_t responseSize, uint8_t frameIndex);
};

