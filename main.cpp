
#include <vector>
#include "stdafx.h"
#include "../DoorOpenerSketch/Desfire.h"
// *****************************************************************************
// The idea of this code is only to check the encryption stuff on Visual Studio.
// But you can also write your own code that communicates with a PN532 board
// through additional hardware (e.g. a PCI card that has digital in/outputs)
// See WinDefines.h
// *****************************************************************************

int _tmain(int argc, _TCHAR* argv[])
{
    // ##################################################
    // Example: Authenticate and ChangeKey (2K3DES)
    // ##################################################
    {

        Utils::Print("\r\n--- Example: Authenticate and ChangeKey (2K3DES) ---\r\n", LF);
        Utils::Print("From: https://www.codeleading.com/article/30286499657/", LF);

        byte keyNo_ex = 1; // example: ChangeKey(KeyNo= 1)
        byte sessionKey[16] = { 0x9C, 0x70, 0x56, 0x82, 0x5C, 0x08, 0x9E, 0xC8, 0x9C, 0x70, 0x56, 0x82, 0x5C, 0x08, 0x9E, 0xC8 };
        byte sessionKeyIV[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        byte newKey[16] = { 0x00, 0x10, 0x20, 0x31, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xB0, 0xA0, 0x90, 0x80 };
        byte currentKey[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

        Utils::Print("*** ChangeKey(Keyno = 1)", LF);
        Utils::Print("* Session Key:        "); Utils::PrintHexBuf(sessionKey, 16, LF);
        Utils::Print("* Session Key IV:     "); Utils::PrintHexBuf(sessionKeyIV, 8, LF);
        Utils::Print("* New Key:            "); Utils::PrintHexBuf(newKey, 16); Utils::Print(" (2K3DES)", LF);
        Utils::Print("* Current Key:        "); Utils::PrintHexBuf(currentKey, 16); Utils::Print(" (DES)", LF);

        // Build cryptogram exactly as Desfire::ChangeKey would do
        std::vector<byte> cryptogram = {};

        // Calculate crc over [ 0xC4, KeyNo, NewKey ] 
        std::vector<byte> crcCryptoBuf{ 0xC4, keyNo_ex };
        for (int i = 0; i < 16; i++) crcCryptoBuf.push_back(newKey[i]);
        uint32_t u32_Crc_crypto = Utils::CalcCrc32(crcCryptoBuf.data(), crcCryptoBuf.size());

        Utils::Print("* CRC Crypto [ 0xC4, KeyNo, NewKey ]: \n\r");
        Utils::Print("CRC Crypto:           0x");   Utils::PrintHex32(u32_Crc_crypto, LF);

        // Calculate CRC of new key alone
        uint32_t u32_Crc_newKey = Utils::CalcCrc32(newKey, 16);
        Utils::Print("* CRC New Key: [ New Key ]: \n\r");
        Utils::Print("CRC New Key           0x"); Utils::PrintHex32(u32_Crc_newKey, LF);

        // Append New Key, CRC Crypto, CRC NewKey
        for (int i = 0; i < 16; i++) cryptogram.push_back(newKey[i]);
        for (int i = 0; i < 4; i++) cryptogram.push_back((u32_Crc_crypto >> (8 * i)) & 0xFF);
        for (int i = 0; i < 4; i++) cryptogram.push_back((u32_Crc_newKey >> (8 * i)) & 0xFF);
        Utils::Print("\*** Cryptogram [New Key, CRC Crypto, CRC NewKey]:", LF);
        Utils::PrintHexBuf((const byte*)cryptogram.data(), (int)cryptogram.size(), LF);

        // Encrypt cryptogram with session key using the same call as Desfire::ChangeKey
        DES sessionKey_ex; // 2K3DES session key (block size 8)
        sessionKey_ex.SetKeyData(sessionKey, 16, 0);
        // Ensure IV is zero (SetKeyData clears IV)
        byte u8_Cryptogram_enc_ex[40] = { 0 };
        // Use CBC_SEND and KEY_ENCIPHER as ChangeKey does
        sessionKey_ex.CryptDataCBC(CBC_SEND, KEY_ENCIPHER, u8_Cryptogram_enc_ex, cryptogram.data(), 24);
        Utils::Print("* CryptogrEnc: ", LF);
        Utils::PrintHexBuf(u8_Cryptogram_enc_ex, 24, LF);

        // Print expected value for quick comparison
        Utils::Print("* Expected CryptogrEnc: ", LF);
        byte expected_enc[24] = { 0x7D,0x83,0xD3,0x4E,0xFB,0x6C,0x84,0x98,0x48,0xE2,0xD6,0x37,0xAD,0xA2,0xD0,0x87,0x14,0x36,0x1A,0xE6,0xC4,0x63,0x14,0x52 };
        Utils::PrintHexBuf(expected_enc, 24, LF);
    }
    {
        // ##################################################
        // Example: Authenticate and ChangeKey (2K3DES)
        // ##################################################

        Utils::Print("\r\n--- Example: Authenticate and ChangeKey (2K3DES) ---\r\n", LF);
        Utils::Print("From: https://www.codeleading.com/article/30286499657/", LF);

        byte keyNo_ex = 1; // example: ChangeKey(KeyNo= 1)

        byte RndA[8] = { 0xC9, 0x6C, 0xE3, 0x5E, 0x4D, 0x60, 0x87, 0xF2 };
        byte RndB[8] = { 0x4C, 0x64, 0x7E, 0x56, 0x72, 0xE2, 0xA6, 0x51 };
        byte sessionKey[16] = {  };
        
        Utils::Print("*** Authenticate example", LF);
        Utils::Print("* RndA:               "); Utils::PrintHexBuf(RndA, 8, LF);
        Utils::Print("* RndB:               "); Utils::PrintHexBuf(RndB, 8, LF);
        Utils::Print("* Session Key:        "); Utils::PrintHexBuf(sessionKey, 16, LF);


        byte sessionKeyIV[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        byte newKey[16] = { 0x00, 0x10, 0x20, 0x31, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xB0, 0xA0, 0x90, 0x80 };
        byte currentKey[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

        Utils::Print("*** ChangeKey(Keyno = 1)", LF);
        Utils::Print("* Session Key:        "); Utils::PrintHexBuf(sessionKey, 16, LF);
        Utils::Print("* Session Key IV:     "); Utils::PrintHexBuf(sessionKeyIV, 8, LF);
        Utils::Print("* New Key:            "); Utils::PrintHexBuf(newKey, 16); Utils::Print(" (2K3DES)", LF);
        Utils::Print("* Current Key:        "); Utils::PrintHexBuf(currentKey, 16); Utils::Print(" (DES)", LF);

        // Build cryptogram exactly as Desfire::ChangeKey would do
        std::vector<byte> cryptogram = {};

        // Calculate crc over [ 0xC4, KeyNo, NewKey ] 
        std::vector<byte> crcCryptoBuf{ 0xC4, keyNo_ex };
        for (int i = 0; i < 16; i++) crcCryptoBuf.push_back(newKey[i]);
        uint32_t u32_Crc_crypto = Utils::CalcCrc32(crcCryptoBuf.data(), crcCryptoBuf.size());

        Utils::Print("* CRC Crypto [ 0xC4, KeyNo, NewKey ]: \n\r");
        Utils::Print("CRC Crypto:           0x");   Utils::PrintHex32(u32_Crc_crypto, LF);

        // Calculate CRC of new key alone
        uint32_t u32_Crc_newKey = Utils::CalcCrc32(newKey, 16);
        Utils::Print("* CRC New Key: [ New Key ]: \n\r");
        Utils::Print("CRC New Key           0x"); Utils::PrintHex32(u32_Crc_newKey, LF);

        // Append New Key, CRC Crypto, CRC NewKey
        for (int i = 0; i < 16; i++) cryptogram.push_back(newKey[i]);
        for (int i = 0; i < 4; i++) cryptogram.push_back((u32_Crc_crypto >> (8 * i)) & 0xFF);
        for (int i = 0; i < 4; i++) cryptogram.push_back((u32_Crc_newKey >> (8 * i)) & 0xFF);
        Utils::Print("\*** Cryptogram [New Key, CRC Crypto, CRC NewKey]:", LF);
        Utils::PrintHexBuf((const byte*)cryptogram.data(), (int)cryptogram.size(), LF);

        // Encrypt cryptogram with session key using the same call as Desfire::ChangeKey
        DES sessionKey_ex; // 2K3DES session key (block size 8)
        sessionKey_ex.SetKeyData(sessionKey, 16, 0);
        // Ensure IV is zero (SetKeyData clears IV)
        byte u8_Cryptogram_enc_ex[40] = { 0 };
        // Use CBC_SEND and KEY_ENCIPHER as ChangeKey does
        sessionKey_ex.CryptDataCBC(CBC_SEND, KEY_ENCIPHER, u8_Cryptogram_enc_ex, cryptogram.data(), 24);
        Utils::Print("* CryptogrEnc: ", LF);
        Utils::PrintHexBuf(u8_Cryptogram_enc_ex, 24, LF);

        // Print expected value for quick comparison
        Utils::Print("* Expected CryptogrEnc: ", LF);
        byte expected_enc[24] = { 0x7D,0x83,0xD3,0x4E,0xFB,0x6C,0x84,0x98,0x48,0xE2,0xD6,0x37,0xAD,0xA2,0xD0,0x87,0x14,0x36,0x1A,0xE6,0xC4,0x63,0x14,0x52 };
        Utils::PrintHexBuf(expected_enc, 24, LF);
    }

    //// ##################################################
    //// Wait for a keypress before exiting
    //// ##################################################
    //getch();
    return 0;
}
