# CBC Decryption with Zero IV - Complete Example

## Input Ciphertext (24 bytes total)
```
Block 0: 0x48 0x65 0x6C 0x6C 0x6F 0x21 0x21 0x21
Block 1: 0x54 0x68 0x69 0x73 0x49 0x73 0x41 0x42
Block 2: 0x43 0x44 0x45 0x46 0x47 0x48 0x49 0x4A
```

---

## Complete CBC Decryption Flow

```
Block 0 (bytes 0-7):
┌─────────────────────────┐
│ Ciphertext₀             │
│ 0x48656C6C6F212121      │
│ (8 bytes)               │
└────────┬────────────────┘
         │
         ⊕ ←─────── IV = 0x0000000000000000 (8 bytes)
         │          (XOR result: 0x48656C6C6F212121)
         ▼
     ┌───────┐
     │ 3DES  │
     │Decrypt│
     └───┬───┘
         │
         ▼
┌─────────────────────────┐        ┌──────────────────────────────┐
│ Plaintext₀              │        │ last_block_B =               │──┐
│ 0x4142434445464748      │        │ 0x4142434445464748           |  |
│ (8 bytes) "ABCDEFGH"    │        | (acts as IV for block 1)     |  |
└─────────────────────────┘        └──────────────────────────────┘  | 
                                                                     │
Block 1 (bytes 8-15):                                                │
┌─────────────────────────┐                                          │
│ Ciphertext₁             │                                          │
│ 0x5468697349734142      │                                          │
│ (8 bytes)               │                                          │
└────────┬────────────────┘                                          │
         │                                                           │
         ⊕ ←────────────────────────────────────────────────────────┘
         │          IV = last_block_B = 0x4142434445464748 (8 bytes)
         │          (XOR result: 0x152A2A374D35041A)
         ▼
     ┌───────┐
     │ 3DES  │
     │Decrypt│
     └───┬───┘
         │
         ▼
┌─────────────────────────┐        ┌──────────────────────────────┐
│ Plaintext₁              │        │ last_block_B =               │──┐
│ 0x3132333435363738      │        │ 0x3132333435363738           │  │
│ (8 bytes) "12345678"    │        │ (acts as IV for block 2)     │  │
└─────────────────────────┘        └──────────────────────────────┘  │
                                                                     │
Block 2 (bytes 16-23):                                               │
┌─────────────────────────┐                                          │
│ Ciphertext₂             │                                          │
│ 0x4344454647484949A     │                                          │
│ (8 bytes)               │                                          │
└────────┬────────────────┘                                          │
         │                                                           │
         ⊕ ←────────────────────────────────────────────────────────┘
         │          IV = last_block_B = 0x3132333435363738 (8 bytes)
         │          (XOR result: 0x7276767272727272)
         ▼
     ┌───────┐
     │ 3DES  │
     │Decrypt│
     └───┬───┘
         │
         ▼
┌─────────────────────────┐
│ Plaintext₂              │
│ 0x6162636465666768      │
│ (8 bytes) "abcdefgh"    │
└─────────────────────────┘
```

---

## Final Decrypted Output (24 bytes)
```
Complete plaintext: "ABCDEFGH12345678abcdefgh"

Block 0: 0x41 0x42 0x43 0x44 0x45 0x46 0x47 0x48  → "ABCDEFGH"
Block 1: 0x31 0x32 0x33 0x34 0x35 0x36 0x37 0x38  → "12345678"
Block 2: 0x61 0x62 0x63 0x64 0x65 0x66 0x67 0x68  → "abcdefgh"
```

## Summary
- Each block is 8 bytes (64 bits)
- `last_block_B` chains each plaintext output as IV for next block
- Operation: `Plaintext = 3DES_Decrypt(Ciphertext ⊕ IV)`