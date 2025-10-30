#pragma once

#include <array>
#include <map>
#include <string>
#include <iostream>
#include <span>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <algorithm>

#include "cppdes/descbc.h"
#include "cppdes/des3cbc.h"

#include "../Headers/Cards/KeyTraits.h"


template <typename T>
constexpr T from_big_endian(std::span<const std::uint8_t> bytes) noexcept
{
    static_assert(std::is_unsigned_v<T>, "from_big_endian requires an unsigned integer type T");
    T v = 0;
    for (std::size_t i = 0; i < bytes.size() && i < sizeof(T); ++i)
        v = (v << 8) | static_cast<T>(bytes[i]);
    return v;
}

template <typename T>
constexpr void to_big_endian(T value, std::span<std::uint8_t> out) noexcept
{
    static_assert(std::is_unsigned_v<T>, "to_big_endian requires an unsigned integer type T");
    // Use a macro-safe call and ensure both args are size_t to avoid
    // MSVC/Windows 'min' macro collisions and type issues.
    std::size_t needed = (std::min)(out.size(), static_cast<std::size_t>(sizeof(T)));
    for (std::size_t i = 0; i < needed; ++i)
    {
        out[needed - 1 - i] = static_cast<std::uint8_t>(value & 0xFFu);
        value >>= 8;
    }
    for (std::size_t i = needed; i < out.size(); ++i)
        out[i] = 0;
}

using namespace desfire;

