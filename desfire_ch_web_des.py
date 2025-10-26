import binascii
import struct

new_key = bytes.fromhex("00 10 20 31 40 50 60 70 80 90 A0 B0 B0 A0 90 80")
# If old_key isn't all zeros, compute xor_key = bytes(a ^ b for a,b in zip(new_key, old_key))
xor_key = new_key

crc32_val = binascii.crc32(xor_key) & 0xFFFFFFFF
crc32_le = struct.pack('<I', crc32_val)  # little-endian 4 bytes

print(f"CRC32 value: 0x{crc32_val:08X}")
print("CRC32 LSB-first bytes:", crc32_le.hex(' ').upper())