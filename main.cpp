
#include <vector>
#include "stdafx.h"
#include "../DoorOpenerSketch/Desfire.h"
// *****************************************************************************
// The idea of this code is only to check the encryption stuff on Visual Studio.
// But you can also write your own code that communicates with a PN532 board
// through additional hardware (e.g. a PCI card that has digital in/outputs)
// See WinDefines.h
// *****************************************************************************

// Derive session key from RndA and RndB (matching the Python snippet):
// session_key_calc = rnda[:4] + rndb[:4] + rnda[4:8] + rndb[4:8]
// first_half = session_key_calc[:8]
// session_key_trace = bytes(b & 0xFE for b in first_half)
// session_key_trace = session_key_trace + session_key_trace
static bool IsSimpleDESKey(const byte key[16])
{
    // simple DES if first 8 bytes == last 8 bytes
    for (int i = 0; i < 8; ++i)
        if (key[i] != key[8 + i])
            return false;
    return true;
}

// Derive session key from RndA/RndB and current key type.
// If currentKey is simple DES (first 8 == last 8) then the session key is
// the first 8 bytes duplicated. Session key trace is the masked first 8 bytes duplicated.
// Otherwise (2K3DES) the session key is the full 16-byte interleaving and the trace
// is each byte masked with 0xFE (no duplication).
static void DeriveSessionKey(const byte rnda[8], const byte rndb[8], const byte currentKey[16], byte outSessionKey[16], byte outSessionKeyTrace[16])
{
    byte session_calc[16];
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

int _tmain(int argc, _TCHAR *argv[])
{
    // ##################################################
    // Example: Authenticate and ChangeKey (2K3DES)
    // ##################################################
    {

        Utils::Print("\r\n--- Example: Authenticate and ChangeKey (2K3DES) ---\r\n", LF);
        Utils::Print("From: https://www.codeleading.com/article/30286499657/", LF);

        byte keyNo_ex = 1; // example: ChangeKey(KeyNo= 1)
        byte sessionKey[16] = {0x9C, 0x70, 0x56, 0x82, 0x5C, 0x08, 0x9E, 0xC8, 0x9C, 0x70, 0x56, 0x82, 0x5C, 0x08, 0x9E, 0xC8};
        byte sessionKeyIV[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        byte newKey[16] = {0x00, 0x10, 0x20, 0x31, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xB0, 0xA0, 0x90, 0x80};
        byte currentKey[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

        Utils::Print("*** ChangeKey(Keyno = 1)", LF);
        Utils::Print("* Session Key:        ");
        Utils::PrintHexBuf(sessionKey, 16, LF);
        Utils::Print("* Session Key IV:     ");
        Utils::PrintHexBuf(sessionKeyIV, 8, LF);
        Utils::Print("* New Key:            ");
        Utils::PrintHexBuf(newKey, 16);
        Utils::Print(" (2K3DES)", LF);
        Utils::Print("* Current Key:        ");
        Utils::PrintHexBuf(currentKey, 16);
        Utils::Print(" (DES)", LF);

        // Build cryptogram exactly as Desfire::ChangeKey would do
        std::vector<byte> cryptogram = {};

        // Calculate crc over [ 0xC4, KeyNo, NewKey ]
        std::vector<byte> crcCryptoBuf{0xC4, keyNo_ex};
        for (int i = 0; i < 16; i++)
            crcCryptoBuf.push_back(newKey[i]);
        uint32_t u32_Crc_crypto = Utils::CalcCrc32(crcCryptoBuf.data(), crcCryptoBuf.size());

        Utils::Print("* CRC Crypto [ 0xC4, KeyNo, NewKey ]: \n\r");
        Utils::Print("CRC Crypto:           0x");
        Utils::PrintHex32(u32_Crc_crypto, LF);

        // Calculate CRC of new key alone
        uint32_t u32_Crc_newKey = Utils::CalcCrc32(newKey, 16);
        Utils::Print("* CRC New Key: [ New Key ]: \n\r");
        Utils::Print("CRC New Key           0x");
        Utils::PrintHex32(u32_Crc_newKey, LF);

        // Append New Key, CRC Crypto, CRC NewKey
        for (int i = 0; i < 16; i++)
            cryptogram.push_back(newKey[i]);
        for (int i = 0; i < 4; i++)
            cryptogram.push_back((u32_Crc_crypto >> (8 * i)) & 0xFF);
        for (int i = 0; i < 4; i++)
            cryptogram.push_back((u32_Crc_newKey >> (8 * i)) & 0xFF);
        Utils::Print("\*** Cryptogram [New Key, CRC Crypto, CRC NewKey]:", LF);
        Utils::PrintHexBuf((const byte *)cryptogram.data(), (int)cryptogram.size(), LF);

        // Encrypt cryptogram with session key using the same call as Desfire::ChangeKey
        DES sessionKey_ex; // 2K3DES session key (block size 8)
        sessionKey_ex.SetKeyData(sessionKey, 16, 0);
        // Set session IV if provided
        sessionKey_ex.SetIV(sessionKeyIV);
        // Ensure IV is zero (SetKeyData clears IV)
        byte u8_Cryptogram_enc_ex[40] = {0};
        // Use CBC_SEND and KEY_ENCIPHER as ChangeKey does
        sessionKey_ex.CryptDataCBC(CBC_SEND, KEY_ENCIPHER, u8_Cryptogram_enc_ex, cryptogram.data(), 24);
        Utils::Print("* CryptogrEnc: ", LF);
        Utils::PrintHexBuf(u8_Cryptogram_enc_ex, 24, LF);

        // Print expected value for quick comparison
        Utils::Print("* Expected CryptogrEnc: ", LF);
        byte expected_enc[24] = {0x7D, 0x83, 0xD3, 0x4E, 0xFB, 0x6C, 0x84, 0x98, 0x48, 0xE2, 0xD6, 0x37, 0xAD, 0xA2, 0xD0, 0x87, 0x14, 0x36, 0x1A, 0xE6, 0xC4, 0x63, 0x14, 0x52};
        Utils::PrintHexBuf(expected_enc, 24, LF);
    }
    {
        // ##################################################
        // Example: Authenticate and ChangeKey (2K3DES)
        // ##################################################

        Utils::Print("\r\n--- Example: Authenticate and ChangeKey (2K3DES) ---\r\n", LF);
        Utils::Print("From: https://www.codeleading.com/article/30286499657/", LF);

        byte keyNo_ex = 0x00; // example: ChangeKey(KeyNo= 0)

        byte RndA[8] = {0xC9, 0x6C, 0xE3, 0x5E, 0x4D, 0x60, 0x87, 0xF2};
        byte RndB[8] = {0x4C, 0x64, 0x7E, 0x56, 0x72, 0xE2, 0xA6, 0x51};
        byte sessionKey[16] = {};
        byte sessionKeyTrace[16] = {0};
        // Current key for this example (DES - both halves identical)
        byte currentKey[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        // Derive the session key and trace from RndA and RndB
        DeriveSessionKey(RndA, RndB, currentKey, sessionKey, sessionKeyTrace);

        Utils::Print("*** Authenticate example", LF);
        Utils::Print("* RndA:               ");
        Utils::PrintHexBuf(RndA, 8, LF);
        Utils::Print("* RndB:               ");
        Utils::PrintHexBuf(RndB, 8, LF);
        Utils::Print("* Session Key:        ");
        Utils::PrintHexBuf(sessionKey, 16, LF);
        Utils::Print("* Session Key Trace:  ");
        Utils::PrintHexBuf(sessionKeyTrace, 16, LF);

        byte sessionKeyIV[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        byte newKey[16] = {0x00, 0x10, 0x20, 0x31, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xB0, 0xA0, 0x90, 0x80};

        Utils::Print("\n*** ChangeKey(Keyno = 1)", LF);
        Utils::Print("* Session Key:        ");
        Utils::PrintHexBuf(sessionKey, 16, LF);
        Utils::Print("* Session Key IV:     ");
        Utils::PrintHexBuf(sessionKeyIV, 8, LF);
        Utils::Print("* New Key:            ");
        Utils::PrintHexBuf(newKey, 16);
        Utils::Print(" (2K3DES)", LF);
        Utils::Print("* Current Key:        ");
        Utils::PrintHexBuf(currentKey, 16);
        Utils::Print(" (DES)", LF);

        // Build cryptogram exactly as Desfire::ChangeKey would do
        std::vector<byte> cryptogram = {};

        // Calculate crc over [ 0xC4, KeyNo, NewKey ]
        std::vector<byte> crcCryptoBuf{0xC4, keyNo_ex};
        for (int i = 0; i < 16; i++)
            crcCryptoBuf.push_back(newKey[i]);
        uint32_t u32_Crc_crypto = Utils::CalcCrc32(crcCryptoBuf.data(), crcCryptoBuf.size());

        Utils::Print("* CRC Crypto [ 0xC4, KeyNo, NewKey ]: \n\r");
        Utils::Print("CRC Crypto:           0x");
        Utils::PrintHex32(u32_Crc_crypto, LF);

        // Calculate CRC of new key alone
        uint32_t u32_Crc_newKey = Utils::CalcCrc32(newKey, 16);
        Utils::Print("* CRC New Key: [ New Key ]: \n\r");
        Utils::Print("CRC New Key           0x");
        Utils::PrintHex32(u32_Crc_newKey, LF);

        // Append New Key, CRC Crypto, CRC NewKey
        for (int i = 0; i < 16; i++)
            cryptogram.push_back(newKey[i]);
        for (int i = 0; i < 4; i++)
            cryptogram.push_back((u32_Crc_crypto >> (8 * i)) & 0xFF);
        for (int i = 0; i < 4; i++)
            cryptogram.push_back(0x00);
        Utils::Print("\*** Cryptogram [New Key, CRC Crypto, CRC NewKey]:", LF);
        Utils::PrintHexBuf((const byte *)cryptogram.data(), (int)cryptogram.size(), LF);

        // Encrypt cryptogram with session key using the same call as Desfire::ChangeKey
        DES sessionKey_ex; // 2K3DES session key (block size 8)
        sessionKey_ex.SetKeyData(sessionKeyTrace, 16, 0);
        sessionKey_ex.SetIV(sessionKeyIV);
        // Ensure IV is zero (SetKeyData clears IV)
        byte u8_Cryptogram_enc_ex[40] = {0};
        // Use CBC_SEND and KEY_ENCIPHER as ChangeKey does
        sessionKey_ex.CryptDataCBC(CBC_SEND, KEY_ENCIPHER, u8_Cryptogram_enc_ex, cryptogram.data(), 24);
        Utils::Print("* CryptogrEnc: ", LF);
        Utils::PrintHexBuf(u8_Cryptogram_enc_ex, 24, LF);

        // Print expected value for quick comparison
        Utils::Print("* Expected CryptogrEnc: ", LF);
        byte expected_enc[24] = {0xBE, 0xDE, 0x0F, 0xC6, 0xED, 0x34, 0x7D, 0xCF, 0x0D, 0x51, 0xC7, 0x17, 0xDF, 0x75, 0xD9, 0x7D, 0x2C, 0x5A, 0x2B, 0xA6, 0xCA, 0xC7, 0x47, 0x9D};
        Utils::PrintHexBuf(expected_enc, 24, LF);
    }

    {
        // ##################################################
        // Example: Authenticate and ChangeKey (2K3DES)
        // ##################################################

        Utils::Print("\r\n--- Example: Authenticate and ChangeKey (2K3DES) ---\r\n", LF);
        Utils::Print("From: https://www.codeleading.com/article/30286499657/", LF);

        byte keyNo_ex = 0x00; // example: ChangeKey(KeyNo= 0)

        byte RndA[8] = {0x53, 0x0E, 0x3D, 0x90, 0xF7, 0xA2, 0x01, 0xC4};
        byte RndB[8] = {0xBC, 0xD8, 0x29, 0x97, 0x47, 0x33, 0x2D, 0xAF};
        byte sessionKey[16] = {};
        byte sessionKeyTrace[16] = {0};
        // Current key for this example (2K3DES - two different halves)
        byte currentKey[16] = {0x00, 0x10, 0x20, 0x31, 0x40, 0x50, 0x60, 0x70,
                               0x80, 0x90, 0xA0, 0xB0, 0xB0, 0xA0, 0x90, 0x80};
        // Derive the session key and trace from RndA and RndB
        DeriveSessionKey(RndA, RndB, currentKey, sessionKey, sessionKeyTrace);

        Utils::Print("*** Authenticate example", LF);
        Utils::Print("* RndA:               ");
        Utils::PrintHexBuf(RndA, 8, LF);
        Utils::Print("* RndB:               ");
        Utils::PrintHexBuf(RndB, 8, LF);
        Utils::Print("* Session Key:        ");
        Utils::PrintHexBuf(sessionKey, 16, LF);
        Utils::Print("* Session Key Trace:  ");
        Utils::PrintHexBuf(sessionKeyTrace, 16, LF);

        byte sessionKeyIV[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        byte newKey[16] = {0x10, 0x18, 0x20, 0x29, 0x30, 0x38, 0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78, 0x80, 0x88};

        Utils::Print("\n*** ChangeKey(Keyno = 0)", LF);
        Utils::Print("* Session Key:        ");
        Utils::PrintHexBuf(sessionKey, 16, LF);
        Utils::Print("* Session Key IV:     ");
        Utils::PrintHexBuf(sessionKeyIV, 8, LF);
        Utils::Print("* New Key:            ");
        Utils::PrintHexBuf(newKey, 16);
        Utils::Print(" (2K3DES)", LF);
        Utils::Print("* Current Key:        ");
        Utils::PrintHexBuf(currentKey, 16);
        Utils::Print(" (DES)", LF);

        // Build cryptogram exactly as Desfire::ChangeKey would do
        std::vector<byte> cryptogram = {};

        // Calculate crc over [ 0xC4, KeyNo, NewKey ]
        std::vector<byte> crcCryptoBuf{0xC4, keyNo_ex};
        for (int i = 0; i < 16; i++)
            crcCryptoBuf.push_back(newKey[i]);
        uint32_t u32_Crc_crypto = Utils::CalcCrc32(crcCryptoBuf.data(), crcCryptoBuf.size());

        Utils::Print("* CRC Crypto [ 0xC4, KeyNo, NewKey ]: \n\r");
        Utils::Print("CRC Crypto:           0x");
        Utils::PrintHex32(u32_Crc_crypto, LF);

        // Calculate CRC of new key alone
        uint32_t u32_Crc_newKey = Utils::CalcCrc32(newKey, 16);
        Utils::Print("* CRC New Key: [ New Key ]: \n\r");
        Utils::Print("CRC New Key           0x");
        Utils::PrintHex32(u32_Crc_newKey, LF);

        // Append New Key, CRC Crypto, CRC NewKey
        for (int i = 0; i < 16; i++)
            cryptogram.push_back(newKey[i]);
        for (int i = 0; i < 4; i++)
            cryptogram.push_back((u32_Crc_crypto >> (8 * i)) & 0xFF);
        for (int i = 0; i < 4; i++)
            cryptogram.push_back(0x00);
        Utils::Print("\*** Cryptogram [New Key, CRC Crypto, CRC NewKey]:", LF);
        Utils::PrintHexBuf((const byte *)cryptogram.data(), (int)cryptogram.size(), LF);

        // Encrypt cryptogram with session key using the same call as Desfire::ChangeKey
        DES sessionKey_ex; // 2K3DES session key (block size 8)
        sessionKey_ex.SetKeyData(sessionKeyTrace, 16, 0);
        sessionKey_ex.SetIV(sessionKeyIV);
        // Ensure IV is zero (SetKeyData clears IV)
        byte u8_Cryptogram_enc_ex[40] = {0};
        // Use CBC_SEND and KEY_ENCIPHER as ChangeKey does
        sessionKey_ex.CryptDataCBC(CBC_SEND, KEY_ENCIPHER, u8_Cryptogram_enc_ex, cryptogram.data(), 24);
        Utils::Print("* CryptogrEnc: ", LF);
        Utils::PrintHexBuf(u8_Cryptogram_enc_ex, 24, LF);

        // Print expected value for quick comparison
        Utils::Print("* Expected CryptogrEnc: ", LF);
        byte expected_enc[24] = {0x94, 0xE4, 0xF7, 0x09, 0xDC, 0x2A, 0x2B, 0x07, 0x55, 0x26, 0x10, 0xA1, 0x96, 0x6E, 0x5C, 0x49, 0xEC, 0x90, 0xF6, 0x16, 0xED, 0xEC, 0xA5, 0x5B};
        Utils::PrintHexBuf(expected_enc, 24, LF);
    }

    {
        // ##################################################
        // Example: Authenticate and ChangeKey (2K3DES)
        // ##################################################

        Utils::Print("\r\n--- Example: Authenticate and ChangeKey (2K3DES) ---\r\n", LF);
        Utils::Print("From: https://www.codeleading.com/article/30286499657/", LF);

        byte keyNo_ex = 0x00; // example: ChangeKey(KeyNo= 0)

        byte RndA[8] = {0xDD, 0xB0, 0x97, 0xC2, 0xA1, 0xE4, 0x7B, 0x96};
        byte RndB[8] = {0xA4, 0x0E, 0x79, 0xE0, 0xF5, 0x2F, 0x63, 0xAF};
        byte sessionKey[16] = {};
        byte sessionKeyTrace[16] = {0};
        // Current key for this example (2K3DES - two different halves)
        byte currentKey[16] = {0x10, 0x18, 0x20, 0x29, 0x30, 0x38, 0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78, 0x80, 0x88};
        // Derive the session key and trace from RndA and RndB
        DeriveSessionKey(RndA, RndB, currentKey, sessionKey, sessionKeyTrace);

        Utils::Print("*** Authenticate example", LF);
        Utils::Print("* RndA:               ");
        Utils::PrintHexBuf(RndA, 8, LF);
        Utils::Print("* RndB:               ");
        Utils::PrintHexBuf(RndB, 8, LF);
        Utils::Print("* Session Key:        ");
        Utils::PrintHexBuf(sessionKey, 16, LF);
        Utils::Print("* Session Key Trace:  ");
        Utils::PrintHexBuf(sessionKeyTrace, 16, LF);

        byte sessionKeyIV[8] = {0x33, 0x45, 0xAA, 0x95, 0xF2, 0xD9, 0x56, 0xCF};
        byte newKey[16] = {0x00};

        Utils::Print("\n*** ChangeKey(Keyno = 0)", LF);
        Utils::Print("* Session Key:        ");
        Utils::PrintHexBuf(sessionKey, 16, LF);
        Utils::Print("* Session Key IV:     ");
        Utils::PrintHexBuf(sessionKeyIV, 8, LF);
        Utils::Print("* New Key:            ");
        Utils::PrintHexBuf(newKey, 16);
        Utils::Print(" (2K3DES)", LF);
        Utils::Print("* Current Key:        ");
        Utils::PrintHexBuf(currentKey, 16);
        Utils::Print(" (DES)", LF);

        // Build cryptogram exactly as Desfire::ChangeKey would do
        std::vector<byte> cryptogram = {};

        // Calculate crc over [ 0xC4, KeyNo, NewKey ]
        std::vector<byte> crcCryptoBuf{0xC4, keyNo_ex};
        for (int i = 0; i < 16; i++)
            crcCryptoBuf.push_back(newKey[i]);
        uint32_t u32_Crc_crypto = Utils::CalcCrc32(crcCryptoBuf.data(), crcCryptoBuf.size());

        Utils::Print("* CRC Crypto [ 0xC4, KeyNo, NewKey ]: \n\r");
        Utils::Print("CRC Crypto:           0x");
        Utils::PrintHex32(u32_Crc_crypto, LF);

        // Calculate CRC of new key alone
        uint32_t u32_Crc_newKey = Utils::CalcCrc32(newKey, 16);
        Utils::Print("* CRC New Key: [ New Key ]: \n\r");
        Utils::Print("CRC New Key           0x");
        Utils::PrintHex32(u32_Crc_newKey, LF);

        // Append New Key, CRC Crypto, CRC NewKey
        for (int i = 0; i < 16; i++)
            cryptogram.push_back(newKey[i]);
        for (int i = 0; i < 4; i++)
            cryptogram.push_back((u32_Crc_crypto >> (8 * i)) & 0xFF);
        for (int i = 0; i < 4; i++)
            cryptogram.push_back(0x00);
        Utils::Print("\*** Cryptogram [New Key, CRC Crypto, CRC NewKey]:", LF);
        Utils::PrintHexBuf((const byte *)cryptogram.data(), (int)cryptogram.size(), LF);

        // Encrypt cryptogram with session key using the same call as Desfire::ChangeKey
        DES sessionKey_ex; // 2K3DES session key (block size 8)
        sessionKey_ex.SetKeyData(sessionKeyTrace, 16, 0);
        sessionKey_ex.SetIV(sessionKeyIV);
        // Ensure IV is zero (SetKeyData clears IV)
        byte u8_Cryptogram_enc_ex[40] = {0};
        // Use CBC_SEND and KEY_ENCIPHER as ChangeKey does
        sessionKey_ex.CryptDataCBC(CBC_SEND, KEY_ENCIPHER, u8_Cryptogram_enc_ex, cryptogram.data(), 24);
        Utils::Print("* CryptogrEnc: ", LF);
        Utils::PrintHexBuf(u8_Cryptogram_enc_ex, 24, LF);

        // Print expected value for quick comparison
        Utils::Print("* Expected CryptogrEnc: ", LF);
        byte expected_enc[24] = {0xFC, 0x9E, 0x20, 0xFD, 0x77, 0x19, 0x1E, 0x2A, 0xAB, 0x0C, 0xFD, 0x53, 0xD9, 0x99, 0x99, 0x84, 0xBC, 0x59, 0xE8, 0x86, 0xBF, 0xEB, 0x42, 0xD0};
        Utils::PrintHexBuf(expected_enc, 24, LF);
    }

    //// ##################################################
    //// Wait for a keypress before exiting
    //// ##################################################
    // getch();
    return 0;
}