template <DesfireKeyType T>
class DesfireKey
{
public:
    DesfireKey(const std::array<uint8_t, DesfireKeyTraits<T>::keySize> &keyData, uint8_t version)
        : blockSize(DesfireKeyTraits<T>::blockSize),
          keyVersion(version),
          keyType(T)
    {
        key = keyData;
        iv.fill(0);
        Cmac1.fill(0);
        Cmac2.fill(0);

        std::cout << "block size: " << blockSize << "\n";
        std::cout << "key version: " << static_cast<int>(keyVersion) << "\n";
        std::cout << "key type: " << DesfireKeyTraits<T>::Name << "\n";
        std::cout << "cmac1 size: " << Cmac1.size() << "\n";
        std::cout << "cmac2 size: " << Cmac2.size() << "\n";

        std::cout << "DesfireKey constructor: Key of type " << DesfireKeyTraits<T>::Name << " initialized.\n";

        this->GenerateCmacSubkeys();
        std::cout << "Generated CMAC subkeys:\n";
        std::cout << "K1: ";
        for (auto byte : Cmac1)
        {
            std::cout << std::hex << std::uppercase << static_cast<int>(byte) << " ";
        }
        std::cout << "\nK2: ";
        for (auto byte : Cmac2)
        {
            std::cout << std::hex << std::uppercase << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl;
    }

    bool encrypt(std::span<const uint8_t> in, std::span<uint8_t> out, bool xorPlain = false)
    {
        std::cout << "DesfireKey::encrypt called\n";
        return crypt(in, out, true, xorPlain);
    }

    bool decrypt(std::span<const uint8_t> in, std::span<uint8_t> out, bool xorPlain = false)
    {
        std::cout << "DesfireKey::decrypt called\n";
        return crypt(in, out, false, xorPlain);
    }

    // Added `xorPlain` parameter: when true, apply XOR-with-last-block seed chaining
    // between reading the block and processing it with the underlying DES3CBC primitive.
    // Default (false) preserves existing behavior.
    bool crypt(std::span<const uint8_t> in, std::span<uint8_t> out, bool encrypting, bool xorPlain = false)
    {
        if (in.size() != out.size() || in.size() % blockSize != 0)
        {
            std::cerr << "Invalid input or output size for encryption\n";
            return false;
        }
        if constexpr (T == DesfireKeyType::DES)
        {
            processDesBlock(in, out, encrypting);
        }
        else if constexpr (T == DesfireKeyType::DES3_2KEY || T == DesfireKeyType::DES3_3KEY)
        {
            process3desBlock(in, out, encrypting, xorPlain);
        }
        else
        {
            // Implement AES or 3DES encryption as needed
            std::cerr << "Encryption for this key type not implemented yet\n";
            return false;
        }
        return true;
    }

    bool GenerateCmacSubkeys()
    {
        std::cout << "Generating CMAC subkeys for key type: " << DesfireKeyTraits<T>::Name << "\n";
        constexpr size_t BS = DesfireKeyTraits<T>::blockSize;

        // Defensive compile-time/runtime guard: a block size of 0 will cause
        // out-of-bounds accesses (BS - 1) below and undefined behaviour.
        if constexpr (BS == 0)
        {
            std::cerr << "Invalid block size (0) for CMAC subkey generation\n";
            return false;
        }

        uint8_t CmacR = (BS == 8) ? 0x1B : 0x87;
        
        std::array<uint8_t, BS> data = {0};
        std::array<uint8_t, BS> encrypted = {0};

        iv.fill(0);
        if (!encrypt(std::span<const uint8_t>(data.data(), BS), std::span<uint8_t>(encrypted.data(), BS))){
            std::cerr << "Failed to encrypt zero block for CMAC subkey generation\n";
            return false;
        };

        std::cout << "Encrypted zero block for CMAC subkey generation: ";
        for (auto byte : encrypted)
        {
            std::cout << std::hex << std::uppercase << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl;

        auto Cmac1Res = lshift1(encrypted);
        if (msb_set(encrypted)) Cmac1Res[BS - 1] ^= CmacR;

        auto Cmac2Res = lshift1(Cmac1Res);
        if (msb_set(Cmac1Res)) Cmac2Res[BS - 1] ^= CmacR;

        Cmac1 = Cmac1Res;
        Cmac2 = Cmac2Res;

        iv.fill(0); // Reset IV after operation
        return true;
    }

    std::span<const uint8_t> getKey() const
    {
        return std::span<const uint8_t>(key.data(), key.size());
    }

    std::span<const uint8_t> getIV() const
    {
        return std::span<const uint8_t>(iv.data(), iv.size());
    }

    void resetIV()
    {
        iv.fill(0);
    }

    std::pair<std::array<uint8_t, DesfireKeyTraits<T>::blockSize>, std::array<uint8_t, DesfireKeyTraits<T>::blockSize>> getCmacSubkeys() const
    {
        return {Cmac1, Cmac2};
    }

    std::string getKeyTypeAsString()
    {
        return std::string(DesfireKeyTraits<T>::Name);
    }

    void printKey()
    {
        std::cout << "Key: ";
        for (auto byte : key)
        {
            std::cout << std::hex << std::uppercase << static_cast<int>(byte) << " ";
        }
        std::cout << "(Type: " << getKeyTypeAsString() << ", Version: " << std::dec << static_cast<int>(keyVersion) << ")\n";
    }

private:
    // Left-shift array of bytes by 1 bit (big-endian bit order across bytes).
    // For CMAC subkey generation: each byte is shifted left and the carry
    // comes from the next byte's MSB.
    template <std::size_t N>
    static std::array<uint8_t, N> lshift1(const std::array<uint8_t, N> &in) noexcept
    {
        std::array<uint8_t, N> out{};
        for (std::size_t i = 0; i < N; ++i)
        {
            uint8_t next = (i + 1 < N) ? in[i + 1] : 0;
            out[i] = static_cast<uint8_t>((in[i] << 1) | (next >> 7));
        }
        return out;
    }

    // Return true if the most significant bit of the array (byte 0, bit 7) is set.
    template <std::size_t N>
    static bool msb_set(const std::array<uint8_t, N> &in) noexcept
    {
        return (in[0] & 0x80u) != 0;
    }
    void processDesBlock(std::span<const uint8_t> inBlock, std::span<uint8_t> outBlock, bool encrypting)
    {
        // Implement DES block processing here
        std::cout << "Processing DES block (" << (encrypting ? "Encrypting" : "Decrypting") << ")\n";

        constexpr size_t BS = DesfireKeyTraits<T>::blockSize;

        auto key64 = from_big_endian<uint64_t>({key.data(), BS});
        auto iv64 = from_big_endian<uint64_t>({iv.data(), BS});

        DESCBC desCbc(key64, iv64); // Placeholder key and IV

        // Convert every 8 bytes to ui64 and process
        for (size_t offset = 0; offset < inBlock.size(); offset += BS)
        {
            auto inChunk = std::span<const uint8_t>(inBlock.data() + offset, BS);
            uint64_t block = from_big_endian<uint64_t>(inChunk);

            uint64_t processed = encrypting ? desCbc.encrypt(block) : desCbc.decrypt(block);

            auto outChunk = std::span<uint8_t>(outBlock.data() + offset, BS);
            to_big_endian<uint64_t>(processed, outChunk);

            std::memcpy(iv.data(), outChunk.data(), iv.size());
        }
    }

    void process3desBlock(std::span<const uint8_t> inBlock, std::span<uint8_t> outBlock, bool encrypting, bool xorPlain = false)
    {
        // Implement 3DES block processing here
        std::cout << "Processing 3DES block (" << (encrypting ? "Encrypting" : "Decrypting") << ")\n";
        // Similar to processDesBlock but using DES3CBC

        constexpr size_t BS = DesfireKeyTraits<T>::blockSize;

        auto key1_u64 = from_big_endian<uint64_t>({key.data(), 8});
        auto key2_u64 = from_big_endian<uint64_t>({key.data() + 8, 8});
        auto key3_u64 = T == DesfireKeyType::DES3_3KEY
                            ? from_big_endian<uint64_t>({key.data() + 16, 8})
                            : key1_u64; // For 2-key 3DES, K3 = K1

        auto iv64 = from_big_endian<uint64_t>({iv.data(), BS});

        std::cout << "3DES Keys: K1=0x" << std::hex << key1_u64 << " K2=0x" << key2_u64 << " K3=0x" << key3_u64 << std::dec << std::endl;
        std::cout << "3DES IV: 0x" << std::hex << iv64 << std::dec << std::endl;

        DES3CBC des3cbc(key1_u64, key2_u64, key3_u64, iv64);

        // Optional XOR-with-last-block seed chaining (minimal change requested).
        // When xorPlain==true we XOR the incoming (plaintext) block with the
        // last processed block (seed), then feed that to the DES3CBC primitive.
        // last_block seeds with zero as in the original snippet.
        uint64_t last_block = 0ULL;

        // Convert every 8 bytes to ui64 and process
        for (size_t offset = 0; offset < inBlock.size(); offset += BS)
        {
            auto inChunk = std::span<const uint8_t>(inBlock.data() + offset, BS);
            uint64_t block = from_big_endian<uint64_t>(inChunk);

            uint64_t toProcess = block;
            if (xorPlain)
            {
                toProcess ^= last_block;
            }

            uint64_t processed = encrypting ? des3cbc.encrypt(toProcess) : des3cbc.decrypt(toProcess);

            auto outChunk = std::span<uint8_t>(outBlock.data() + offset, BS);
            to_big_endian<uint64_t>(processed, outChunk);

            if (xorPlain)
            {
                des3cbc.reset();
                last_block = processed;
            }
        }

        // debug output
        std::cout << "Processed 3DES block output: ";
        for (size_t i = 0; i < outBlock.size(); ++i)
        {
            std::cout << std::hex << std::uppercase << static_cast<int>(outBlock[i]) << " ";
        }
        std::cout << std::dec << std::endl;
    }

    std::array<uint8_t, DesfireKeyTraits<T>::keySize>   key   = {0}; // The key to authenticate with
    std::array<uint8_t, DesfireKeyTraits<T>::blockSize> iv    = {0}; // Initialization Vector for CBC mode
    std::array<uint8_t, DesfireKeyTraits<T>::blockSize> Cmac1 = {0}; // CMAC subkey 1
    std::array<uint8_t, DesfireKeyTraits<T>::blockSize> Cmac2 = {0}; // CMAC subkey 2

    uint8_t blockSize;      // Block size in bytes (8 for DES, 16 for AES)
    uint8_t keyVersion;     // Key version
    DesfireKeyType keyType; // Type of the key (DES, TDES, AES)
};
