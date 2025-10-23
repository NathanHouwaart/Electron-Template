#!/usr/bin/env python3
"""
DESFire ChangeKey Simulator
============================
This script simulates the DESFire ChangeKey command for all key type transitions.
It generates the encrypted payload and APDU that should be sent to the card.

Supports:
- DES (8 bytes)
- 3DES 2-key (16 bytes)
- 3DES 3-key (24 bytes)
- AES (16 bytes)

Usage:
    python desfire_changekey_simulator.py
"""

from Crypto.Cipher import DES3, AES
from Crypto.Util.Padding import pad
import struct

# ============================================================================
# DESFire Key Types
# ============================================================================

KEY_TYPES = {
    'DES': {'size': 8, 'algo_code': 0x0, 'block_size': 8, 'name': 'DES'},
    '3DES_2KEY': {'size': 16, 'algo_code': 0x1, 'block_size': 8, 'name': '3DES (2-key)'},
    '3DES_3KEY': {'size': 24, 'algo_code': 0x2, 'block_size': 8, 'name': '3DES (3-key)'},
    'AES': {'size': 16, 'algo_code': 0x3, 'block_size': 16, 'name': 'AES'},
}

# ============================================================================
# CRC16 Calculation (ISO 14443-3 Type A for DESFire)
# ============================================================================

def calculate_crc16(data):
    """
    Calculate CRC16 for DESFire (ISO 14443-3 Type A)
    Polynomial: 0x8005
    Initial value: 0x6363
    """
    crc = 0x6363
    
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x0001:
                crc = (crc >> 1) ^ 0x8005
            else:
                crc = crc >> 1
    
    return crc & 0xFFFF

# ============================================================================
# Session Key Derivation
# ============================================================================

def derive_session_key_3des(rnd_a, rnd_b):
    """
    Derive 3DES session key from RndA and RndB
    Formula: RndA[0..3] || RndB[0..3] || RndA[4..7] || RndB[4..7]
    """
    if len(rnd_a) != 8 or len(rnd_b) != 8:
        raise ValueError("RndA and RndB must be 8 bytes for 3DES")
    
    session_key = bytearray(16)
    session_key[0:4] = rnd_a[0:4]
    session_key[4:8] = rnd_b[0:4]
    session_key[8:12] = rnd_a[4:8]
    session_key[12:16] = rnd_b[4:8]
    
    return bytes(session_key)

def derive_session_key_aes(rnd_a, rnd_b):
    """
    Derive AES session key from RndA and RndB
    Formula: RndA[0..3] || RndB[0..3] || RndA[12..15] || RndB[12..15]
    """
    if len(rnd_a) != 16 or len(rnd_b) != 16:
        raise ValueError("RndA and RndB must be 16 bytes for AES")
    
    session_key = bytearray(16)
    session_key[0:4] = rnd_a[0:4]
    session_key[4:8] = rnd_b[0:4]
    session_key[8:12] = rnd_a[12:16]
    session_key[12:16] = rnd_b[12:16]
    
    return bytes(session_key)

# ============================================================================
# Encryption Functions
# ============================================================================

def encrypt_3des_cbc(data, key, iv=None):
    """
    Encrypt data using 3DES-CBC
    
    Note: pycryptodome rejects weak keys where all sub-keys are identical
    (e.g., all-zero session key). This is a security feature.
    """
    if iv is None:
        iv = b'\x00' * 8
    
    # For 2-key 3DES, expand to 3-key by repeating first 8 bytes
    if len(key) == 16:
        key = key + key[0:8]
    
    # Check for weak key (all sub-keys identical)
    k1, k2, k3 = key[0:8], key[8:16], key[16:24]
    if k1 == k2 == k3:
        raise ValueError(
            f"Weak 3DES key detected (all sub-keys identical): {key.hex()}\n"
            "This would degenerate to single DES. Use non-zero RndA/RndB values.\n"
            "Example: RndA=12345678 9ABCDEF0, RndB=FEDCBA98 76543210"
        )
    
    cipher = DES3.new(key, DES3.MODE_CBC, iv)
    return cipher.encrypt(data)

def encrypt_aes_cbc(data, key, iv=None):
    """
    Encrypt data using AES-CBC
    """
    if iv is None:
        iv = b'\x00' * 16
    
    cipher = AES.new(key, AES.MODE_CBC, iv)
    return cipher.encrypt(data)

# ============================================================================
# ChangeKey Payload Builder
# ============================================================================

