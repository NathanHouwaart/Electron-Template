#pragma once

#include "KeyVersion.h"

namespace desfire
{
    
// Template for key size based on type
template<DesfireKeyType KeyType>
struct DesfireKeyTraits;

template<>
struct DesfireKeyTraits<DesfireKeyType::DES> {
    static constexpr size_t KeySize = 8;
    static constexpr const char* Name = "DES";
};

template<>
struct DesfireKeyTraits<DesfireKeyType::DES3_2KEY> {
    static constexpr size_t KeySize = 16;
    static constexpr const char* Name = "2-Key 3DES";
};

template<>
struct DesfireKeyTraits<DesfireKeyType::DES3_3KEY> {
    static constexpr size_t KeySize = 24;
    static constexpr const char* Name = "3-Key 3DES";
};

template<>
struct DesfireKeyTraits<DesfireKeyType::AES> {
    static constexpr size_t KeySize = 16;
    static constexpr const char* Name = "AES-128";
};

    
} // namespace desfire