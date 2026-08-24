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
