#pragma once

#include "KeyVersion.h"

namespace desfire
{
    
template<DesfireKeyType T>
struct DesfireKeyTraits;

template<>
struct DesfireKeyTraits<DesfireKeyType::DES>
{
    static constexpr size_t keySize = 8;
    static constexpr const char* Name = "DES";
    static constexpr size_t blockSize = 8;
};

template<>
struct DesfireKeyTraits<DesfireKeyType::DES3_2KEY>
{
    static constexpr size_t keySize = 16;
    static constexpr const char* Name = "2K3DES";
    static constexpr size_t blockSize = 8;
};

template<>
struct DesfireKeyTraits<DesfireKeyType::DES3_3KEY>
{
    static constexpr size_t keySize = 24;
    static constexpr const char* Name = "3K3DES";
    static constexpr size_t blockSize = 8;
};

template<>
struct DesfireKeyTraits<DesfireKeyType::AES>
{
    static constexpr size_t keySize = 16;
    static constexpr const char* Name = "AES";
    static constexpr size_t blockSize = 16;
};

    
} // namespace desfire