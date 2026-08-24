# MFRC522 Driver — API Reference (Phase 1)

This document is the authoritative API contract for the library. Signatures
are declared in the public headers under `include/`. Implementations land in
later phases; this phase fixes the interface so it can be reviewed before any
code is written.

Every function entry documents, where relevant: purpose, parameters, return
value, side effects, timeout behavior, thread safety and an example.

---

## 1. Lifecycle

### `MFRC522_Init`
```c
MFRC522_Status_t MFRC522_Init(MFRC522_Handle_t *handle);
```
- **Purpose**: hard+soft reset, version check, timer/FIFO/antenna/CRC config,
  self-test, antenna enable.
- **Parameters**: `handle` — configured by a platform adapter first.
- **Return**: `MFRC522_OK`, or `MFRC522_ERR_DEVICE` (bad version),
  `MFRC522_ERR_COMM` (no response), `MFRC522_ERR_TIMEOUT`.
- **Side effects**: resets the IC; sets `state.version_raw`, `state.flags`.
- **Thread safety**: not re-entrant against the same handle.

### `MFRC522_Deinit`
```c
MFRC522_Status_t MFRC522_Deinit(MFRC522_Handle_t *handle);
```
Disables antenna, stops Crypto1, clears `state`.

### `MFRC522_SoftReset` / `MFRC522_HardReset`
```c
MFRC522_Status_t MFRC522_SoftReset(MFRC522_Handle_t *handle);
MFRC522_Status_t MFRC522_HardReset(MFRC522_Handle_t *handle);
```
Soft reset writes `MFRC522_CMD_SOFT_RESET` and waits for the power-down bit to
clear; hard reset pulses `NRSTPD` via the platform GPIO.

### `MFRC522_GetVersion`
```c
MFRC522_Status_t MFRC522_GetVersion(MFRC522_Handle_t *handle, MFRC522_Version_t *version);
```
Reads `VersionReg` (0x30). Known raw values: `0x91` (v1.0), `0x92` (v2.0).
Anything else → `MFRC522_ERR_DEVICE`.

### `MFRC522_SelfTest`
```c
MFRC522_Status_t MFRC522_SelfTest(MFRC522_Handle_t *handle);
```
Runs the digital self-test (with antenna), returns `MFRC522_OK` on pass.

---

## 2. Register / FIFO layer

```c
MFRC522_Status_t MFRC522_ReadRegister (MFRC522_Handle_t *, uint8_t addr, uint8_t *value);
MFRC522_Status_t MFRC522_WriteRegister(MFRC522_Handle_t *, uint8_t addr, uint8_t value);
MFRC522_Status_t MFRC522_SetBits     (MFRC522_Handle_t *, uint8_t addr, uint8_t mask);
MFRC522_Status_t MFRC522_ClearBits   (MFRC522_Handle_t *, uint8_t addr, uint8_t mask);
MFRC522_Status_t MFRC522_ReadFIFO    (MFRC522_Handle_t *, uint8_t *data, uint32_t len);
MFRC522_Status_t MFRC522_WriteFIFO   (MFRC522_Handle_t *, const uint8_t *data, uint32_t len);
MFRC522_Status_t MFRC522_CalcCRC     (MFRC522_Handle_t *, const uint8_t *data, uint32_t len, uint16_t *crc);
```
- Read-modify-write for Set/Clear bits is atomic under the platform lock.
- `ReadFIFO`/`WriteFIFO` enforce `len <= MFRC522_FIFO_SIZE` (64).
- `CalcCRC` uses the hardware CRC coprocessor (`MFRC522_CMD_CALC_CRC`).

---

## 3. RF / antenna

```c
MFRC522_Status_t MFRC522_AntennaOn (MFRC522_Handle_t *handle);
MFRC522_Status_t MFRC522_AntennaOff(MFRC522_Handle_t *handle);
uint8_t          MFRC522_IsAntennaOn(const MFRC522_Handle_t *handle);
```
- Enables/disables the TX drivers (`TxControlReg`).
- `IsAntennaOn` reads the cached `state` flag (no bus access).

---

## 4. Card detection

```c
MFRC522_Status_t MFRC522_IsCardPresent(MFRC522_Handle_t *handle);
MFRC522_Status_t MFRC522_WaitForCard(MFRC522_Handle_t *handle, uint32_t timeout_ms);
```
- `IsCardPresent`: REQA with a short timeout; returns `MFRC522_OK` or
  `MFRC522_ERR_NO_CARD`.
- `WaitForCard`: blocks (polling REQA) until a card answers or `timeout_ms`
  (0 → default) elapses.

