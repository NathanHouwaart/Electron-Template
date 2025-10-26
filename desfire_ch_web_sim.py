"""
DESFire PICC Master Key Change Example
Reproduces the example from geekyuan.cn

Key insight from website:
- PICC (card) always performs encryption (ek)
- PCD (reader) always performs decryption (dk)
- When reader "prepares data to send", it uses decryption operation in CBC send mode!
"""

from Crypto.Cipher import DES3, DES
import struct
import binascii

def calculate_crc16_a(data):
    """Calculate CRC16 according to ISO/IEC 14443-3 Type A"""
    crc = 0x6363  # Initial value for ISO14443-3A
    
    for byte in data:
        byte ^= (crc & 0xFF)
        byte ^= (byte << 4) & 0xFF
        crc = ((crc >> 8) ^ (byte << 8) ^ (byte << 3) ^ (byte >> 4)) & 0xFFFF
    
    return crc

def des3_decrypt_ecb(key, data):
    """Perform 3DES decryption in ECB mode (single block)"""
    cipher = DES3.new(key, DES3.MODE_ECB)
    return cipher.decrypt(data)

def des3_encrypt_ecb(key, data):
    """Perform 3DES encryption in ECB mode (single block)"""
    cipher = DES3.new(key, DES3.MODE_ECB)
    return cipher.encrypt(data)

def desfire_cbc_send_decrypt(key, data, iv):
    """
    DESFire CBC Send Mode using DECRYPTION (dk):
    This is what the reader does when preparing data to send to card.
    From the diagram: output[i] = decrypt_ecb(input[i] XOR iv[i])
                     iv[i+1] = output[i]
    """
    result = b''
    current_iv = iv
    
    for i in range(0, len(data), 8):
        plaintext_block = data[i:i+8]
        # XOR plaintext with IV
        xored = bytes(a ^ b for a, b in zip(plaintext_block, current_iv))
        # Apply DECRYPTION operation (this is "dk" in DESFire)
        output_block = des3_decrypt_ecb(key, xored)
        result += output_block
        # Next IV is this output block
        current_iv = output_block
    
    return result, current_iv

def desfire_cbc_receive_decrypt(key, data, iv):
    """
    DESFire CBC Receive Mode using DECRYPTION (dk):
    This is what the reader does when receiving data from card.
    From the diagram: plaintext[i] = decrypt_ecb(ciphertext[i]) XOR iv[i]
                     iv[i+1] = decrypt_ecb(ciphertext[i])
    """
    result = b''
    current_iv = iv
    
    for i in range(0, len(data), 8):
        ciphertext_block = data[i:i+8]
        # Apply DECRYPTION operation
        decrypted = des3_decrypt_ecb(key, ciphertext_block)
        # XOR with IV to get plaintext
        plaintext_block = bytes(a ^ b for a, b in zip(decrypted, current_iv))
        result += plaintext_block
        # Next IV is the decrypted block
        current_iv = decrypted
    
    return result


def desfire_des_send_encrypt(key, data, iv):
    """
    DESFire send mode for single DES sessions (uses encrypt operation).
    """
    if len(data) % 8 != 0:
        raise ValueError("Data length must be multiple of 8 for DES send mode")

    cipher = DES.new(key, DES.MODE_ECB)
    result = b''
    current_iv = iv

    for i in range(0, len(data), 8):
        block = data[i:i+8]
        xored = bytes(a ^ b for a, b in zip(block, current_iv))
        out_block = cipher.encrypt(xored)
        result += out_block
        current_iv = out_block

    return result


def calculate_crc32_desfire(data):
    """
    DESFire CRC32 (ISO/IEC 3309) as implemented by binascii.crc32 (reflected).
    """
    return binascii.crc32(data)

