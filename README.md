# MFRC522 Driver for STM32

A **portable, modular, production-grade MFRC522 driver + ISO/IEC 14443-A /
MIFARE protocol stack** for embedded systems. Primary target: **STM32H743**
with the STM32 HAL, written in **C99**, with **no dynamic allocation**, **no
printf in the core**, and full support for **SPI / I²C / UART** host
interfaces.

> This is *not* an Arduino-library port. It is a layered, hardware-abstracted
> driver designed around embedded-systems principles (see
> [docs/architecture.md](docs/architecture.md)).

---

## Status / roadmap

The project is delivered in phases:

| Phase | Content | Status |
|-------|---------|:------:|
| 1 | Architecture + API design + file structure | ✅ done |
| 2 | Register driver + transport layer | ✅ done |
| 3 | SPI / I²C / UART transports | ✅ done |
| 4 | ISO/IEC 14443-A protocol | ✅ done |
| 5 | MIFARE layer | ✅ done |
| 6 | STM32H743 HAL adapter | ⏳ next |
| 7 | Examples | ⏳ |
| 8 | Tests + docs + optimization | ⏳ |

---

## Quick start (target usage)

```c
#include "mfrc522.h"

MFRC522_Handle_t rfid = {0};

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART3_UART_Init();

    /* Attach the STM32 SPI adapter, then initialize the reader. */
    MFRC522_STM32_SPI_Init(&rfid, &hspi1,
                           RFID_CS_GPIO_Port,  RFID_CS_Pin,
                           RFID_RST_GPIO_Port, RFID_RST_Pin,
                           NULL, 0);

    if (MFRC522_Init(&rfid) != MFRC522_OK)
        Error_Handler();

    while (1)
    {
        if (MFRC522_IsCardPresent(&rfid) == MFRC522_OK)
        {
            MFRC522_UID_t uid;
            if (MFRC522_ReadUID(&rfid, &uid) == MFRC522_OK)
            {
                /* Process UID (uid.bytes, uid.length, uid.sak) */
            }
        }
    }
}
```

Switching to I²C or UART is a one-line change (`MFRC522_STM32_I2C_Init` /
`MFRC522_STM32_UART_Init`) — the core library does not change.

---

## Repository layout

```
include/                 Public headers (core is hardware-independent)
  mfrc522.h              Umbrella API
  mfrc522_types.h        Types / enums / structs
  mfrc522_config.h       Compile-time configuration
  mfrc522_registers.h    Register map + bit fields (datasheet-accurate)
  mfrc522_transport.h    Platform + transport abstraction (the HAL boundary)
  mfrc522_protocol.h     ISO/IEC 14443-A protocol layer
  mfrc522_mifare.h       MIFARE Classic / Ultralight layer
interface/               Host-interface transports (SPI / I2C / UART framing)
platform/stm32/          STM32 HAL adapters (the only place HAL_* is used)
src/                     Core implementation
examples/                Example programs (basic, uid, mifare, spi, i2c, uart, stm32h743)
docs/                    Architecture, API, register map, interface & wiring docs
tests/                   Unit / host-side tests
CMakeLists.txt           Build definition
```

---

## Documentation

- [Architecture](docs/architecture.md) — layering, abstractions, threading, MISRA
- [API reference](docs/api.md) — full function contracts
- [Register map](docs/register_map.md) — datasheet-accurate register/bit reference
- [SPI interface](docs/spi.md) — framing, timing, wiring
- [I2C interface](docs/i2c.md) — addressing (ADR/EA), framing, recovery
- [UART interface](docs/uart.md) — LSB-first framing, baud table, limitations
- [MIFARE support](docs/mifare.md) — auth, blocks, sectors, value blocks, limits
- (still to come: `stm32h743.md`, `troubleshooting.md`)

## Host-side tests

The core library compiles and runs on a host (no hardware needed) using a
mocked MFRC522 register file:

```sh
cc -std=c99 -Wall -Wextra -Iinclude \
   tests/test_transport.c src/mfrc522.c src/mfrc522_registers.c src/mfrc522_crc.c \
   interface/mfrc522_spi.c interface/mfrc522_i2c.c interface/mfrc522_uart.c \
   -o test_transport && ./test_transport
```

or via CMake: `cmake -S . -B build -DMFRC522_BUILD_TESTS=ON && cmake --build build && ctest --test-dir build`.

---

## Key facts (NXP MFRC522 datasheet)

- Host interfaces: **SPI** (up to 10 Mbit/s), **I²C** (Fast mode 400 kbit/s,
  High-speed 3.4 Mbit/s), **serial UART** (up to 1228.8 kBd, LSB-first, 8N1).
- I²C 7-bit address default **0x28** (all ADR pins + EA low).
- 64-byte FIFO; hardware CRC/parity/framing; hardware Crypto1 (MFAuthent).
- Version register default **0x92** (silicon v2.0).

---

## License

Project under construction; license to be added.
