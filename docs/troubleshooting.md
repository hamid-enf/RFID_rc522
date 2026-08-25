# Troubleshooting

## Communication failures

### `MFRC522_ERR_DEVICE` at init (bad/unknown version)

- Check that `VersionReg` reads back a known value (0x92, 0x91, 0x90, 0x88).
- Reading `0x00` or `0xFF` usually means the reader is not answering at all:
  - SPI: verify wiring (SCK/MOSI/MISO/CS), mode 0, MSB first, and that CS is
    driven by a GPIO (not hardware NSS).
  - I2C: verify the 7-bit address (default 0x28), the pull-ups, and that pin
    `I2C` is tied high to select I2C mode.
  - UART: verify baud rate matches the MFRC522 `SerialSpeedReg` and 8N1.

### `MFRC522_ERR_COMM` (host interface error)

- SPI: MISO floating? Ensure the MISO pin has a pull-up and the module is
  powered at 3.3 V.
- I2C: NACK means wrong address or the device is not in I2C mode; a stuck SDA
  needs a bus-recovery (toggle SCL until SDA releases).
- UART: framing noise; shorten wires, check ground.

### Every register read fails, but the logic analyzer shows SCK/CS and MISO activity

The MFRC522 is answering on the wire, yet `HAL_SPI_Transmit`/
`HAL_SPI_Receive` return non-OK — the fault is in the STM32 SPI setup, not
the module. Use the raw SPI probe from example 01 (it prints the HAL status
codes) and check:

- `tx`/`rx` = 3 (`HAL_TIMEOUT`): the transfer never completes. Verify the
  SPI1 clock source in CubeMX (e.g. PLLP) and that `SPI_SR.BSY` is not stuck.
- `tx`/`rx` = 1 (`HAL_ERROR`) with HAL error `0x01` (`OVR`): RX overrun — the
  RX FIFO was not drained between transfers. Keep the SPI in full-duplex
  2-line mode.
- `tx`/`rx` = 2 (`HAL_BUSY`): the HAL handle is stuck after an earlier
  error; call `HAL_SPI_Abort()` before retrying.
- The raw probe succeeds (`value` = 0x92/0x91/0x88) while the driver fails:
  the fault is in the driver's transfer sequence — report it with the full
  console output.

Run the probe twice: once right after `MX_SPI1_Init()` (clean state) and
once after a failed `MFRC522_Init()`. If the first probe already fails, the
SPI peripheral/clock configuration is wrong; if only the second fails, the
init sequence is corrupting the SPI state.

### HAL transfers succeed but the read values are wrong (e.g. 0x37 never reads 0x92)

The SPI works, the MFRC522 answers, but the sampled data is not a healthy
chip's data. Run the raw **canary scan** (example 01): it reads registers
with fixed reset values directly through the HAL, bypassing the driver.

- `0x0B` must read `0x40` (WaterLevel reset value). If it does, the data
  path is healthy and the chip is simply a different/clone silicon whose
  `VersionReg` is not one of the accepted values (0x92/0x91/0x90/0x88/0xB2)
  — report the raw 0x37 value so it can be added to the accepted list.
- If **all** canary values are garbage, the data path is corrupt: slow the
  SPI clock down (e.g. `SPI_BAUDRATEPRESCALER_128` or `_256`) and re-run.
  Long MISO traces and weak pull-ups need slower clocks.

Also do a full clean rebuild (Project -> Clean, then Build) when mixing
hand-edited files with CubeMX-generated code, to rule out stale objects.

### The same register reads different values on different boots/runs

VersionReg is a read-only silicon register — it must return the same value
on every read. If the probe (or the printed firmware version) shows e.g.
`0xF6`, `0xB2`, `0xF6` across reboots, the MISO data is intermittently
corrupt: the MCU samples a bit while the line is still transitioning
(setup/hold margin exhausted) or the line rings.

