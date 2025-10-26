#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Helper: trim spaces from hex string and convert to bytes
// ---------------------------------------------------------------------------
std::vector<uint8_t> parseHex(std::string hex)
{
    std::string clean;
    clean.reserve(hex.size());
    for (char ch : hex)
    {
        if (!std::isspace(static_cast<unsigned char>(ch)))
        {
            clean.push_back(ch);
        }
    }
    if (clean.size() % 2 != 0)
    {
        throw std::runtime_error("Hex string length must be even");
    }

    std::vector<uint8_t> bytes;
    bytes.reserve(clean.size() / 2);
    for (std::size_t i = 0; i < clean.size(); i += 2)
    {
        uint8_t value = static_cast<uint8_t>(std::stoi(clean.substr(i, 2), nullptr, 16));
        bytes.push_back(value);
    }
    return bytes;
}

std::string toHex(const std::vector<uint8_t>& data, bool upper = true)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    if (upper)
    {
        oss.setf(std::ios::uppercase);
    }
    for (std::size_t i = 0; i < data.size(); ++i)
    {
        if (i > 0)
        {
            oss << ' ';
        }
        oss << std::setw(2) << static_cast<int>(data[i]);
    }
    return oss.str();
}

// ---------------------------------------------------------------------------
// CRC32 helper (reflected polynomial, identical to DESFire implementation)
// ---------------------------------------------------------------------------
uint32_t crc32_desfire(const uint8_t* data, std::size_t length, uint32_t crc)
{
    constexpr uint32_t poly = 0xEDB88320;
    for (std::size_t i = 0; i < length; ++i)
    {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ poly;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

uint32_t calcCrc32Desfire(const std::vector<uint8_t>& first,
                          const std::vector<uint8_t>& second = {})
{
    // CRC-32 (reflected) standard: initialize to 0xFFFFFFFF, process bytes,
    // then final xor with 0xFFFFFFFF. The byte-wise helper expects the
    // current CRC value, so start with 0xFFFFFFFF here.
    uint32_t crc = crc32_desfire(first.data(), first.size(), 0xFFFFFFFFu);
    if (!second.empty())
    {
        crc = crc32_desfire(second.data(), second.size(), crc);
    }
    return crc ^ 0xFFFFFFFF;
}

// ---------------------------------------------------------------------------
// DES implementation (adapted from the Doorlock project, trimmed for clarity)
// ---------------------------------------------------------------------------
namespace des_detail
{
constexpr uint32_t SPtrans[8][64] = {
    {0x02080800, 0x00080000, 0x02000002, 0x02080802, 0x02000000, 0x00080802, 0x00080002, 0x02000002,
     0x00080802, 0x02080800, 0x02080000, 0x00000802, 0x02000802, 0x02000000, 0x00000000, 0x00080002,
     0x00080000, 0x00000002, 0x02000800, 0x00080800, 0x02080802, 0x02080000, 0x00000802, 0x02000800,
     0x00000002, 0x00000800, 0x00080800, 0x02080002, 0x00000800, 0x02000802, 0x02080002, 0x00000000,
     0x00000000, 0x02080802, 0x02000800, 0x00080002, 0x02080800, 0x00080000, 0x00000802, 0x02000800,
     0x02080002, 0x00000800, 0x00080800, 0x02000002, 0x00080802, 0x00000002, 0x02000002, 0x02080000,
     0x02080802, 0x00080800, 0x02080000, 0x02000802, 0x02000000, 0x00000802, 0x00080002, 0x00000000,
     0x00080000, 0x02000000, 0x02000802, 0x02080800, 0x00000002, 0x02080002, 0x00000800, 0x00080802},
    {0x40108010, 0x00000000, 0x00108000, 0x40100000, 0x40000010, 0x00008010, 0x40008000, 0x00108000,
     0x00008000, 0x40100010, 0x00000010, 0x40008000, 0x00100010, 0x40108000, 0x40100000, 0x00000010,
     0x00100000, 0x40008010, 0x40100010, 0x00008000, 0x00108010, 0x40000000, 0x00000000, 0x00100010,
     0x40008010, 0x00108010, 0x40108000, 0x40000010, 0x40000000, 0x00100000, 0x00008010, 0x40108010,
     0x00100010, 0x40108000, 0x40008000, 0x00108010, 0x40108010, 0x00100010, 0x40000010, 0x00000000,
     0x40000000, 0x00008010, 0x00100000, 0x40100010, 0x00008000, 0x40000000, 0x00108010, 0x40008010,
     0x40108000, 0x00008000, 0x00000000, 0x40000010, 0x00000010, 0x40108010, 0x00108000, 0x40100000,
     0x40100010, 0x00100000, 0x00008010, 0x40008000, 0x40008010, 0x00000010, 0x40100000, 0x00108000},
    {0x04000001, 0x04040100, 0x00000100, 0x04000101, 0x00040001, 0x04000000, 0x04000101, 0x00040100,
     0x04000100, 0x00040000, 0x04040000, 0x00000001, 0x04040101, 0x00000101, 0x00000001, 0x04040001,
     0x00000000, 0x00040001, 0x04040100, 0x00000100, 0x00000101, 0x04040101, 0x00040000, 0x04000001,
     0x04040001, 0x04000100, 0x00040101, 0x04040000, 0x00040100, 0x00000000, 0x04000000, 0x00040101,
     0x04040100, 0x00000100, 0x00000001, 0x00040000, 0x00000101, 0x00040001, 0x04040000, 0x04000101,
     0x00000000, 0x04040100, 0x00040100, 0x04040001, 0x00040001, 0x04000000, 0x04040101, 0x00000001,
     0x00040101, 0x04000001, 0x04000000, 0x04040101, 0x00040000, 0x04000100, 0x04000101, 0x00040100,
     0x04000100, 0x00000000, 0x04040001, 0x00000101, 0x04000001, 0x00040101, 0x00000100, 0x04040000},
    {0x00401008, 0x10001000, 0x00000008, 0x10401008, 0x00000000, 0x10400000, 0x10001008, 0x00400008,
     0x10401000, 0x10000008, 0x10000000, 0x00001008, 0x10000008, 0x00401008, 0x00400000, 0x10000000,
     0x10400008, 0x00401000, 0x00001000, 0x00000008, 0x00401000, 0x10001008, 0x10400000, 0x00001000,
     0x00001008, 0x00000000, 0x00400008, 0x10401000, 0x10001000, 0x10400008, 0x10401008, 0x00400000,
     0x10400008, 0x00001008, 0x00400000, 0x10000008, 0x00401000, 0x10001000, 0x00000008, 0x10400000,
     0x10001008, 0x00000000, 0x00001000, 0x00400008, 0x00000000, 0x10400008, 0x10401000, 0x00001000,
     0x10000000, 0x10401008, 0x00401008, 0x00400000, 0x10401008, 0x00000008, 0x10001000, 0x00401008,
     0x00400008, 0x00401000, 0x10400000, 0x10001008, 0x00001008, 0x10000000, 0x10000008, 0x10401000},
    {0x08000000, 0x00010000, 0x00000400, 0x08010400, 0x08010000, 0x00000000, 0x00010400, 0x08000000,
     0x00010000, 0x00000004, 0x08000004, 0x00010004, 0x00000404, 0x08010004, 0x08010400, 0x00000400,
     0x00010004, 0x08010404, 0x00000004, 0x08010000, 0x00010404, 0x08000004, 0x08000000, 0x00010404,
     0x08010404, 0x00000404, 0x08010004, 0x00010400, 0x00000400, 0x00000000, 0x08010000, 0x00010000,
     0x00000404, 0x08010004, 0x08010404, 0x00000400, 0x00000004, 0x00000000, 0x08010000, 0x00010404,
     0x00010000, 0x00010400, 0x00000400, 0x08000000, 0x08010404, 0x08000004, 0x00010400, 0x00010004,
     0x08000004, 0x00010000, 0x00000000, 0x08010404, 0x08010004, 0x08010400, 0x00010404, 0x00000404,
     0x08010400, 0x08010000, 0x00010004, 0x00000004, 0x00000404, 0x08000000, 0x00000004, 0x08010004},
    {0x80000040, 0x00200040, 0x00200000, 0x00000040, 0x80000000, 0x00002000, 0x00202040, 0x80002040,
     0x00002040, 0x80002000, 0x80002000, 0x80200040, 0x00200040, 0x00202000, 0x80200040, 0x00200000,
     0x80002000, 0x80000040, 0x00202000, 0x80200000, 0x00202040, 0x80002040, 0x80200000, 0x00200040,
     0x80200040, 0x00202000, 0x00000040, 0x00202040, 0x00002000, 0x80000000, 0x00200000, 0x00002040,
     0x00202000, 0x80200040, 0x80002040, 0x00200000, 0x00200040, 0x00002000, 0x80000000, 0x00000040,
     0x80000040, 0x00002040, 0x00002040, 0x00202040, 0x00202040, 0x00200040, 0x80200000, 0x80002000,
     0x00000040, 0x80000000, 0x00002000, 0x80200000, 0x00200000, 0x80002040, 0x80002000, 0x80000040,
     0x80200040, 0x00002040, 0x80002040, 0x00000040, 0x00002000, 0x00202000, 0x80000000, 0x80200040},
    {0x00004000, 0x00000000, 0x40004000, 0x40040040, 0x40000040, 0x40004000, 0x00040040, 0x40000000,
     0x00000000, 0x00040000, 0x40040000, 0x00004040, 0x40040040, 0x00040040, 0x00004040, 0x40040000,
     0x40000000, 0x40000040, 0x00000040, 0x00004000, 0x00040000, 0x00000040, 0x00004000, 0x40004040,
     0x00004040, 0x40040000, 0x40004040, 0x00000040, 0x00040040, 0x40000000, 0x40000040, 0x00000000,
     0x40040000, 0x00004040, 0x00004000, 0x40000040, 0x00000000, 0x40004000, 0x00040000, 0x00040040,
     0x40000000, 0x40004040, 0x00000040, 0x00004000, 0x40004040, 0x00000040, 0x00004040, 0x40000000,
     0x00040040, 0x40040040, 0x40004000, 0x00040000, 0x40000040, 0x00000000, 0x00040000, 0x40040040,
     0x40040040, 0x00040040, 0x00004040, 0x00040000, 0x40000000, 0x40000040, 0x40004000, 0x00000040},
    {0x00000001, 0x02000001, 0x02040001, 0x00040000, 0x00040001, 0x02040000, 0x02000000, 0x02040001,
     0x02000001, 0x00000000, 0x00040000, 0x02000001, 0x02040000, 0x00040001, 0x00000001, 0x02040000,
     0x00000000, 0x00040001, 0x02040001, 0x00000000, 0x02000000, 0x02000001, 0x00040001, 0x02040001,
     0x02000001, 0x00040000, 0x00040000, 0x02000000, 0x02040001, 0x00000001, 0x02000000, 0x00040001,
     0x02040000, 0x00000000, 0x02000000, 0x02040000, 0x00040001, 0x02000000, 0x02040001, 0x00040000,
     0x02000001, 0x02040001, 0x00000000, 0x02000000, 0x00000001, 0x00040000, 0x02000000, 0x02040001,
     0x02040000, 0x00040001, 0x00000001, 0x02040000, 0x02000000, 0x00040000, 0x00040001, 0x00000000,
     0x00000000, 0x02000001, 0x02040000, 0x02040001, 0x02000001, 0x00000001, 0x00040001, 0x02000000}};

constexpr uint32_t bigEndian(const uint8_t* b)
{
    return (static_cast<uint32_t>(b[0]) << 24) | (static_cast<uint32_t>(b[1]) << 16) |
           (static_cast<uint32_t>(b[2]) << 8) | static_cast<uint32_t>(b[3]);
}

constexpr uint32_t ROTATE(uint32_t a, int n)
{
    return (a >> n) | (a << (32 - n));
}

void loadData(uint32_t& u, uint32_t& t, const uint32_t* s, uint32_t R, std::size_t idx)
{
    u = R ^ s[idx];
    t = R ^ s[idx + 1];
}

struct KeySchedule
{
    std::array<uint32_t, 32> subkey{};
};

void setKey(const uint8_t raw[8], KeySchedule& ks)
{
    static const uint32_t pc1[56] = {
        56, 48, 40, 32, 24, 16, 8, 0, 57, 49, 41, 33, 25, 17,
        9, 1, 58, 50, 42, 34, 26, 18, 10, 2, 59, 51, 43, 35,
        62, 54, 46, 38, 30, 22, 14, 6, 61, 53, 45, 37, 29, 21,
        13, 5, 60, 52, 44, 36, 28, 20, 12, 4, 27, 19, 11, 3};

    static const uint32_t shifts[16] = {
        1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1};

    static const uint32_t pc2[48] = {
        13, 16, 10, 23, 0, 4, 2, 27, 14, 5, 20, 9, 22, 18, 11, 3,
        25, 7, 15, 6, 26, 19, 12, 1, 40, 51, 30, 36, 46, 54, 29, 39,
        50, 44, 32, 47, 43, 48, 38, 55, 33, 52, 45, 41, 49, 35, 28, 31};

    uint64_t key = 0;
    for (int i = 0; i < 8; ++i)
    {
        key = (key << 8) | raw[i];
    }

    uint32_t C = 0;
    uint32_t D = 0;
    for (int i = 0; i < 28; ++i)
    {
        C <<= 1;
        C |= (key >> (63 - pc1[i])) & 0x1;
        D <<= 1;
        D |= (key >> (63 - pc1[i + 28])) & 0x1;
    }

    for (int round = 0; round < 16; ++round)
    {
        C = ((C << shifts[round]) | (C >> (28 - shifts[round]))) & 0x0FFFFFFF;
        D = ((D << shifts[round]) | (D >> (28 - shifts[round]))) & 0x0FFFFFFF;

        uint64_t CD = (static_cast<uint64_t>(C) << 28) | D;

        ks.subkey[round * 2] = 0;
        ks.subkey[round * 2 + 1] = 0;
        for (int j = 0; j < 24; ++j)
        {
            ks.subkey[round * 2] <<= 1;
            ks.subkey[round * 2] |= (CD >> (55 - pc2[j])) & 1;
        }
        for (int j = 24; j < 48; ++j)
        {
            ks.subkey[round * 2 + 1] <<= 1;
            ks.subkey[round * 2 + 1] |= (CD >> (55 - pc2[j])) & 1;
        }
    }
}

void permuteIP(uint32_t& L, uint32_t& R)
{
    uint32_t tt;
    tt = ((L >> 4) ^ R) & 0x0F0F0F0F;
    R ^= tt;
    L ^= (tt << 4);
    tt = ((L >> 16) ^ R) & 0x0000FFFF;
    R ^= tt;
    L ^= (tt << 16);
    tt = ((R >> 2) ^ L) & 0x33333333;
    L ^= tt;
    R ^= (tt << 2);
    tt = ((R >> 8) ^ L) & 0x00FF00FF;
    L ^= tt;
    R ^= (tt << 8);
    tt = ((L >> 1) ^ R) & 0x55555555;
    R ^= tt;
    L ^= (tt << 1);
}

void permuteFP(uint32_t& L, uint32_t& R)
{
    uint32_t tt;
    tt = ((L >> 1) ^ R) & 0x55555555;
    R ^= tt;
    L ^= (tt << 1);
    tt = ((R >> 8) ^ L) & 0x00FF00FF;
    L ^= tt;
    R ^= (tt << 8);
    tt = ((R >> 2) ^ L) & 0x33333333;
    L ^= tt;
    R ^= (tt << 2);
    tt = ((L >> 16) ^ R) & 0x0000FFFF;
    R ^= tt;
    L ^= (tt << 16);
    tt = ((L >> 4) ^ R) & 0x0F0F0F0F;
    R ^= tt;
    L ^= (tt << 4);
}

void encryptBlock(const KeySchedule& ks, const uint8_t in[8], uint8_t out[8])
{
    uint32_t L = bigEndian(in);
    uint32_t R = bigEndian(in + 4);

    permuteIP(L, R);

    const uint32_t* s = ks.subkey.data();

    for (int i = 0; i < 16; ++i)
    {
        uint32_t u, t;
        loadData(u, t, s, R, i * 2);
        t = ROTATE(t, 4);
        uint32_t tmp = L ^ SPtrans[0][(u >> 2) & 0x3F] ^
                       SPtrans[2][(u >> 10) & 0x3F] ^
                       SPtrans[4][(u >> 18) & 0x3F] ^
                       SPtrans[6][(u >> 26) & 0x3F] ^
                       SPtrans[1][(t >> 2) & 0x3F] ^
                       SPtrans[3][(t >> 10) & 0x3F] ^
                       SPtrans[5][(t >> 18) & 0x3F] ^
                       SPtrans[7][(t >> 26) & 0x3F];
        L = R;
        R = tmp;
    }

    permuteFP(R, L);

    out[0] = static_cast<uint8_t>((R >> 24) & 0xFF);
    out[1] = static_cast<uint8_t>((R >> 16) & 0xFF);
    out[2] = static_cast<uint8_t>((R >> 8) & 0xFF);
    out[3] = static_cast<uint8_t>(R & 0xFF);
    out[4] = static_cast<uint8_t>((L >> 24) & 0xFF);
    out[5] = static_cast<uint8_t>((L >> 16) & 0xFF);
    out[6] = static_cast<uint8_t>((L >> 8) & 0xFF);
    out[7] = static_cast<uint8_t>(L & 0xFF);
}

void decryptBlock(const KeySchedule& ks, const uint8_t in[8], uint8_t out[8])
{
    KeySchedule reversed = ks;
    for (int i = 0; i < 16; ++i)
    {
        std::swap(reversed.subkey[i * 2], reversed.subkey[(15 - i) * 2]);
        std::swap(reversed.subkey[i * 2 + 1], reversed.subkey[(15 - i) * 2 + 1]);
    }
    encryptBlock(reversed, in, out);
}

} // namespace des_detail

void desEncryptBlock(const uint8_t key[8], const uint8_t in[8], uint8_t out[8])
{
    des_detail::KeySchedule ks;
    des_detail::setKey(key, ks);
    des_detail::encryptBlock(ks, in, out);
}

void desDecryptBlock(const uint8_t key[8], const uint8_t in[8], uint8_t out[8])
{
    des_detail::KeySchedule ks;
    des_detail::setKey(key, ks);
    des_detail::decryptBlock(ks, in, out);
}

void des2K3EncryptBlock(const std::vector<uint8_t>& key, const uint8_t in[8], uint8_t out[8])
{
    uint8_t tmp1[8];
    uint8_t tmp2[8];
    desEncryptBlock(key.data(), in, tmp1);
    desDecryptBlock(key.data() + 8, tmp1, tmp2);
    desEncryptBlock(key.data(), tmp2, out);
}

// ---------------------------------------------------------------------------
// DESFire helper routines
// ---------------------------------------------------------------------------
std::vector<uint8_t> buildSessionKey(const std::vector<uint8_t>& rndA,
                                     const std::vector<uint8_t>& rndB,
                                     bool is2K3DES)
{
    if (rndA.size() < 8 || rndB.size() < 8)
    {
        throw std::runtime_error("RndA and RndB must be at least 8 bytes each");
    }

    std::vector<uint8_t> key;
    key.insert(key.end(), rndA.begin(), rndA.begin() + 4);
    key.insert(key.end(), rndB.begin(), rndB.begin() + 4);
    key.insert(key.end(), rndA.begin() + 4, rndA.begin() + 8);
    key.insert(key.end(), rndB.begin() + 4, rndB.begin() + 8);

    if (!is2K3DES)
    {
        key.resize(8);
    }
    else if (key.size() < 16)
    {
        key.resize(16, 0x00);
    }
    return key;
}

std::vector<uint8_t> applyVersionBits(const std::vector<uint8_t>& key, uint8_t version)
{
    if (key.empty())
    {
        throw std::runtime_error("Key must not be empty");
    }

    std::vector<uint8_t> result = key;
    uint8_t mask = 0x80;
    std::size_t limit = std::min<std::size_t>(8, result.size());
    for (std::size_t i = 0; i < limit; ++i)
    {
        uint8_t parity = (version & mask) ? 0x01 : 0x00;
        result[i] = (result[i] & 0xFE) | parity;
        mask >>= 1;
    }
    for (std::size_t i = 8; i < result.size(); ++i)
    {
        result[i] &= 0xFE;
    }
    return result;
}

std::vector<uint8_t> xorKeys(const std::vector<uint8_t>& newKey,
                             const std::vector<uint8_t>& oldKey)
{
    if (newKey.size() != oldKey.size())
    {
        throw std::runtime_error("New key and old key must have the same length");
    }
    std::vector<uint8_t> result(newKey.size());
    for (std::size_t i = 0; i < newKey.size(); ++i)
    {
        result[i] = newKey[i] ^ oldKey[i];
    }
    return result;
}

std::vector<uint8_t> encryptSendMode(const std::vector<uint8_t>& sessionKey,
                                     const std::vector<uint8_t>& plaintext,
                                     const std::vector<uint8_t>& iv)
{
    if (plaintext.size() % 8 != 0)
    {
        throw std::runtime_error("Plaintext length must be a multiple of 8 bytes");
    }
    if (iv.size() != 8)
    {
        throw std::runtime_error("IV must be exactly 8 bytes");
    }

    std::vector<uint8_t> currentIV = iv;
    std::vector<uint8_t> ciphertext(plaintext.size());

    if (sessionKey.size() == 8)
    {
        for (std::size_t offset = 0; offset < plaintext.size(); offset += 8)
        {
            uint8_t block[8];
            for (int i = 0; i < 8; ++i)
            {
                block[i] = static_cast<uint8_t>(plaintext[offset + i] ^ currentIV[i]);
            }
            uint8_t enc[8];
            desEncryptBlock(sessionKey.data(), block, enc);
            std::copy(enc, enc + 8, ciphertext.begin() + offset);
            currentIV.assign(enc, enc + 8);
        }
    }
    else if (sessionKey.size() == 16)
    {
        for (std::size_t offset = 0; offset < plaintext.size(); offset += 8)
        {
            uint8_t block[8];
            for (int i = 0; i < 8; ++i)
            {
                block[i] = static_cast<uint8_t>(plaintext[offset + i] ^ currentIV[i]);
            }
            uint8_t enc[8];
            des2K3EncryptBlock(sessionKey, block, enc);
            std::copy(enc, enc + 8, ciphertext.begin() + offset);
            currentIV.assign(enc, enc + 8);
        }
    }
    else
    {
        throw std::runtime_error("Session key must be 8 or 16 bytes long");
    }
    return ciphertext;
}

std::vector<uint8_t> padToBlock(const std::vector<uint8_t>& data)
{
    if (data.empty())
    {
        return std::vector<uint8_t>(8, 0x00);
    }
    std::vector<uint8_t> out = data;
    while (out.size() % 8 != 0)
    {
        out.push_back(0x00);
    }
    return out;
}

// ---------------------------------------------------------------------------
// CLI parsing
// ---------------------------------------------------------------------------
struct Options
{
    std::vector<uint8_t> rndA;
    std::vector<uint8_t> rndB;
    std::vector<uint8_t> newKey;
    std::vector<uint8_t> oldKey;
    std::vector<uint8_t> sessionIV = std::vector<uint8_t>(8, 0x00);
    std::optional<std::vector<uint8_t>> sessionKeyOverride;
    uint8_t version = 0x00;
    uint8_t oldVersion = 0x00;
    uint8_t keyNo = 0x00;
    bool is2K3DES = true;
    std::optional<uint8_t> authKeyNo;
    std::optional<bool> sameKeyOverride;
    bool includeKeyType = false;
};

Options parseArguments(int argc, char* argv[])
{
    Options opts;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        auto requireValue = [&](const std::string& name) -> std::string {
            if (i + 1 >= argc)
            {
                throw std::runtime_error("Missing value for " + name);
            }
            return argv[++i];
        };

        if (arg == "--rnda")
        {
            opts.rndA = parseHex(requireValue(arg));
        }
        else if (arg == "--rndb")
        {
            opts.rndB = parseHex(requireValue(arg));
        }
        else if (arg == "--new-key")
        {
            opts.newKey = parseHex(requireValue(arg));
        }
        else if (arg == "--old-key")
        {
            opts.oldKey = parseHex(requireValue(arg));
        }
        else if (arg == "--key-version")
        {
            opts.version = static_cast<uint8_t>(std::stoi(requireValue(arg), nullptr, 16));
        }
        else if (arg == "--old-key-version")
        {
            opts.oldVersion = static_cast<uint8_t>(std::stoi(requireValue(arg), nullptr, 16));
        }
        else if (arg == "--key-no")
        {
            opts.keyNo = static_cast<uint8_t>(std::stoi(requireValue(arg)));
        }
        else if (arg == "--auth-key-no")
        {
            opts.authKeyNo = static_cast<uint8_t>(std::stoi(requireValue(arg)));
        }
        else if (arg == "--key-type")
        {
            std::string value = requireValue(arg);
            if (value == "des")
            {
                opts.is2K3DES = false;
            }
            else if (value == "2k3des")
            {
                opts.is2K3DES = true;
            }
            else
            {
                throw std::runtime_error("Unknown key type: " + value);
            }
        }
        else if (arg == "--iv")
        {
            opts.sessionIV = parseHex(requireValue(arg));
        }
        else if (arg == "--session-key")
        {
            opts.sessionKeyOverride = parseHex(requireValue(arg));
        }
        else if (arg == "--same-key")
        {
            opts.sameKeyOverride = true;
        }
        else if (arg == "--different-key")
        {
            opts.sameKeyOverride = false;
        }
        else if (arg == "--include-key-type-bit")
        {
            opts.includeKeyType = true;
        }
        else if (arg == "--help" || arg == "-h")
        {
            std::cout << "Usage: desfire_key_tool [options]\n\n"
                      << "  --rnda <hex>         8-byte random A\n"
                      << "  --rndb <hex>         8-byte random B\n"
                      << "  --new-key <hex>      New key material (8 or 16 bytes)\n"
                      << "  --old-key <hex>      Current key (defaults to zero)\n"
                      << "  --key-version <hex>  Key version (default 0x00)\n"
                      << "  --old-key-version <hex> Version of current key (default 0x00)\n"
                      << "  --key-no <int>       ChangeKey number (default 0)\n"
                      << "  --auth-key-no <int>  Key number used for authentication\n"
                      << "  --key-type <des|2k3des> (default 2k3des)\n"
                      << "  --iv <hex>           CBC IV (default 0000000000000000)\n"
                      << "  --session-key <hex>  Override session key (skip RndA/B)\n"
                      << "  --same-key           Force \"same key\" behavior (no XOR)\n"
                      << "  --different-key      Force XOR with current key\n"
                      << "  --include-key-type-bit  OR key type into ChangeKey P1 (PICC master behavior)\n"
                      << std::endl;
            std::exit(0);
        }
        else
        {
            throw std::runtime_error("Unknown argument: " + arg);
        }
    }

    if (!opts.sessionKeyOverride)
    {
        if (opts.rndA.size() < 8 || opts.rndB.size() < 8)
        {
            throw std::runtime_error("RndA and RndB must be provided (8 bytes each)");
        }
    }
    if (opts.newKey.empty())
    {
        throw std::runtime_error("New key must be provided");
    }
    if (opts.sessionIV.size() != 8)
    {
        throw std::runtime_error("IV must be exactly 8 bytes");
    }
    return opts;
}

int main(int argc, char* argv[])
{
    try
    {
        Options opts = parseArguments(argc, argv);

        std::vector<uint8_t> sessionKeyRaw;
        if (opts.sessionKeyOverride)
        {
            sessionKeyRaw = *opts.sessionKeyOverride;
        }
        else
        {
            sessionKeyRaw = buildSessionKey(opts.rndA, opts.rndB, opts.is2K3DES);
        }

        std::vector<uint8_t> sessionKeyParity = applyVersionBits(sessionKeyRaw, opts.version);
        const std::vector<uint8_t>& newKeyData = opts.newKey;
        std::vector<uint8_t> oldKeyData = opts.oldKey;
        std::vector<uint8_t> newKeyVersioned = applyVersionBits(newKeyData, opts.version);
        std::vector<uint8_t> oldKeyVersioned;
        if (!oldKeyData.empty())
        {
            oldKeyVersioned = applyVersionBits(oldKeyData, opts.oldVersion);
        }

        std::cout << "Session key (raw):        " << toHex(sessionKeyRaw) << '\n';
        std::cout << "Session key (with ver):   " << toHex(sessionKeyParity) << '\n';

        if (!oldKeyData.empty() && oldKeyData.size() != newKeyData.size())
        {
            throw std::runtime_error("New key and old key lengths differ");
        }

        bool sameKey = opts.sameKeyOverride.value_or(false);
        if (!opts.sameKeyOverride)
        {
            if (opts.authKeyNo.has_value())
            {
                sameKey = (*opts.authKeyNo == opts.keyNo);
            }
            else if (!oldKeyVersioned.empty() && oldKeyVersioned == newKeyVersioned)
            {
                sameKey = true;
            }
        }

        if (!sameKey && oldKeyData.empty())
        {
            oldKeyData.assign(newKeyData.size(), 0x00);
            oldKeyVersioned = applyVersionBits(oldKeyData, opts.oldVersion);
        }

        if (!sameKey && oldKeyData.size() != newKeyData.size())
        {
            throw std::runtime_error("Current key length must match new key for XOR");
        }

        std::vector<uint8_t> xorKey;
        if (!sameKey)
        {
            xorKey = xorKeys(newKeyVersioned, oldKeyVersioned);
            std::cout << "XOR(New,Old):             " << toHex(xorKey) << '\n';
        }

        std::cout << "New key:                  " << toHex(newKeyData) << '\n';
        std::cout << "New key (with ver):       " << toHex(newKeyVersioned) << '\n';
        if (!oldKeyData.empty())
        {
            std::cout << "Old key:                  " << toHex(oldKeyData) << '\n';
            std::cout << "Old key (with ver):       " << toHex(oldKeyVersioned) << '\n';
        }

        uint8_t commandKeyNo = opts.keyNo;
        if (opts.includeKeyType && opts.is2K3DES)
        {
            commandKeyNo |= 0x40;
        }

        std::vector<uint8_t> command = {0xC4, commandKeyNo};
        std::vector<uint8_t> cryptogram = sameKey ? newKeyData : xorKey;
        uint32_t crcCrypto = calcCrc32Desfire(cryptogram);
        cryptogram.push_back(static_cast<uint8_t>((crcCrypto >> 24) & 0xFF));
        cryptogram.push_back(static_cast<uint8_t>((crcCrypto >> 16) & 0xFF));
        cryptogram.push_back(static_cast<uint8_t>((crcCrypto >> 8) & 0xFF));
        cryptogram.push_back(static_cast<uint8_t>(crcCrypto & 0xFF));

        uint32_t crcNewKey = 0;
        if (!sameKey)
        {
            crcNewKey = calcCrc32Desfire(newKeyVersioned);
            cryptogram.push_back(static_cast<uint8_t>((crcNewKey >> 24) & 0xFF));
            cryptogram.push_back(static_cast<uint8_t>((crcNewKey >> 16) & 0xFF));
            cryptogram.push_back(static_cast<uint8_t>((crcNewKey >> 8) & 0xFF));
            cryptogram.push_back(static_cast<uint8_t>(crcNewKey & 0xFF));
        }

        std::cout << "CRC Crypto:               0x" << std::hex << std::uppercase << std::setw(8)
                  << std::setfill('0') << crcCrypto << std::dec << std::nouppercase << '\n';
        if (!sameKey)
        {
            std::cout << "CRC New Key:              0x" << std::hex << std::uppercase << std::setw(8)
                      << std::setfill('0') << crcNewKey << std::dec << std::nouppercase << '\n';
        }

        std::vector<uint8_t> plaintext = padToBlock(cryptogram);
        std::cout << "Cryptogram (padded):      " << toHex(plaintext) << '\n';

        std::vector<uint8_t> ciphertext = encryptSendMode(sessionKeyParity, plaintext, opts.sessionIV);
        std::cout << "Encrypted payload:        " << toHex(ciphertext) << '\n';

        std::vector<uint8_t> apdu = {0xC4, commandKeyNo};
        apdu.insert(apdu.end(), ciphertext.begin(), ciphertext.end());

        std::cout << "APDU (C4 P1 + payload):   " << toHex(apdu) << '\n';
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
