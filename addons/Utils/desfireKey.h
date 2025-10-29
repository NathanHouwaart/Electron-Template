#include <array>
#include <map>
#include <string>
#include <iostream>
#include <span>
#include <cstddef>

#include "cppdes/descbc.h"
#include "cppdes/des3cbc.h"

template <std::unsigned_integral T>
constexpr T from_big_endian(std::span<const std::uint8_t> bytes) noexcept
{
    T v = 0;
    for (std::size_t i = 0; i < bytes.size() && i < sizeof(T); ++i)
        v = (v << 8) | static_cast<T>(bytes[i]);
    return v;
}

template <std::unsigned_integral T>
constexpr void to_big_endian(T value, std::span<std::uint8_t> out) noexcept
{
    std::size_t needed = std::min(out.size(), sizeof(T));
    for (std::size_t i = 0; i < needed; ++i)
    {
        out[needed - 1 - i] = static_cast<std::uint8_t>(value & 0xFFu);
        value >>= 8;
    }
    for (std::size_t i = needed; i < out.size(); ++i)
        out[i] = 0;
}

enum class DesfireKeyType
{
    INVALID,
    DES,
    DES3_2KEY,
    DES3_3KEY,
    AES
};

template <DesfireKeyType T>
struct DesfireKeyTraits;

template <>
struct DesfireKeyTraits<DesfireKeyType::DES>
{
    static constexpr size_t keySize = 8;
    static constexpr const char *Name = "DES";
    static constexpr size_t blockSize = 8;
};

template <>
struct DesfireKeyTraits<DesfireKeyType::DES3_2KEY>
{
    static constexpr size_t keySize = 16;
    static constexpr const char *Name = "2K3DES";
    static constexpr size_t blockSize = 8;
};

template <>
struct DesfireKeyTraits<DesfireKeyType::DES3_3KEY>
{
    static constexpr size_t keySize = 24;
    static constexpr const char *Name = "3K3DES";
    static constexpr size_t blockSize = 8;
};

template <>
struct DesfireKeyTraits<DesfireKeyType::AES>
{
    static constexpr size_t keySize = 16;
    static constexpr const char *Name = "AES";
    static constexpr size_t blockSize = 16;
};

template <DesfireKeyType T>
class DesfireKey
{
public:
    DesfireKey()
        : blockSize(0), keyVersion(0), keyType(DesfireKeyType::INVALID)
    {
        key.fill(0);
        iv.fill(0);
        Cmac1.fill(0);
        Cmac2.fill(0);
    }

    DesfireKey(const std::array<uint8_t, DesfireKeyTraits<T>::keySize> &keyData, uint8_t version)
        : blockSize(DesfireKeyTraits<T>::blockSize),
          keyVersion(version),
          keyType(T)
    {
        key = keyData;
        iv.fill(0);
        Cmac1.fill(0);
        Cmac2.fill(0);

        this->GenerateCmacSubkeys();
    }

    bool encrypt(std::span<const uint8_t> in, std::span<uint8_t> out)
    {
        if (in.size() != out.size() || in.size() % blockSize != 0)
        {
            std::cerr << "Invalid input or output size for encryption\n";
            return false;
        }
        if constexpr (T == DesfireKeyType::DES)
        {
            processDesBlock(in, out, true);
        }
        else if constexpr (T == DesfireKeyType::DES3_2KEY || T == DesfireKeyType::DES3_3KEY)
        {
            process3desBlock(in, out, true);
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
        constexpr size_t BS = DesfireKeyTraits<T>::blockSize;

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

        auto lshift1 = [&](const std::array<uint8_t, BS> &in){
            std::array<uint8_t, BS> out{};
            uint8_t carry = 0;
            for (size_t i = BS; i-- > 0; ) {
                uint8_t next = (in[i] & 0x80) ? 1 : 0;
                out[i] = static_cast<uint8_t>((in[i] << 1) | carry);
                carry = next;
            }
            return out;
        };

        auto msb_set = [&](const std::array<uint8_t, BS> &a)->bool { return (a[0] & 0x80) != 0; };

        Cmac1 = lshift1(encrypted);
        if (msb_set(encrypted)) Cmac1[BS - 1] ^= CmacR;

        Cmac2 = lshift1(Cmac1);
        if (msb_set(Cmac1)) Cmac2[BS - 1] ^= CmacR;

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

    void process3desBlock(std::span<const uint8_t> inBlock, std::span<uint8_t> outBlock, bool encrypting)
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

        DES3CBC des3cbc(key1_u64, key2_u64, key3_u64, iv64);

        // Convert every 8 bytes to ui64 and process
        for (size_t offset = 0; offset < inBlock.size(); offset += BS)
        {
            auto inChunk = std::span<const uint8_t>(inBlock.data() + offset, BS);
            uint64_t block = from_big_endian<uint64_t>(inChunk);

            uint64_t processed = encrypting ? des3cbc.encrypt(block) : des3cbc.decrypt(block);

            auto outChunk = std::span<uint8_t>(outBlock.data() + offset, BS);
            to_big_endian<uint64_t>(processed, outChunk);
        }
    }

    std::array<uint8_t, DesfireKeyTraits<T>::keySize> key;     // The key to authenticate with
    std::array<uint8_t, DesfireKeyTraits<T>::blockSize> iv;    // Initialization Vector for CBC mode
    std::array<uint8_t, DesfireKeyTraits<T>::blockSize> Cmac1; // CMAC subkey 1
    std::array<uint8_t, DesfireKeyTraits<T>::blockSize> Cmac2; // CMAC subkey 2

    uint8_t blockSize;      // Block size in bytes (8 for DES, 16 for AES)
    uint8_t keyVersion;     // Key version
    DesfireKeyType keyType; // Type of the key (DES, TDES, AES)
};