- Slow the SPI clock: `BaudRatePrescaler` 32 -> 128 (or 256) and re-run.
  The probe reads the version five times; all five must be identical.
- Shorten the MISO trace / add a pull-up near the MFRC522 (many modules
  have one on board; wire-wrapped or breadboard links often do not).
- Add a 10-100 uF bulk capacitor on the module VCC-GND: supply droop
  during SPI bursts changes edge timing and causes the same symptom.

Note: intermittent MISO corruption also breaks card detection, because
REQA/anti-collision traffic is moved through the SPI register/FIFO path —
fix the link first, then test cards.

### `MFRC522_ERR_TIMEOUT` everywhere

- The reader may be in soft power-down; call `MFRC522_WakeUp()`.
- After `MFRC522_PowerDown()` the device needs `MFRC522_HardReset()` +
  `MFRC522_Init()`.
- After `MFRC522_SelfTest()` make sure the registers were re-initialized
  (the function does this itself).

## Card detection issues

### No card detected

- Confirm the antenna is on (`MFRC522_IsAntennaOn` == 1).
- Move the card closer / center it over the antenna.
- Check the antenna matching components on the module.
- Increase the poll timeout (`MFRC522_CARD_POLL_TIMEOUT_MS`).

### Intermittent detection

- Antenna tuning or supply decoupling (a 10–100 µF bulk cap near the module
  helps on many breakout boards).
- Reduce SPI speed (use `MFRC522_SPI_SPEED_LOW`) to rule out bus noise.

### Two cards, flaky reads

- The anti-collision loop resolves collisions, but a weak antenna can cause
  REQA timeouts before collision detection engages. Present one card at a time
  where possible.

### Card detected once, then never again (card stays in the field)

- The card is in the **authenticated** state (post-MFAuthent): such a card
  ignores REQA/WUPA until it leaves that state. Call `MFRC522_HaltTag()` and
  re-poll, or wait for the card to be removed.
- On older revisions `IsCardPresent` sent WUPA only, so it missed cards that
  were already selected (READY). The driver now sends REQA first (which also
  answers selected cards, as RTSA) and falls back to WUPA to wake HALT'ed
  cards — see `docs/api.md` §4.

## MIFARE issues

### `MFRC522_ERR_AUTH` (authentication failed)

- Wrong key (factory default is `FF FF FF FF FF FF`).
- Key type mismatch (A vs B).
- The card left the field between select and auth — re-select.

### `MFRC522_ERR_CRC` on block read

- The block was not authenticated before reading.
- Communication error mid-frame; retry.

### Write "succeeds" but data unchanged

- You may have written a value- or access-controlled block, or the card uses a
  non-default key. Verify the sector access bits.
- Writing the **sector trailer** or **manufacturer block (block 0)** is
  restricted — the sector helpers deliberately skip the trailer.

### Value operations return `MFRC522_ERR_AUTH`/NAK

- The block's access bits are not in value mode (`[C1 C2 C3]` = 110b/001b).
- The sector is not authenticated.

## Build / integration

- **Linker: undefined `MFRC522_STM32_*`** — the platform adapter files
  (`platform/stm32/*.c`) are not in the build, or the include path to
  `platform/stm32/` is missing.
- **Missing `stm32h7xx_hal.h`** — define your device macro (e.g.
  `STM32H743xx`) and add the HAL include directories.
- **`MFRC522_ERR_NOT_SUPPORTED` at init** — the selected transport is compiled
  out; set the matching `MFRC522_ENABLE_SPI/I2C/UART` to 1 in
  `mfrc522_config.h`.

## Debugging tips

- Attach a debug sink (`MFRC522_AttachDebug`) with `MFRC522_ENABLE_DEBUG=1`
  to see register-level traces routed through your own sink.
- Dump `VersionReg` first — it distinguishes "no comms" (0x00/0xFF) from "wrong
  device" (unexpected value).
- Use the host tests (`tests/test_transport.c`) to validate the core without
  hardware.
