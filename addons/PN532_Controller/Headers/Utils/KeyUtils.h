#pragma once

#include <cstdint>
#include <etl/vector.h>

// Derive session key from RndA and RndB (matching the Python snippet):
// session_key_calc = rnda[:4] + rndb[:4] + rnda[4:8] + rndb[4:8]
// first_half = session_key_calc[:8]
// session_key_trace = bytes(b & 0xFE for b in first_half)
// session_key_trace = session_key_trace + session_key_trace
static bool IsSimpleDESKey(const uint8_t key[16])
{
    // simple DES if first 8 bytes == last 8 bytes
    for (int i = 0; i < 8; ++i)
        if (key[i] != key[8 + i])
            return false;
    return true;
}

template <typename T, typename... Ts>
auto make_etl_vector(Ts... args) {
    return etl::vector<T, sizeof...(Ts)>{args...};
}

// Derive session key from RndA/RndB and current key type.
// If currentKey is simple DES (first 8 == last 8) then the session key is
// the first 8 bytes duplicated. Session key trace is the masked first 8 bytes duplicated.
// Otherwise (2K3DES) the session key is the full 16-byte interleaving and the trace
// is each byte masked with 0xFE (no duplication).
static void DeriveSessionKey(const uint8_t rnda[8], const uint8_t rndb[8], const uint8_t currentKey[16], uint8_t outSessionKey[16], uint8_t outSessionKeyTrace[16])
{
    uint8_t session_calc[16];
    // Build session_calc: rnda[0..3], rndb[0..3], rnda[4..7], rndb[4..7]
    for (int i = 0; i < 4; ++i)
        session_calc[i] = rnda[i];
    for (int i = 0; i < 4; ++i)
        session_calc[4 + i] = rndb[i];
    for (int i = 0; i < 4; ++i)
        session_calc[8 + i] = rnda[4 + i];
    for (int i = 0; i < 4; ++i)
        session_calc[12 + i] = rndb[4 + i];

    if (IsSimpleDESKey(currentKey))
    {
        // session key is first 8 bytes duplicated
        for (int i = 0; i < 8; ++i)
            outSessionKey[i] = session_calc[i];
        for (int i = 0; i < 8; ++i)
            outSessionKey[8 + i] = session_calc[i];

        // trace: mask LSB off for first 8 bytes and duplicate
        for (int i = 0; i < 8; ++i)
            outSessionKeyTrace[i] = outSessionKey[i] & 0xFE;
        for (int i = 0; i < 8; ++i)
            outSessionKeyTrace[8 + i] = outSessionKeyTrace[i];
    }
    else
    {
        // session key is the full 16 bytes
        for (int i = 0; i < 16; ++i)
            outSessionKey[i] = session_calc[i];
        // trace is full session key masked per-byte
        for (int i = 0; i < 16; ++i)
            outSessionKeyTrace[i] = outSessionKey[i] & 0xFE;
    }
}

// Calculate CRC-32 (IEEE 802.3 / PKZIP / zlib) over data buffer
uint32_t CalcCrc32(const uint8_t* u8_Data, int s32_Length, uint32_t u32_Crc)
{
    for (int i=0; i<s32_Length; i++)
    {
        u32_Crc ^= u8_Data[i];
        for (int b=0; b<8; b++)
        {
            bool b_Bit = (u32_Crc & 0x01) > 0;
            u32_Crc >>= 1;
            if (b_Bit) u32_Crc ^= 0xEDB88320;
        }
    }
    // Final XOR with 0xFFFFFFFF to match the standard CRC-32 (IEEE 802.3 / PKZIP / zlib)
    // The algorithm above implements the reflected CRC iteration with initial value 0xFFFFFFFF.
    // The canonical final CRC value is the bitwise NOT of the internal register.
    return u32_Crc ^ 0xFFFFFFFF;
}