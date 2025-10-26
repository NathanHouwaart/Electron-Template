"""
Recreate the StackOverflow DESFire EV1 debug traces for:
  1. ISO authentication with the default 2K3DES key (key #0).
  2. ChangeKey that programs a new 2K3DES key while authenticated with the default DES key.

All intermediate values from the public trace are reproduced so the output can be
compared byte-for-byte with the reference.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Iterable, Tuple

from Crypto.Cipher import DES
import binascii


def _chunks(data: bytes, block_size: int = 8) -> Iterable[bytes]:
    for idx in range(0, len(data), block_size):
        yield data[idx : idx + block_size]


def _format_hex(data: bytes) -> str:
    return data.hex(" ").upper()


def _rotate_left(block: bytes) -> bytes:
    return block[1:] + block[:1]


def _split_3des_keys(key: bytes) -> Tuple[bytes, bytes, bytes]:
    if len(key) == 16:
        return key[:8], key[8:], key[:8]
    if len(key) == 24:
        return key[:8], key[8:16], key[16:]
    raise ValueError("3DES key must be 16 or 24 bytes long")


def _triple_des_encrypt_block(key: bytes, block: bytes) -> bytes:
    k1, k2, k3 = _split_3des_keys(key)
    stage1 = DES.new(k1, DES.MODE_ECB).encrypt(block)
    stage2 = DES.new(k2, DES.MODE_ECB).decrypt(stage1)
    return DES.new(k3, DES.MODE_ECB).encrypt(stage2)


def _triple_des_decrypt_block(key: bytes, block: bytes) -> bytes:
    k1, k2, k3 = _split_3des_keys(key)
    stage1 = DES.new(k3, DES.MODE_ECB).decrypt(block)
    stage2 = DES.new(k2, DES.MODE_ECB).encrypt(stage1)
    return DES.new(k1, DES.MODE_ECB).decrypt(stage2)


def encrypt_send_mode(session_key: bytes, data: bytes, iv: bytes) -> bytes:
    """
    DESFire CBC send mode (encrypt) for DES / 2K3DES / 3K3DES session keys.
    """
    if len(data) % 8:
        raise ValueError("Data must be a multiple of 8 bytes")
    if len(iv) != 8:
        raise ValueError("IV must be 8 bytes")

    if len(session_key) == 8:
        cipher = DES.new(session_key, DES.MODE_CBC, iv=iv)
        return cipher.encrypt(data)

    if len(session_key) in (16, 24):
        result = b""
        current_iv = iv
        for block in _chunks(data):
            xored = bytes(a ^ b for a, b in zip(block, current_iv))
            out = _triple_des_encrypt_block(session_key, xored)
            result += out
            current_iv = out
        return result

    raise ValueError("Unsupported session key length for DESFire send mode")


def calc_crc32_desfire(*chunks: bytes) -> int:
    """
    Replicate Utils::CalcCrc32 from the reference firmware.

    The routine accumulates CRC32 over the provided chunks using the reflected
    polynomial (0xEDB88320) and returns the internal register (bitwise NOT of
    the standard CRC-32 value).
    """
    crc = 0
    for chunk in chunks:
        if not chunk:
            continue
        crc = binascii.crc32(chunk, crc)
    crc &= 0xFFFFFFFF
    return (~crc) & 0xFFFFFFFF


def build_change_key_payload(
    session_key: bytes,
    key_number: int,
    new_key: bytes,
    *,
    current_key: bytes | None = None,
    same_key: bool = False,
    iv: bytes | None = None,
) -> tuple[bytes, int, int | None, bytes, bytes]:
    """
    Construct the cleartext cryptogram and encrypted payload for a ChangeKey command.
    """
    if current_key is None:
        current_key = bytes(len(new_key))
    if len(current_key) != len(new_key):
        raise ValueError("Current key must match new key length")

    command = bytes([0xC4, key_number])
    xor_key = bytes(a ^ b for a, b in zip(new_key, current_key))

    crc_crypto = calc_crc32_desfire(command, xor_key)
    cryptogram = xor_key + crc_crypto.to_bytes(4, "little")

    crc_new_key = None
    if not same_key:
        crc_new_key = calc_crc32_desfire(new_key)
        cryptogram += crc_new_key.to_bytes(4, "little")

    padded_len = ((len(cryptogram) + 7) // 8) * 8
    plaintext = cryptogram.ljust(padded_len, b"\x00")

    iv_bytes = iv if iv is not None else b"\x00" * 8
    encrypted = encrypt_send_mode(session_key, plaintext, iv_bytes)

    return xor_key, crc_crypto, crc_new_key, plaintext, encrypted


@dataclass
class IsoAuthState:
    """Maintain CBC IV state across alternating send/receive phases."""

    key: bytes
    iv: bytes = b"\x00" * 8

    def receive_plaintext(self, ciphertext: bytes) -> bytes:
        """PCD receive flow (CBC RECEIVE / KEY_DECIPHER)."""
        plaintext_blocks: list[bytes] = []
        for block in _chunks(ciphertext):
            decrypted = _triple_des_decrypt_block(self.key, block)
            plain = bytes(a ^ b for a, b in zip(decrypted, self.iv))
            plaintext_blocks.append(plain)
            self.iv = block  # next IV is the ciphertext we just processed
        return b"".join(plaintext_blocks)

    def send_ciphertext(self, plaintext: bytes) -> bytes:
        """PCD send flow (CBC SEND / KEY_ENCIPHER)."""
        if len(plaintext) % 8:
            raise ValueError("Plaintext length must be a multiple of 8 bytes")
        ciphertext_blocks: list[bytes] = []
        for block in _chunks(plaintext):
            xored = bytes(a ^ b for a, b in zip(block, self.iv))
            encrypted = _triple_des_encrypt_block(self.key, xored)
            ciphertext_blocks.append(encrypted)
            self.iv = encrypted  # next IV is the ciphertext we just produced
        return b"".join(ciphertext_blocks)


def iso_authentication_example() -> None:
    """Simulate the ISO authentication trace from the StackOverflow post."""
    print("=" * 70)
    print("ISO AUTHENTICATION WITH 2K3DES (DEFAULT KEY #0)")
    print("=" * 70)

    auth_key = bytes.fromhex("00" * 16)
    state = IsoAuthState(auth_key)

    rndb_enc = bytes.fromhex("B8 90 04 7F 2D C8 D6 8B")
    rndb = state.receive_plaintext(rndb_enc)

    rndb_rot = _rotate_left(rndb)
    rnda = bytes.fromhex("92 31 34 8B 66 35 A8 AF")
    rndab = rnda + rndb_rot
    rndab_enc = state.send_ciphertext(rndab)

    rnda_enc = bytes.fromhex("B7 96 DD 3F 81 15 45 F3")
    rnda_dec = state.receive_plaintext(rnda_enc)
    rnda_rot = _rotate_left(rnda_dec)

    session_key_calc = rnda[:4] + rndb[:4] + rnda[4:8] + rndb[4:8]
    first_half = session_key_calc[:8]
    session_key_trace = bytes(b & 0xFE for b in first_half)
    session_key_trace = session_key_trace + session_key_trace

    print(f"* RndB_enc:   {_format_hex(rndb_enc)}")
    print(f"* RndB:       {_format_hex(rndb)}")
    print(f"* RndB_rot:   {_format_hex(rndb_rot)}")
    print(f"* RndA:       {_format_hex(rnda)}")
    print(f"* RndAB:      {_format_hex(rndab)}")
    print(f"* RndAB_enc:  {_format_hex(rndab_enc)}")
    print(f"* RndA_enc:   {_format_hex(rnda_enc)}")
    print(f"* RndA_dec:   {_format_hex(rnda_dec)}")
    print(f"* RndA_rot:   {_format_hex(rnda_rot)}")
    print(f"* SessKey:    {_format_hex(session_key_trace)} (DES, reference)")
    print(f"             {_format_hex(session_key_calc)} (calculated raw)")

    # Sanity-check against known values from the trace.
    assert rndb == bytes.fromhex("74 B8 43 5F CB A0 B6 75")
    assert rndab_enc == bytes.fromhex("7C 84 6A 50 7B 9B 6E 68 64 BC 33 72 A3 06 A8 C1")
    assert rnda_dec == bytes.fromhex("31 34 8B 66 35 A8 AF 92")
    assert session_key_trace == bytes.fromhex(
        "92 30 34 8A 74 B8 42 5E 92 30 34 8A 74 B8 42 5E"
    )


def change_key_example() -> None:
    """Reproduce the ChangeKey example (default DES session changing key #0)."""
    print("\n" + "=" * 70)
    print("CHANGE 2K3DES DEFAULT KEY #0 (DES SESSION)")
    print("=" * 70)

    session_key = bytes.fromhex("B4 28 2E FA 9E B8 2C AE")
    new_key = bytes.fromhex("00 10 20 31 40 50 60 70 80 90 A0 B0 B0 A0 90 80")

    xor_key, crc_crypto, _, plaintext, encrypted = build_change_key_payload(
        session_key, 0x00, new_key, same_key=True
    )

    display_sess_key = session_key + session_key

    print(f"* SessKey:    {_format_hex(display_sess_key)} (DES)")
    print(f"* SessKey IV: 00 00 00 00 00 00 00 00")
    print(f"* New Key:    {_format_hex(new_key)} (2K3DES)")
    print(f"* CRC Crypto: 0x{crc_crypto:08X}")
    print(f"* Cryptogram: {_format_hex(plaintext)}")
    print(f"* CryptogrEnc: {_format_hex(encrypted)}")

    assert encrypted == bytes.fromhex(
        "87 99 59 11 8B D7 7C 70 10 7B CD B0 C0 9C C7 DA 82 15 04 AA 1E 36 04 9C"
    )
    assert xor_key == new_key


def change_key_example_key1() -> None:
    """Change application key #1 (DES session -> 2K3DES) including current key XOR."""
    print("\n" + "=" * 70)
    print("CHANGE 2K3DES DEFAULT KEY #1 (DES SESSION)")
    print("=" * 70)

    session_key = bytes.fromhex("9C 70 56 82 5C 08 9E C8")
    new_key = bytes.fromhex("00 10 20 31 40 50 60 70 80 90 A0 B0 B0 A0 90 80")
    current_key = bytes.fromhex("00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")

    xor_key, crc_crypto, crc_new_key, plaintext, encrypted = build_change_key_payload(
        session_key, 0x01, new_key, current_key=current_key, same_key=False
    )

    display_sess_key = session_key + session_key

    print(f"* SessKey:    {_format_hex(display_sess_key)} (DES)")
    print(f"* SessKey IV: 00 00 00 00 00 00 00 00")
    print(f"* New Key:    {_format_hex(new_key)} (2K3DES)")
    print(f"* Cur Key:    {_format_hex(current_key)} (DES)")
    print(f"* XOR(New,Cur): {_format_hex(xor_key)}")
    print(f"* CRC Crypto: 0x{crc_crypto:08X}")
    if crc_new_key is not None:
        print(f"* CRC New Key: 0x{crc_new_key:08X}")
    print(f"* Cryptogram: {_format_hex(plaintext)}")
    print(f"* CryptogrEnc: {_format_hex(encrypted)}")

    assert encrypted == bytes.fromhex(
        "7D 83 D3 4E FB 6C 84 98 48 E2 D6 37 AD A2 D0 87 14 36 1A E6 C4 63 14 52"
    )
    assert crc_crypto == 0xD7A73486
    assert crc_new_key == 0xC4EF3A3A


def main() -> None:
    iso_authentication_example()
    change_key_example()
    change_key_example_key1()


if __name__ == "__main__":
    main()
