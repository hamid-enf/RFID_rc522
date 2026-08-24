# Supported / Partially supported / Not supported features

This page draws a hard line between what the **MFRC522 silicon**, the **card
(MIFARE / ISO 14443-A PICC)**, the **protocol** and this **library** actually
support. Claims here follow the NXP MFRC522 data sheet (Rev. 3.9) and the
ISO/IEC 14443-3 / MIFARE specifications — not Arduino-library folklore.

---

## Host interfaces

| Interface | Support | Notes |
|-----------|---------|-------|
| SPI       | ✅ Full | Recommended. Up to 10 Mbit/s; mode 0, MSB first. |
| I2C       | ✅ Full | Fast mode (400 kbit/s) + HS mode; 7-bit address (default 0x28). |
| UART      | ✅ Full (with caveats) | Logic-level 8N1 **LSB-first**; DTRQ optional. See `docs/uart.md`. |

All three transports are implemented and host-tested against a mocked device;
SPI is the best-tested path on real hardware.

---

## ISO/IEC 14443-A protocol

| Feature | Support | Notes |
|---------|---------|-------|
| REQA / WUPA (7-bit short frame) | ✅ | ATQA validated (16 bits). |
| Anti-collision (SEL 0x93/0x95/0x97) | ✅ | Cascade levels 1–3, collision position from CollReg. |
| SELECT (NVB + BCC + CRC_A) | ✅ | SAK validated. |
| Cascade tags (CT) for 7/10-byte UID | ✅ | 4/7/10-byte UID resolution. |
| HALT | ✅ | Timeout == success (per spec). |
| CRC_A (hardware + software) | ✅ | Poly 0x1021, init 0x6363. |
| Bit-collision position reporting | ✅ | Via `MFRC522_ERR_COLLISION` + CollReg. |

---

## MIFARE

| Feature | Support | Notes |
|---------|---------|-------|
| MIFARE Classic 1K / 4K / Mini | ✅ | 4-byte-UID cards. |
| Crypto1 auth (Key A / Key B) | ✅ | Hardware `MFAuthent` command. |
| 16-byte block READ / WRITE | ✅ | CRC/ACK/NAK validated. |
| Sector read / write / auth | ✅ | Trailer block never written. |
| Value block Inc/Dec/Restore/Transfer | ✅ | Library-level protocol (see below). |
| Value-block formatting | ✅ | Pure helper function. |
| MIFARE Ultralight / NTAG page read | ✅ | Same READ command path. |
| NTAG password auth / ECC | ❌ | Not implemented (out of scope). |
| MIFARE DESFire / Plus / ISO 14443-4 (T=CL) | ❌ | Out of scope. |
| 7/10-byte UID *authentication* (MF1xxx shortcut) | ⚠️ Partial | Auth uses the last 4 UID bytes (covers Classic 1K/4K/Mini). |

**Value blocks are a *protocol* feature, not a hardware command.** The MFRC522
has no "increment" command; the library drives the two-step MIFARE value
protocol (command + operand transfer) through the transceive primitive.

---

## Reader (MFRC522) management

| Feature | Support |
|---------|---------|
| Hard reset (NRSTPD) / soft reset | ✅ |
| Firmware version detection | ✅ (0x90/0x91/0x92, FM17522 0x88) |
| Digital self-test (datasheet 16.1.1) | ✅ (exact NXP reference vectors) |
| Antenna on/off | ✅ |
| IRQ (Timer/Err/HiAlert/LoAlert/Idle/Rx/Tx) | ✅ dispatcher + callback |
| Soft power-down (`Sleep`/`WakeUp`) | ✅ |
| Full power-down (`PowerDown` via NRSTPD) | ✅ |
| Receiver gain (RFCfgReg RxGain) | ⚠️ Partial | Register exposed; no high-level setter. |

---

## Non-functional requirements

| Requirement | Status |
|-------------|--------|
| No dynamic allocation (malloc/free) | ✅ verified |
| No printf in the core | ✅ (debug via callback) |
| No HAL dependency in the core | ✅ (isolated to platform/stm32) |
| Compile-time feature toggles | ✅ (`mfrc522_config.h`) |
| C99, explicit-width integers | ✅ |
| Thread-safety hooks (lock/unlock) | ✅ (opt-in) |
| Bounded timeouts on all waits | ✅ |
| MISRA-C:2012-lean | ✅ (see architecture.md §10/12) |

---

## Deliberately NOT implemented (with rationale)

| Feature | Rationale |
|---------|-----------|
| Non-blocking `StartXxx()`/`Process()` state machine | The cascade anti-collision loop is iterative and collision-driven; a resumable state machine would be large and hard to verify. The bounded blocking primitives (`IsCardPresent` ~50 ms, IRQ callbacks, RTOS task) give the same real-time responsiveness. See architecture.md §5. |
| DMA transports in the core | DMA is a platform concern; the SPI adapter can use DMA via the HAL handle without any core change. |
| DESFire / Plus / T=CL command sets | Different protocol stack, outside the MFRC522 transparent-frontend scope. |
| NTAG password auth / ECC | Card-side crypto, not an MFRC522 capability. |

---

## Known limitations

1. **UART interface** is the least tested and least robust path; SPI is
   recommended for production.
2. **Two cards in the field** can cause REQA timeouts on some antenna
   designs; the anti-collision loop resolves collisions, but a weak antenna
   may report `MFRC522_ERR_TIMEOUT` before collision detection kicks in.
3. **Value blocks** require the card's access bits to be configured for value
   mode; the factory default (`000b`, transport config) does **not** allow
   Inc/Dec/Transfer — the example 09 documents this.
4. **Receiver gain** tuning is exposed as a register, not as a
   high-level API; most applications do not need it.
5. The **self-test** leaves the reader unusable until the registers are
   re-configured; `MFRC522_SelfTest()` re-initializes them automatically.