---

## 5. UID / card info

```c
MFRC522_Status_t MFRC522_ReadUID    (MFRC522_Handle_t *, MFRC522_UID_t *uid);
MFRC522_Status_t MFRC522_GetCardInfo(MFRC522_Handle_t *, MFRC522_CardInfo_t *info);
```
- `ReadUID`: full cascade (up to 3 levels) → 4/7/10-byte UID + SAK.
- `GetCardInfo`: also captures ATQA and derives the card type from SAK/ATQA.

Card type derivation (best effort, SAK-based):
| SAK   | Type                       |
|-------|----------------------------|
| 0x08  | MIFARE Classic 1K          |
| 0x18  | MIFARE Classic 4K          |
| 0x09  | MIFARE Mini                |
| 0x00  | Ultralight / NTAG          |
| 0x20  | ISO/IEC 14443-4 compliant  |

---

## 6. ISO/IEC 14443-A protocol primitives

```c
MFRC522_Status_t MFRC522_REQA        (MFRC522_Handle_t *, uint8_t *atqa, uint32_t *atqa_len);
MFRC522_Status_t MFRC522_WUPA        (MFRC522_Handle_t *, uint8_t *atqa, uint32_t *atqa_len);
MFRC522_Status_t MFRC522_Anticollision(MFRC522_Handle_t *, uint8_t cascade, uint8_t *uid, uint32_t *uid_len, uint8_t *sak);
MFRC522_Status_t MFRC522_SelectCard  (MFRC522_Handle_t *, uint8_t *uid, uint32_t *uid_len, uint8_t *sak);
MFRC522_Status_t MFRC522_HaltTag     (MFRC522_Handle_t *);
MFRC522_Status_t MFRC522_TransceiveData(MFRC522_Handle_t *, const uint8_t *tx, uint32_t tx_len, uint8_t *rx, uint32_t *rx_len, uint32_t timeout_ms);
```
- `REQA`/`WUPA` use the 7-bit short-frame format; `Anticollision` runs one
  cascade level (`cascade` = 0/1/2 → SEL 0x93/0x95/0x97) and returns that
  level's UID fragment + SAK (SAK bit 2 set ⇒ UID continues).
- `SelectCard` resolves the complete UID (4/7/10 bytes) across all cascade
  levels, handling cascade tags and BCC.
- `Anticollision` reports `MFRC522_ERR_COLLISION` on a bit collision (caller
  may retry with the collision position from `CollReg`).
- `TransceiveData` is the generic host-driven command path used by everything.

### Software CRC
```c
void MFRC522_CRC_A(const uint8_t *data, uint32_t len, uint16_t *crc_out);
```
ISO/IEC 14443-A CRC (poly 0x1021, init 0x6363), pure software, for validation
and tests.

---

## 7. MIFARE layer (`MFRC522_ENABLE_MIFARE`)

```c
MFRC522_Status_t MFRC522_Authenticate(MFRC522_Handle_t *, MFRC522_KeyType_t, const MFRC522_Key_t *, uint8_t block, const uint8_t *uid, uint32_t uid_len);
MFRC522_Status_t MFRC522_AuthKeyA    (MFRC522_Handle_t *, const MFRC522_Key_t *, uint8_t block, const uint8_t *uid, uint32_t uid_len);
MFRC522_Status_t MFRC522_AuthKeyB    (MFRC522_Handle_t *, const MFRC522_Key_t *, uint8_t block, const uint8_t *uid, uint32_t uid_len);
MFRC522_Status_t MFRC522_StopCrypto1 (MFRC522_Handle_t *);
MFRC522_Status_t MFRC522_ReadBlock   (MFRC522_Handle_t *, uint8_t block, uint8_t *data);
MFRC522_Status_t MFRC522_WriteBlock  (MFRC522_Handle_t *, uint8_t block, const uint8_t *data);
MFRC522_Status_t MFRC522_AuthenticateSector(MFRC522_Handle_t *, uint8_t sector, MFRC522_KeyType_t, const MFRC522_Key_t *, const uint8_t *uid, uint32_t uid_len);
MFRC522_Status_t MFRC522_ReadSector  (MFRC522_Handle_t *, uint8_t sector, MFRC522_KeyType_t, const MFRC522_Key_t *, const uint8_t *uid, uint32_t uid_len, uint8_t *data, uint32_t *data_len);
MFRC522_Status_t MFRC522_WriteSector (MFRC522_Handle_t *, uint8_t sector, MFRC522_KeyType_t, const MFRC522_Key_t *, const uint8_t *uid, uint32_t uid_len, const uint8_t *data, uint32_t data_len);

void MFRC522_FormatValueBlock(uint8_t block[MFRC522_BLOCK_SIZE], int32_t value, uint8_t address);
MFRC522_Status_t MFRC522_Increment(MFRC522_Handle_t *, uint8_t block, int32_t value);
MFRC522_Status_t MFRC522_Decrement(MFRC522_Handle_t *, uint8_t block, int32_t value);
MFRC522_Status_t MFRC522_Restore  (MFRC522_Handle_t *, uint8_t block);
MFRC522_Status_t MFRC522_Transfer (MFRC522_Handle_t *, uint8_t block);
```