def build_changekey_payload(old_key_type, new_key_type, old_key, new_key, key_no=0):
    """
    Build the complete ChangeKey payload
    
    Args:
        old_key_type: String - 'DES', '3DES_2KEY', '3DES_3KEY', 'AES'
        new_key_type: String - 'DES', '3DES_2KEY', '3DES_3KEY', 'AES'
        old_key: bytes - Current key (length depends on old_key_type)
        new_key: bytes - New key to install (length depends on new_key_type)
        key_no: int - Key slot number (0-13)
    
    Returns:
        dict with 'plaintext', 'padded_plaintext', 'crc16', 'xor_data'
    """
    old_info = KEY_TYPES[old_key_type]
    new_info = KEY_TYPES[new_key_type]
    
    # Validate key lengths
    if len(old_key) != old_info['size']:
        raise ValueError(f"Old key must be {old_info['size']} bytes for {old_key_type}")
    if len(new_key) != new_info['size']:
        raise ValueError(f"New key must be {new_info['size']} bytes for {new_key_type}")
    
    # Step 1: XOR new key with old key
    # Pad keys to same length if needed
    max_len = max(len(old_key), len(new_key))
    old_key_padded = old_key + b'\x00' * (max_len - len(old_key))
    new_key_padded = new_key + b'\x00' * (max_len - len(new_key))
    
    xor_data = bytes(a ^ b for a, b in zip(new_key_padded[:new_info['size']], 
                                            old_key_padded[:new_info['size']]))
    
    # Step 2: Calculate CRC16
    crc_input = bytes([0xC4, key_no]) + xor_data
    crc16 = calculate_crc16(crc_input)
    crc_bytes = struct.pack('<H', crc16)  # Little-endian
    
    # Step 3: Build plaintext payload
    plaintext = xor_data + crc_bytes
    
    # Step 4: Pad to block size (depends on current key type for encryption)
    block_size = old_info['block_size']
    padded_size = ((len(plaintext) + block_size - 1) // block_size) * block_size
    padded_plaintext = plaintext + b'\x00' * (padded_size - len(plaintext))
    
    return {
        'plaintext': plaintext,
        'padded_plaintext': padded_plaintext,
        'crc16': crc16,
        'xor_data': xor_data,
        'block_size': block_size,
    }

def encrypt_changekey_payload(payload_data, old_key_type, session_key):
    """
    Encrypt the ChangeKey payload with session key
    
    Args:
        payload_data: dict from build_changekey_payload()
        old_key_type: String - current key type (determines encryption algorithm)
        session_key: bytes - derived session key
    
    Returns:
        bytes - encrypted payload
    """
    old_info = KEY_TYPES[old_key_type]
    padded_plaintext = payload_data['padded_plaintext']
    
    if old_key_type in ['DES', '3DES_2KEY', '3DES_3KEY']:
        return encrypt_3des_cbc(padded_plaintext, session_key)
    elif old_key_type == 'AES':
        return encrypt_aes_cbc(padded_plaintext, session_key)
    else:
        raise ValueError(f"Unknown key type: {old_key_type}")

def build_changekey_apdu(encrypted_payload, new_key_type, key_no=0, key_revision=0):
    """
    Build complete ChangeKey APDU
    
    Args:
        encrypted_payload: bytes - encrypted payload
        new_key_type: String - type of new key
        key_no: int - key slot number
        key_revision: int - key revision (0-15)
    
    Returns:
        bytes - complete APDU
    """
    new_info = KEY_TYPES[new_key_type]
    
    # Build keyVersion byte (high nibble = algo, low nibble = revision)
    key_version = ((new_info['algo_code'] & 0x0F) << 4) | (key_revision & 0x0F)
    
    # Build APDU: CLA INS P1 P2 Lc KeyNo EncryptedData KeyVersion Le
    apdu = bytearray()
    apdu.append(0x90)  # CLA
    apdu.append(0xC4)  # INS = ChangeKey
    apdu.append(0x00)  # P1
    apdu.append(0x00)  # P2
    apdu.append(len(encrypted_payload) + 2)  # Lc = encrypted + keyNo + keyVersion
    apdu.append(key_no)  # Key number
    apdu.extend(encrypted_payload)  # Encrypted payload
    apdu.append(key_version)  # Key version
    apdu.append(0x00)  # Le
    
    return bytes(apdu)

# ============================================================================
# Main Simulation Function
# ============================================================================

def simulate_changekey(old_key_type, new_key_type, old_key, new_key, 
                       rnd_a, rnd_b, key_no=0, key_revision=0):
    """
    Complete simulation of ChangeKey process
    
    Args:
        old_key_type: String - current key type
        new_key_type: String - new key type
        old_key: bytes - current key
        new_key: bytes - new key to install
        rnd_a: bytes - RndA from authentication (8 or 16 bytes)
        rnd_b: bytes - RndB from authentication (8 or 16 bytes)
        key_no: int - key slot number
        key_revision: int - key revision number
    
    Returns:
        dict with all intermediate values and final APDU
    """
    print("=" * 80)
    print("DESFire ChangeKey Simulation")
    print("=" * 80)
    
    print(f"\nKey Transition: {old_key_type} → {new_key_type}")
    print(f"Key Slot: {key_no}")
    print(f"Key Revision: {key_revision}")
    
    # Step 1: Derive session key
    print("\n--- Step 1: Derive Session Key ---")
    if old_key_type in ['DES', '3DES_2KEY', '3DES_3KEY']:
        session_key = derive_session_key_3des(rnd_a, rnd_b)
        print(f"Using 3DES session key derivation")
    else:  # AES
        session_key = derive_session_key_aes(rnd_a, rnd_b)
        print(f"Using AES session key derivation")
    
    print(f"RndA: {rnd_a.hex(' ')}")
    print(f"RndB: {rnd_b.hex(' ')}")
    print(f"Session Key: {session_key.hex(' ')}")
    
    # Step 2: Build payload
    print("\n--- Step 2: Build Payload ---")
    payload_data = build_changekey_payload(old_key_type, new_key_type, 
                                           old_key, new_key, key_no)
    
    print(f"Old Key ({old_key_type}): {old_key.hex(' ')}")
    print(f"New Key ({new_key_type}): {new_key.hex(' ')}")
    print(f"XOR Data: {payload_data['xor_data'].hex(' ')}")
    print(f"CRC16: 0x{payload_data['crc16']:04x}")
    print(f"Plaintext ({len(payload_data['plaintext'])} bytes): {payload_data['plaintext'].hex(' ')}")
    print(f"Padded ({len(payload_data['padded_plaintext'])} bytes): {payload_data['padded_plaintext'].hex(' ')}")
    
    # Step 3: Encrypt payload
    print("\n--- Step 3: Encrypt Payload ---")
    encrypted = encrypt_changekey_payload(payload_data, old_key_type, session_key)
    print(f"Encrypted ({len(encrypted)} bytes): {encrypted.hex(' ')}")
    
    # Step 4: Build APDU
    print("\n--- Step 4: Build APDU ---")
    apdu = build_changekey_apdu(encrypted, new_key_type, key_no, key_revision)
    
    new_info = KEY_TYPES[new_key_type]
    key_version = ((new_info['algo_code'] & 0x0F) << 4) | (key_revision & 0x0F)
    
    print(f"APDU ({len(apdu)} bytes):")
    print(f"  {apdu.hex(' ')}")
    print(f"\nAPDU Breakdown:")
    print(f"  CLA:       0x{apdu[0]:02x}")
    print(f"  INS:       0x{apdu[1]:02x} (ChangeKey)")
    print(f"  P1:        0x{apdu[2]:02x}")
    print(f"  P2:        0x{apdu[3]:02x}")
    print(f"  Lc:        0x{apdu[4]:02x} ({apdu[4]} bytes)")
    print(f"  KeyNo:     0x{apdu[5]:02x}")
    print(f"  Encrypted: {encrypted.hex(' ')}")
    print(f"  KeyVer:    0x{key_version:02x} (algo={new_info['algo_code']}, rev={key_revision})")
    print(f"  Le:        0x{apdu[-1]:02x}")
    
    return {
        'session_key': session_key,
        'payload_data': payload_data,
        'encrypted': encrypted,
        'apdu': apdu,
    }

# ============================================================================
# Interactive Menu
# ============================================================================

def interactive_menu():
    """Interactive menu for testing different key transitions"""
    
    while True:
        print("\n" + "=" * 80)
        print("DESFire ChangeKey Simulator - Interactive Mode")
        print("=" * 80)
        
        # Choose old key type
        print("\nCurrent Key Type:")
        for i, kt in enumerate(KEY_TYPES.keys(), 1):
            print(f"  {i}. {KEY_TYPES[kt]['name']} ({KEY_TYPES[kt]['size']} bytes)")
        
        try:
            old_choice = int(input("\nSelect current key type (1-4): "))
            old_key_type = list(KEY_TYPES.keys())[old_choice - 1]
        except (ValueError, IndexError):
            print("Invalid choice!")
            continue
        
        # Choose new key type
        print("\nNew Key Type:")
        for i, kt in enumerate(KEY_TYPES.keys(), 1):
            print(f"  {i}. {KEY_TYPES[kt]['name']} ({KEY_TYPES[kt]['size']} bytes)")
        
        try:
            new_choice = int(input("\nSelect new key type (1-4): "))
            new_key_type = list(KEY_TYPES.keys())[new_choice - 1]
        except (ValueError, IndexError):
            print("Invalid choice!")
            continue
        
        # Use all-zero keys?
        use_zeros = input("\nUse all-zero keys? (y/n): ").lower() == 'y'
        
        old_size = KEY_TYPES[old_key_type]['size']
        new_size = KEY_TYPES[new_key_type]['size']
        
        if use_zeros:
            old_key = b'\x00' * old_size
            new_key = b'\x00' * new_size
        else:
            print(f"\nEnter old key ({old_size} bytes, hex): ", end='')
            old_key = bytes.fromhex(input().replace(' ', ''))
            print(f"Enter new key ({new_size} bytes, hex): ", end='')
            new_key = bytes.fromhex(input().replace(' ', ''))
        
        # RndA and RndB
        rnd_size = 8 if old_key_type in ['DES', '3DES_2KEY', '3DES_3KEY'] else 16
        
        use_zero_rnds = input(f"\nUse all-zero RndA/RndB ({rnd_size} bytes each)? (y/n): ").lower() == 'y'
        
        if use_zero_rnds:
            rnd_a = b'\x00' * rnd_size
            rnd_b = b'\x00' * rnd_size
        else:
            print(f"\nEnter RndA ({rnd_size} bytes, hex): ", end='')
            rnd_a = bytes.fromhex(input().replace(' ', ''))
            print(f"Enter RndB ({rnd_size} bytes, hex): ", end='')
            rnd_b = bytes.fromhex(input().replace(' ', ''))
        
        # Key number
        key_no = int(input("\nKey slot number (0-13, default 0): ") or "0")
        
        # Run simulation
        result = simulate_changekey(old_key_type, new_key_type, old_key, new_key,
                                    rnd_a, rnd_b, key_no=key_no)
        
        print("\n" + "=" * 80)
        print("✅ Simulation Complete!")
        print("=" * 80)
        
        # Continue?
        if input("\nRun another simulation? (y/n): ").lower() != 'y':
            break

# ============================================================================
# Main Entry Point
# ============================================================================

if __name__ == "__main__":
    import sys
    
    # Check if pycryptodome is installed
    try:
        from Crypto.Cipher import DES3, AES
    except ImportError:
        print("Error: pycryptodome is not installed!")
        print("Install it with: pip install pycryptodome")
        sys.exit(1)
    
    # Example: Test with realistic random values (3DES → AES)
    if len(sys.argv) > 1 and sys.argv[1] == '--example':
        print("Running example: 3DES_2KEY → AES (factory default → new AES key)")
        simulate_changekey(
            old_key_type='3DES_2KEY',
            new_key_type='AES',
            old_key=b'\x00' * 16,  # Factory default
            new_key=b'\x00' * 16,  # New key (all-zero for demo)
            rnd_a=b'\xf0\x6d\xf8\xe4\xed\x3f\xf3\xd0',  # Example RndA from authentication
            rnd_b=b'\x59\x95\xe1\x7b\x55\x46\xcf\x8f',  # Example RndB from authentication
            key_no=0,
            key_revision=0
        )
    else:
        # Interactive mode
        interactive_menu()

# 0x49 0xba 0xc3 0xc2 0x4d 0xb7 0xf0 0x10 0x4d 0xb0 0xc4 0xa7 0xb7 0x47 0x55 0xbd 0xb2 0x16 0x52 0xef 0x42 0xc9 0x08 0xab
# 0x49 0xba 0xc3 0xc2 0x4d 0xb7 0xf0 0x10 0x4d 0xb0 0xc4 0xa7 0xb7 0x47 0x55 0xbd 0xb2 0x16 0x52 0xef 0x42 0xc9 0x08 0xab