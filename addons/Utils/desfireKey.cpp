#include <array>
#include <map>
#include <string>

enum class DesfireKeyType
{
    INVALID,
    DES,
    DES3_2KEY,
    DES3_3KEY,
    AES
};

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

    DesfireKey(const std::array<uint8_t, DesfireKeyTraits<T>::>keySize& keyData, uint8_t version)
        : blockSize(DesfireKeyTraits<T>::blockSize), 
        keyVersion(version), 
        keyType(T)
    {
        key = keyData;
        iv.fill(0);
        Cmac1.fill(0);
        Cmac2.fill(0);
    }
    
    std::string getKeyTypeAsString(){
        return std::string(DesfireKeyTraits<T>::Name);
    }


private:

    std::array<uint8_t, 16> key;            // The key to authenticate with
    std::array<uint8_t, 16> iv;             // Initialization Vector for CBC mode
    std::array<uint8_t, 16> Cmac1;          // CMAC subkey 1
    std::array<uint8_t, 16> Cmac2;          // CMAC subkey 2
    
    uint8_t blockSize;                      // Block size in bytes (8 for DES, 16 for AES)
    uint8_t keyVersion;                     // Key version
    DesfireKeyType keyType;                 // Type of the key (DES, TDES, AES)
};