- `Authenticate` runs the hardware `MFAuthent` command; failure →
  `MFRC522_ERR_AUTH`.
- `ReadBlock`/`WriteBlock` validate the card CRC / ACK(0x0A)/NAK(0x00).
- Sector ops never touch the trailer block (avoids bricking the key/AC bytes).
- Value ops implement the MIFARE value-block protocol on top of transceive.

---

## 8. Non-blocking API (`MFRC522_ENABLE_NONBLOCKING`)

```c
MFRC522_Status_t MFRC522_StartReadUID(MFRC522_Handle_t *handle);
MFRC522_Status_t MFRC522_Process(MFRC522_Handle_t *handle);
uint8_t          MFRC522_IsOperationComplete(const MFRC522_Handle_t *handle, MFRC522_Status_t *result);
```
- `StartReadUID` returns `MFRC522_ERR_BUSY` if another op is in flight.
- `Process` advances one step; returns `MFRC522_ERR_BUSY` while running.
- `IsOperationComplete` returns non-zero when done and copies the final status.

---

## 9. IRQ API (`MFRC522_ENABLE_IRQ`)

```c
typedef void (*MFRC522_IrqCallback_t)(MFRC522_Handle_t *, uint8_t irq_source, void *user);
MFRC522_Status_t MFRC522_AttachIRQCallback(MFRC522_Handle_t *, MFRC522_IrqCallback_t, void *user);
MFRC522_Status_t MFRC522_ProcessIRQ(MFRC522_Handle_t *handle);
```
- `ProcessIRQ` reads `ComIrqReg`, clears the latched bits and dispatches to the
  callback with a bit-mask of the pending sources
  (Timer/Err/HiAlert/LoAlert/Idle/Rx/Tx).
- Call from the GPIO EXTI handler or poll it from the main loop.

---

## 10. Low power

```c
MFRC522_Status_t MFRC522_Sleep   (MFRC522_Handle_t *handle);  /* soft power-down */
MFRC522_Status_t MFRC522_WakeUp  (MFRC522_Handle_t *handle);
MFRC522_Status_t MFRC522_PowerDown(MFRC522_Handle_t *handle); /* NRSTPD low */
```
- `Sleep` sets the `PowerDown` bit; the IC keeps its register contents but
  switches the antenna off.
- `WakeUp` clears `PowerDown` and waits for the internal oscillator.
- `PowerDown` holds `NRSTPD` low (≈µA range); full re-init required after.

---

## 11. Debug / utility

```c
void              MFRC522_AttachDebug(MFRC522_Handle_t *, const MFRC522_Debug_t *debug);
MFRC522_Status_t  MFRC522_GetLastError(const MFRC522_Handle_t *handle);
const char       *MFRC522_StatusToString(MFRC522_Status_t status);
```

---

## 12. Platform adapters (STM32)

```c
MFRC522_Status_t MFRC522_STM32_SPI_Init (MFRC522_Handle_t *, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin, GPIO_TypeDef *rst_port, uint16_t rst_pin, GPIO_TypeDef *irq_port, uint16_t irq_pin);
MFRC522_Status_t MFRC522_STM32_I2C_Init (MFRC522_Handle_t *, I2C_HandleTypeDef *hi2c, uint8_t dev_addr, GPIO_TypeDef *rst_port, uint16_t rst_pin, GPIO_TypeDef *irq_port, uint16_t irq_pin);
MFRC522_Status_t MFRC522_STM32_UART_Init(MFRC522_Handle_t *, UART_HandleTypeDef *huart, uint8_t baud, GPIO_TypeDef *rst_port, uint16_t rst_pin, GPIO_TypeDef *irq_port, uint16_t irq_pin);
void MFRC522_STM32_SPI_SetSpeed(MFRC522_Handle_t *, uint8_t speed);
```
Each adapter stores its private context inside the handle and wires the
`platform` + `transport_ops` fields. After any of these, `MFRC522_Init()` is
the next call.
