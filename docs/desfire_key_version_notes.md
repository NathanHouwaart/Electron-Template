# DESFire Session-Key Version Handling

This note documents how the reference *RFID Electronic Doorlock* project treats
key versions for both DES/3DES and AES keys, and why the session-key bytes you
see in debug output may differ from the raw concatenation of `RndA` and `RndB`.

## Raw Session Key Construction

During ISO authentication (`Desfire::Authenticate` in
`RFID Electronic Doorlock/DoorOpenerSketch/Desfire.cpp`) the PICC returns
`RndB`, the reader generates `RndA`, and the session key is built exactly as
described in the datasheet:

```
SessionKey_raw = RndA[0:4] + RndB[0:4] + RndA[4:8] + RndB[4:8]
```

For simple DES the first 8 bytes are used; for 2K3DES/3K3DES additional bytes
from `RndA`/`RndB` are appended. This "raw" buffer still contains whatever least
significant bits the card produced.

## DES / (2K3DES, 3K3DES) Key Version Encoding

Immediately after the raw buffer is assembled, the code loads it into a
`DES`/`DESFireKey` object:

```cpp
if (!mpi_SessionKey->SetKeyData(i_SessKey, i_SessKey.GetCount(), 0) ||
    !mpi_SessionKey->GenerateCmacSubkeys())
    return false;
```

`DES::SetKeyData` (`RFID Electronic Doorlock/DoorOpenerSketch/DES.cpp`) calls
`StoreKeyVersion`. This routine copies the key bytes and **forces the least
significant bit of each byte** so it matches the supplied key version. The
DESFire data sheet specifies that the LSB (the parity bit) is ignored by the
cipher core, so firmware stores the version there:

* Version `0x00`: all LSBs are cleared -> every byte becomes even (`byte & 0xFE`).
* Version `0x10`: only the LSB corresponding to the `0x10` bit is set, the rest
  remain zero.
* Any other version is encoded by copying its bits into the LSBs of the first
  eight key bytes.

When the key size is 8 (legacy DES), `SetKeyData` also duplicates those bytes so
the internal buffer is 16 bytes long, matching the ChangeKey logic.

**Result:** the session key still encrypts correctly (DES ignores parity), but
the debug printout from the firmware shows the parity-adjusted value, e.g.

```
raw:      92 31 34 8B 74 B8 43 5F 66 35 A8 AF CB A0 B6 75
stored:   92 30 34 8A 74 B8 42 5E 92 30 34 8A 74 B8 42 5E (version 0x00)
```

If you skipped `StoreKeyVersion`, the stored key version would effectively be
random, because it would inherit the uncontrolled LSBs from the random numbers.

## AES Key Version Encoding

AES keys do **not** have parity bits. The firmware therefore transmits the key
version explicitly. In `Desfire::ChangeKey` you will find:

```cpp
// While DES stores the key version in bit 0 of the key bytes,
// AES transmits the version separately
if (pi_NewKey->GetKeyType() == DF_KEY_AES)
{
    i_Cryptogram.AppendUint8(pi_NewKey->GetKeyVersion());
}
```

The AES key bytes themselves are not modified; the version is appended as an
extra byte when required.

## 2K3DES vs. "all-zero" Default Key

The PICC advertises the master key slot as 2K3DES even when it still contains
the factory default value. DESFire cards treat a 16-byte key where `K1 == K2`
(for example all zeroes) as a degenerate 2K3DES key that behaves like single
DES. The host code therefore always transmits 16 bytes:

* Real 2K3DES: `K1` and `K2` differ -> full 112-bit keying option.
* Default DES: `K1 == K2 == 00...00` (or any repeated pattern) -> the card
  collapses it to a simple DES key internally.

That is why the debug trace labels the default key as "2K3DES" even though it is
effectively DES—the distinction is made by the card based on whether the two
halves differ, not on the length of the buffer sent during ChangeKey.

## Summary

* Session keys are always composed from `RndA`/`RndB` as per the datasheet.
* For DES/3DES keys, the project stores the key version inside the LSB of each
  byte. That step explains why the debug output differs from the raw
  concatenation.
* For AES keys, the version is carried separately—no bits inside the key are
  modified.