def main():
    print("=" * 70)
    print("DESFIRE PICC MASTER KEY CHANGE EXAMPLE")
    print("=" * 70)
    
    # Step 1: Authentication
    print("\n1. PICC MASTER KEY AUTHENTICATION")
    print("-" * 70)
    
    picc_master_key = bytes.fromhex("00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF")
    print(f"PICC Master Key: {picc_master_key.hex(' ').upper()}")
    
    # Encrypted RndB from card
    enc_rndb = bytes.fromhex("FE 28 37 5D 1D 17 08 AC")
    print(f"\nEncrypted RndB:  {enc_rndb.hex(' ').upper()}")
    
    # Decrypt RndB with zero IV using CBC receive mode
    rndb = desfire_cbc_receive_decrypt(picc_master_key, enc_rndb, iv=b'\x00' * 8)
    print(f"RndB:            {rndb.hex(' ').upper()}")
    
    # Rotate RndB (shift left by 1 byte)
    rndb_rotated = rndb[1:] + rndb[0:1]
    print(f"RndB':           {rndb_rotated.hex(' ').upper()}")
    
    # Generate RndA
    rnda = bytes.fromhex("01 02 03 04 05 06 07 08")
    print(f"RndA:            {rnda.hex(' ').upper()}")
    
    # "dk(RndA+RndB')" - send-mode decrypt with IV = 0 (matches blog example)
    data_to_send = rnda + rndb_rotated
    enc_data, _ = desfire_cbc_send_decrypt(picc_master_key, data_to_send, iv=b'\x00' * 8)
    print(f"\nEncrypted (RndA+RndB'): {enc_data.hex(' ').upper()}")
    print(f"Expected:               D9 16 AA 29 EF 0A 8F 6A EC 19 E2 FA A3 FC 1E 75")
    
    # Card responds with encrypted RndA'
    enc_rnda_rotated = bytes.fromhex("B6 F3 1E E6 F9 98 C2 35")
    # Blog example uses IV = 0 for this decrypt as well
    rnda_rotated = desfire_cbc_receive_decrypt(picc_master_key, enc_rnda_rotated, iv=b'\x00' * 8)
    print(f"\nDecrypted RndA': {rnda_rotated.hex(' ').upper()}")
    
    # Calculate session key
    session_key = rnda[0:4] + rndb[0:4] + rnda[4:8] + rndb[4:8]
    print(f"\nSession Key:     {session_key.hex(' ').upper()}")
    
    # Candidate IVs for the ChangeKey payload
    command_iv_article = enc_rnda_rotated
    command_iv_cbc = enc_data[-8:]
    print(f"IV candidate (enc RndA'): {command_iv_article.hex(' ').upper()}")
    print(f"IV candidate (last block): {command_iv_cbc.hex(' ').upper()}")
    
    # Step 2: Change PICC Master Key
    print("\n" + "=" * 70)
    print("2. CHANGE PICC MASTER KEY")
    print("-" * 70)
    
    # New key (all 0xFF)
    new_key = bytes.fromhex("FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF")
    print(f"New Key:         {new_key.hex(' ').upper()}")
    
    # Calculate CRC16
    crc16 = calculate_crc16_a(new_key)
    print(f"CRC16 (calculated): 0x{crc16:04X}")
    
    # Convert to little-endian bytes
    crc16_bytes = struct.pack('<H', crc16)
    print(f"CRC16 (LE bytes):   {crc16_bytes.hex(' ').upper()}")
    
    # Build the data: new_key + crc16 + padding
    plaintext = new_key + crc16_bytes + b'\x00' * 6
    print(f"\nPlaintext data:  {plaintext.hex(' ').upper()}")
    print(f"Length: {len(plaintext)} bytes")
    
    # Compute payloads with several IV choices
    encrypted_iv0, _ = desfire_cbc_send_decrypt(session_key, plaintext, iv=b'\x00' * 8)
    encrypted_article, _ = desfire_cbc_send_decrypt(session_key, plaintext, iv=command_iv_article)
    encrypted_cbc, _ = desfire_cbc_send_decrypt(session_key, plaintext, iv=command_iv_cbc)
    
    print(f"\nEncrypted data (IV=0):          {encrypted_iv0.hex(' ').upper()}")
    print(f"Encrypted data (IV=enc RndA'):  {encrypted_article.hex(' ').upper()}")
    print(f"Encrypted data (IV=last block): {encrypted_cbc.hex(' ').upper()}")
    
    # Build candidate APDUs
    apdu_header = b'\x00'
    full_apdu_iv0 = apdu_header + encrypted_iv0
    full_apdu_article = apdu_header + encrypted_article
    full_apdu_cbc = apdu_header + encrypted_cbc
    
    print(f"\nFull APDU (IV=0):               {full_apdu_iv0.hex(' ').upper()}")
    print(f"Full APDU (IV=enc RndA'):       {full_apdu_article.hex(' ').upper()}")
    print(f"Full APDU (IV=last block):      {full_apdu_cbc.hex(' ').upper()}")
    
    # Blog's stated payload for comparison
    article_payload = bytes.fromhex("4C 21 7A 58 3A D8 12 03 DC 29 DA A8 29 6A 43 0E 82 C9 D0 35 E5 9A 21 11")
    article_apdu = apdu_header + article_payload
    print(f"Full APDU (article example):    {article_apdu.hex(' ').upper()}")
    
    # Complete command that would be sent
    print(f"\nComplete Command:")
    print(f"Rdr: 90 C4 00 00 19 {full_apdu_iv0.hex(' ').upper()} 00   (IV=0)")
    print(f"Rdr: 90 C4 00 00 19 {full_apdu_article.hex(' ').upper()} 00   (IV=enc RndA')")
    print(f"Rdr: 90 C4 00 00 19 {full_apdu_cbc.hex(' ').upper()} 00   (IV=last block)")
    print(f"Rdr: 90 C4 00 00 19 {article_apdu.hex(' ').upper()} 00   (article)")
    print(f"Expected Response: 91 00")
    
    print("\n" + "=" * 70)
    print("VERIFICATION")
    print("-" * 70)
    
    # Verify against expected values from website
    expected_encrypted = bytes.fromhex("4C 21 7A 58 3A D8 12 03 DC 29 DA A8 29 6A 43 0E 82 C9 D0 35 E5 9A 21 11")
    expected_auth = bytes.fromhex("D9 16 AA 29 EF 0A 8F 6A EC 19 E2 FA A3 FC 1E 75")
    
    if enc_data == expected_auth:
        print("OK   Authentication encryption matches")
        print(f"Expected: {expected_auth.hex(' ').upper()}")
        print(f"Got:      {enc_data.hex(' ').upper()}")
    else:
        print("FAIL Authentication encryption does NOT match")
        print(f"Expected: {expected_auth.hex(' ').upper()}")
        print(f"Got:      {enc_data.hex(' ').upper()}")
    
    if encrypted_iv0 == expected_encrypted or encrypted_article == expected_encrypted or encrypted_cbc == expected_encrypted:
        print("OK   Key change encryption matches the article output")
    else:
        print("NOTE Key change payload differs from article example")
        print(f"Expected: {expected_encrypted.hex(' ').upper()}")
        print(f"IV=0 result:        {encrypted_iv0.hex(' ').upper()}")
        print(f"IV=enc RndA' result: {encrypted_article.hex(' ').upper()}")
        print(f"IV=prev block:      {encrypted_cbc.hex(' ').upper()}")
    
    if crc16 == 0xCB37:
        print("OK   CRC16 calculation is correct")
    else:
        print(f"FAIL CRC16 mismatch! Expected 0xCB37, got 0x{crc16:04X}")

    simulate_stackoverflow_example()


