# Examples

Fifteen example programs, one directory each. Every example is a
self-contained `main.c` designed to be dropped into a **STM32CubeIDE /
CubeMX** project (it expects the usual generated files: `SystemClock_Config`,
`MX_GPIO_Init`, `MX_SPI1_Init`, `MX_USART3_UART_Init`, `Error_Handler`, and the
peripheral handles `hspi1` / `hi2c1` / `huart3`).

| # | Example | What it shows |
|---|---------|---------------|
| 01 | `basic_init`        | adapter attach + init + version + self-test + antenna |
| 02 | `detect_card`       | `IsCardPresent` (poll) vs `WaitForCard` (block) |
| 03 | `read_uid`          | `ReadUID` (4/7/10-byte) + `HaltTag` |
| 04 | `card_info`         | `GetCardInfo` (ATQA + SAK + UID + type) |
| 05 | `mifare_auth`       | `AuthKeyA` / `AuthKeyB` / `StopCrypto1` |
| 06 | `read_block`        | authenticate + `ReadBlock` |
| 07 | `write_block`       | authenticate + `WriteBlock` |
| 08 | `sector_operations` | `AuthenticateSector` / `ReadSector` / `WriteSector` |
| 09 | `value_block`       | `FormatValueBlock` + Inc/Dec/Restore/Transfer |
| 10 | `irq`               | `AttachIRQCallback` + `ProcessIRQ` |
| 11 | `low_power`         | `Sleep` / `WakeUp` / `PowerDown` |
| 12 | `spi`               | SPI interface (recommended) |
| 13 | `i2c`               | I2C interface |
| 14 | `uart`              | UART interface |
| 15 | `stm32h743_full_demo` | monitoring demo (§31 console layout) |

## Shared helper

`common/mfrc522_demo.h` + `common/mfrc522_demo.c` provide a small
`demo_printf()` (printf over the debug UART) and declare the CubeMX functions
each example relies on. This helper is for examples only — the core library
never uses printf.

## Adding the library to your CubeIDE project

1. Copy `include/`, `src/`, `interface/` and `platform/stm32/` into your
   project (or add them as linked source folders).
2. Add `include/` and `platform/stm32/` to the compiler include paths.
3. Add all `.c` files from `src/`, `interface/` and `platform/stm32/` to the
   build.
4. Define your device macro (`STM32H743xx` etc.) — CubeIDE already does this.
5. Copy one example's `main.c` (+ `common/`) into your project and adapt the
   pin/peripheral names.

## Pin/peripheral names

The examples reference `RFID_CS_GPIO_Port`, `RFID_CS_Pin`, `RFID_RST_*`,
`RFID_IRQ_*`, `hspi1`, `hi2c1`, `huart3`. Create these in CubeMX (or map them
to your own names via `#define` / `extern` in your project).

## Notes

- The demo key is the MIFARE Classic **factory default** (`FF FF FF FF FF FF`).
- Examples 06/07/15 write to **block 4** (sector 1, safe with the factory
  key). Never write the sector trailer (block 3/7/11/...) or the manufacturer
  block (block 0) unless you know what you are doing.
