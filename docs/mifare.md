# MIFARE support

The MFRC522 is a **transparent** frontend: it implements the ISO/IEC 14443-A
air interface (framing, parity, CRC) and provides **one** MIFARE-specific
hardware feature — the Crypto1 authentication command (`MFAuthent`).
Everything else in this layer (block/sector/value semantics) is *protocol*,
implemented by the library on top of the transceive primitive.

> Capability boundaries (hardware vs library vs card) are summarized in
> [architecture.md](architecture.md) §13.

## What is actually supported

| Feature | Where it lives | Notes |
|---------|----------------|-------|
| Crypto1 authentication (Key A / Key B) | MFRC522 hardware (`MFAuthent`) | 6-byte key + last 4 UID bytes |
| 16-byte block READ | library (0x30) | card CRC_A is validated |
| 16-byte block WRITE | library (0xA0) | two-step, ACK/NAK checked |
| Sector read/write | library | never touches the trailer block |
| Value block Increment/Decrement/Restore/Transfer | library (0xC0/0xC1/0xC2/0xB0) | two-step protocol |
| Value-block formatting helper | library | pure function |
| MIFARE Ultralight / NTAG read | library (same 0x30) | 16-byte page reads |

**Not supported** (outside the MFRC522's scope, or not implemented):
- MIFARE DESFire / Plus / ISO 14443-4 (T=CL) command sets.
- NTAG password authentication / ECC.
- 7-byte/10-byte UID *authentication* (the `MF1Sxxx` shortcut activation with
  cascade-tag bytes); the library authenticates with the last 4 UID bytes,
  which covers MIFARE Classic 1K/4K/Mini (all 4-byte-UID cards).

## Card geometry (MIFARE Classic)

| Card | Sectors | Blocks/sector | Data blocks |
|------|--------:|--------------:|------------:|
| Mini  | 5       | 4             | 3 (320 B)   |
| 1K    | 16      | 4             | 3 (1024 B)  |
| 4K    | 40      | 4 (sectors 0-31), 16 (sectors 32-39) | 3 / 15 |

Sector 0 block 0 is the manufacturer block (read-only UID etc.). The last
block of every sector is the **sector trailer** (key A + access bits + key B)
and is never written by `MFRC522_WriteSector`.

## Authentication

```c
MFRC522_Key_t key = { { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } }; /* factory */
MFRC522_Status_t s = MFRC522_Authenticate(&rfid, MFRC522_KEY_A, &key,
                                          block, uid.bytes, uid.length);
if (s == MFRC522_OK) { /* sector authenticated */ }
```

- A wrong key surfaces as `MFRC522_ERR_AUTH` (the MFAuthent command times
  out — the card stays silent).
- After communicating with an authenticated card, call
  `MFRC522_StopCrypto1()` to drop the Crypto1 state, otherwise no new
  (unauthenticated) communication can start.

## Value blocks

A value block is a 16-byte block with the following layout (built by
`MFRC522_FormatValueBlock`):

```
byte 0..3   : value (signed 32-bit, little-endian)
byte 4..7   : ~value (inverted)
byte 8..11  : value (redundant copy)
byte 12     : address (one byte)
byte 13..15 : ~address (inverted), repeated
```

`Increment`/`Decrement`/`Restore` write the result to a *volatile* buffer on
the card; `Transfer` commits it to the target block. A typical sequence is
`Increment(block, n)` → `Transfer(block)`.
