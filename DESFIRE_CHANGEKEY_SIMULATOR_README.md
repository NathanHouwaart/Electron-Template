# DESFire ChangeKey Simulator

A Python script to simulate and verify DESFire ChangeKey operations for all key type transitions.

## Features

- ✅ Supports all DESFire key types: DES, 3DES (2-key), 3DES (3-key), AES
- ✅ Simulates complete ChangeKey process with proper encryption
- ✅ CRC16 calculation (ISO 14443-3 Type A)
- ✅ Session key derivation for both 3DES and AES
- ✅ Interactive menu for testing different scenarios
- ✅ Detailed output showing all intermediate values
- ✅ APDU generation and breakdown

## Installation

Install the required dependency:

```powershell
pip install pycryptodome
```

## Usage

### Interactive Mode

Run the script without arguments to enter interactive mode:

```powershell
python desfire_changekey_simulator.py
```

You'll be prompted to:
1. Select current key type
2. Select new key type
3. Enter keys (or use all-zero keys)
4. Enter RndA/RndB from authentication
5. Specify key slot number

### Example Mode

Run a quick example with all-zero keys (3DES → AES):

```powershell
python desfire_changekey_simulator.py --example
```

## Example Output

```
================================================================================
DESFire ChangeKey Simulation
================================================================================

Key Transition: 3DES_2KEY → AES
Key Slot: 0
Key Revision: 0

--- Step 1: Derive Session Key ---
Using 3DES session key derivation
RndA: 00 00 00 00 00 00 00 00
RndB: 00 00 00 00 00 00 00 00
Session Key: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

--- Step 2: Build Payload ---
Old Key (3DES_2KEY): 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
New Key (AES): 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
XOR Data: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
CRC16: 0x8498
Plaintext (18 bytes): 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 98 84
Padded (24 bytes): 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 98 84 00 00 00 00 00 00

--- Step 3: Encrypt Payload ---
Encrypted (24 bytes): b1 33 d1 aa 77 39 7b 83 9b 3f 7c 2e 8f 1a 35 cf 3b aa 07 37 8c db 99 6f

--- Step 4: Build APDU ---
APDU (32 bytes):
  90 c4 00 00 1a 00 b1 33 d1 aa 77 39 7b 83 9b 3f 7c 2e 8f 1a 35 cf 3b aa 07 37 8c db 99 6f 30 00

APDU Breakdown:
  CLA:       0x90
  INS:       0xc4 (ChangeKey)
  P1:        0x00
  P2:        0x00
  Lc:        0x1a (26 bytes)
  KeyNo:     0x00
  Encrypted: b1 33 d1 aa 77 39 7b 83 9b 3f 7c 2e 8f 1a 35 cf 3b aa 07 37 8c db 99 6f
  KeyVer:    0x30 (algo=3, rev=0)
  Le:        0x00
```

## Use Cases

1. **Verify C++ Implementation**: Compare APDU output with your C++ code
2. **Test Key Transitions**: Try all possible key type combinations
3. **Debug Encryption Issues**: See intermediate values (XOR, CRC, padding, encryption)
4. **Generate Test Vectors**: Create known-good APDUs for unit tests

## Supported Key Transitions

| From ↓ | To → | DES | 3DES (2-key) | 3DES (3-key) | AES |
|--------|------|-----|--------------|--------------|-----|
| **DES** | | ✅ | ✅ | ✅ | ✅ |
| **3DES (2-key)** | | ✅ | ✅ | ✅ | ✅ |
| **3DES (3-key)** | | ✅ | ✅ | ✅ | ✅ |
| **AES** | | ✅ | ✅ | ✅ | ✅ |

## Technical Details

### CRC16 Calculation
- Polynomial: 0x8005
- Initial value: 0x6363
- Used for payload integrity

### Session Key Derivation

**3DES Mode:**
```
SessionKey = RndA[0..3] || RndB[0..3] || RndA[4..7] || RndB[4..7]
```

**AES Mode:**
```
SessionKey = RndA[0..3] || RndB[0..3] || RndA[12..15] || RndB[12..15]
```

### Payload Structure

```
[XOR Data] [CRC16 LSB] [CRC16 MSB] [Padding]
    ↓           ↓          ↓            ↓
  N bytes    1 byte    1 byte    0-15 bytes
```

Padding to block size:
- 3DES: 8 bytes
- AES: 16 bytes

## Notes

- RndA and RndB come from the authentication phase
- The encryption algorithm is determined by the **current** key type (before change)
- The keyVersion byte specifies the **new** key type and revision
- Session becomes invalid after ChangeKey - must re-authenticate

## License

MIT
