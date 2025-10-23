#pragma once

#include <cstdint>
#include <string>

namespace desfire
{

    enum class DesfireKeyType : uint8_t
    {
        DES = 0x0,
        DES3_2KEY = 0x1,
        DES3_3KEY = 0x2,
        AES = 0x3,
        UNKNOWN = 0xF
    };

    inline uint8_t makeKeyVersion(DesfireKeyType algo, uint8_t rev)
    {
        return (static_cast<uint8_t>(algo) & 0x0F) << 4 | (rev & 0x0F);
    }

    inline DesfireKeyType keyVersionGetAlgo(uint8_t keyVersion)
    {
        uint8_t hi = (keyVersion >> 4) & 0x0F;
        switch (hi)
        {
        case 0x0:
            return DesfireKeyType::DES;
        case 0x1:
            return DesfireKeyType::DES3_2KEY;
        case 0x2:
            return DesfireKeyType::DES3_3KEY;
        case 0x3:
            return DesfireKeyType::AES;
        default:
            return DesfireKeyType::UNKNOWN;
        }
    }

    inline uint8_t keyVersionGetRevision(uint8_t keyVersion)
    {
        return keyVersion & 0x0F;
    }

    inline std::string keyAlgoToString(DesfireKeyType a)
    {
        switch (a)
        {
        case DesfireKeyType::DES:
            return "DES";
        case DesfireKeyType::DES3_2KEY:
            return "3DES(2K)";
        case DesfireKeyType::DES3_3KEY:
            return "3DES(3K)";
        case DesfireKeyType::AES:
            return "AES";
        default:
            return "UNKNOWN";
        }
    }

} // namespace desfire