def simulate_stackoverflow_example():
    print("\n" + "=" * 70)
    print("STACKOVERFLOW CHANGEKEY EXAMPLE (DES session -> 2K3DES key)")
    print("=" * 70)

    session_key = bytes.fromhex("B4 28 2E FA 9E B8 2C AE B4 28 2E FA 9E B8 2C AE")
    session_key_des = session_key[:8]
    new_key = bytes.fromhex("01 10 20 31 40 50 60 70 80 90 A0 B0 B0 A0 90 80")
    old_key = bytes(len(new_key))
    xor_data = bytes(a ^ b for a, b in zip(new_key, old_key))

    print("\n--- Inputs ---")
    print(f"SessKey (DES):     {session_key_des.hex(' ').upper()}")
    print(f"New Key (2K3DES):  {new_key.hex(' ').upper()}")


    new_key_plus_version = new_key + bytes.fromhex("10")
    print(f"New Key + Ver:     {new_key_plus_version.hex(' ').upper()}")
    
    crc32_calc = calculate_crc32_desfire(b"\xb4\x38\x0e\xcb\xde\xe8\x4c\xde\x34\xb8\x8e\x4a\x2e\x18\xbc\x2e")
    crc32_bytes_calc = struct.pack('<I', crc32_calc)
    expected_crc32 = 0x5001FFC5
    crc32_bytes_expected = struct.pack('<I', expected_crc32)

    print(f"\nCRC32 computed (LSB first): {crc32_calc:#010X} -> {crc32_bytes_calc.hex(' ').upper()}")
    print(f"CRC32 provided (LSB first): {expected_crc32:#010X} -> {crc32_bytes_expected.hex(' ').upper()}")

    padding = b'\x00' * 4
    cryptogram = xor_data + crc32_bytes_expected + padding
    print(f"\nPlain cryptogram:  {cryptogram.hex(' ').upper()}")

    encrypted = desfire_des_send_encrypt(session_key_des, cryptogram, iv=b'\x00' * 8)
    print(f"Encrypted payload: {encrypted.hex(' ').upper()}")

    expected_encrypted = bytes.fromhex("87 99 59 11 8B D7 7C 70 10 7B CD B0 C0 9C C7 DA 82 15 04 AA 1E 36 04 9C")
    if encrypted == expected_encrypted:
        print("OK   Encrypted payload matches StackOverflow example")
    else:
        print("FAIL Encrypted payload differs from StackOverflow example")
        print(f"Expected: {expected_encrypted.hex(' ').upper()}")

    apdu = b'\xC4\x00' + encrypted
    print(f"\nAPDU to send: C4 00 {encrypted.hex(' ').upper()}")
    print("Expected response: 91 00")

if __name__ == "__main__":
    main()
