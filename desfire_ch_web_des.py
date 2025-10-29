from Crypto.Cipher import DES3
from Crypto.Util.Padding import pad, unpad

CBC_SEND = 0
CBC_RECEIVE = 1
KEY_ENCIPHER = 0
KEY_DECIPHER = 1


def xor_bytes(a: bytes, b: bytes) -> bytes:
    """XOR two byte strings of equal length."""
    return bytes(x ^ y for x, y in zip(a, b))


def CryptDataBlock(data_block: bytes, key: bytes, e_cipher: int) -> bytes:
    """Implements single-block 3DES ECB encrypt/decrypt (DESFire-style)."""
    cipher = DES3.new(key, DES3.MODE_ECB)
    if e_cipher == KEY_ENCIPHER:
        return cipher.encrypt(data_block)
    else:
        return cipher.decrypt(data_block)


def CryptDataCBC(e_CBC: int, e_Cipher: int, key: bytes, iv: bytes, data: bytes) -> tuple[bytes, bytes]:
    """
    NXP DESFire-style CBC encryption/decryption.
    Returns (output, updated_iv).
    """
    block_size = 8
    if len(data) % block_size != 0 or len(data) < block_size:
        raise ValueError("Invalid CBC block size")

    output = b""
    iv_curr = iv

    for i in range(0, len(data), block_size):
        block = data[i:i + block_size]

        if e_CBC == CBC_SEND:
            # Encrypting direction (TX)
            temp = xor_bytes(block, iv_curr)
            out_block = CryptDataBlock(temp, key, e_Cipher)
            iv_curr = out_block  # Update IV = ciphertext
        else:
            # Receiving direction (RX)
            temp = CryptDataBlock(block, key, e_Cipher)
            out_block = xor_bytes(temp, iv_curr)
            iv_curr = block  # Update IV = input ciphertext

        output += out_block

    return output, iv_curr


from Crypto.Cipher import DES
from Crypto.Util.Padding import pad, unpad

def CryptDataBlockDES(data_block: bytes, key: bytes, e_cipher: int) -> bytes:
    """Single-block DES ECB encrypt/decrypt (NXP-style)."""
    cipher = DES.new(key, DES.MODE_ECB)
    if e_cipher == KEY_ENCIPHER:
        return cipher.encrypt(data_block)
    else:
        return cipher.decrypt(data_block)

def CryptDataCBC_DES(e_CBC: int, e_Cipher: int, key: bytes, iv: bytes, data: bytes) -> tuple[bytes, bytes]:
    """NXP DESFire-style CBC using single DES."""
    block_size = 8
    if len(data) % block_size != 0 or len(data) < block_size:
        raise ValueError("Invalid CBC block size")

    output = b""
    iv_curr = iv

    for i in range(0, len(data), block_size):
        block = data[i:i + block_size]

        if e_CBC == CBC_SEND:
            temp = xor_bytes(block, iv_curr)
            out_block = CryptDataBlockDES(temp, key, e_Cipher)
            iv_curr = out_block
        else:
            temp = CryptDataBlockDES(block, key, e_Cipher)
            out_block = xor_bytes(temp, iv_curr)
            iv_curr = block

        output += out_block

    return output, iv_curr

# Use only first 8 bytes of your original key for single DES
key = bytes.fromhex("48EC62DE3A3CFE62")  # 8 bytes for DES
iv = bytes.fromhex("0000000000000000")  # Initial IV
data = bytes.fromhex("CAC6B3CADF0A63D1")

encrypted, new_iv = CryptDataCBC_DES(
    e_CBC=CBC_SEND,
    e_Cipher=KEY_ENCIPHER,
    key=key,
    iv=iv,
    data=data
)

print("Output :", encrypted.hex().upper())
print("New IV :", new_iv.hex().upper())


# # Example: DESFire TX CMAC test
# key = bytes.fromhex("48EC62DE3A3CFE6248EC62DE3A3CFE62")   # Session key
# iv = bytes.fromhex("8476D1CF3024B7C7")                    # RndB_enc
# data = bytes.fromhex("CAC6B3CADF0A63D1")                  # CMAC input block

# # Encrypt in NXP CBC_SEND mode
# encrypted, new_iv = CryptDataCBC(
#     e_CBC=CBC_SEND,
#     e_Cipher=KEY_ENCIPHER,
#     key=key,
#     iv=iv,
#     data=data
# )

# print("Output :", encrypted.hex().upper())
# print("New IV :", new_iv.hex().upper())
