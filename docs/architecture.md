# MFRC522 Driver — Architecture

This document describes the design of a **portable, modular, production-grade
MFRC522 driver + protocol stack** for embedded systems, with STM32 as the
primary target.

> Design goal (from the project brief): *a Portable, Modular, Low-Level,
> Production-Grade MFRC522 Driver + Protocol Stack for Embedded Systems* —
> **not** an Arduino library port.

---

## 1. Layering

```
Application
    │
    ▼
MFRC522 High-Level API            (mfrc522.h  → src/mfrc522.c)
    ├── Card detection / UID / card info
    ├── Antenna / power / version / self-test
    └── Non-blocking + IRQ facade
    │
    ▼
MFRC522 Protocol Layer            (mfrc522_protocol.h → src/mfrc522_protocol.c)
    ├── REQA / WUPA / ANTICOLL / SELECT / HALT
    ├── TRANSCEIVE
    └── CRC_A (software) 
    │
    ▼
MIFARE Layer                      (mfrc522_mifare.h → src/mfrc522_mifare.c)
    ├── Auth (KeyA/KeyB) / Read / Write
    ├── Sector ops / Value ops
    │
    ▼
MFRC522 Register Driver           (src/mfrc522_registers.c)
    ├── Read/Write register, Set/Clear bits
    ├── FIFO read/write, hardware CRC, command control
    │
    ▼
Transport Layer                   (interface/mfrc522_{spi,i2c,uart}.c)
    ├── SPI  : address byte  (addr<<1 | R/W) + 8-bit frames
    ├── I2C  : device address + register frame
    └── UART : LSB-first address byte framing
    │
    ▼
MCU Hardware Abstraction (Platform) (platform/stm32/mfrc522_stm32_*.c)
    ├── Timing (µs/ms delay, ms tick)
    ├── GPIO (CS, RST, IRQ)
    ├── Byte I/O (HAL_SPI / HAL_I2C / HAL_UART)
    └── Optional locking (RTOS)
```

**Golden rule:** the core library (`src/`, `interface/`) only knows two small
interfaces (`MFRC522_PlatformOps_t` and `MFRC522_TransportOps_t`). It never
calls `HAL_*()`, never includes a HAL header, never calls `printf()` and never
calls `malloc()/free()`.

---

## 2. The two abstraction interfaces

### 2.1 `MFRC522_PlatformOps_t` — MCU abstraction

Implemented **once per platform** (here: `platform/stm32/`). It abstracts
everything the driver needs from the MCU:

| Function          | Purpose                                     |
|-------------------|---------------------------------------------|
| `delay_us/ms`     | Busy/blocking delays                        |
| `get_tick_ms`     | Free-running millisecond counter (timeouts) |
| `cs_assert/deassert` | Chip-select control                       |
| `reset_assert/deassert` | NRSTPD control                         |
| `irq_read`        | Read the IRQ pin level                      |
| `transmit/receive/transmit_receive` | Raw byte I/O to the selected peripheral |
| `lock/unlock`     | Optional RTOS mutex (NULL = single-thread)  |

The platform carries an opaque `void *ctx` (its own private context — for
STM32 that is a struct holding the peripheral handle + GPIO pins). The
adapter stores that context **inside the handle** (`handle->platform_storage`)
so the application never allocates memory.

### 2.2 `MFRC522_TransportOps_t` — host-interface framing

Implemented **once per host interface** (`interface/`). Converts abstract
register requests into the byte framing the silicon expects:

| Function          | SPI                          | I2C                              | UART                            |
|-------------------|------------------------------|----------------------------------|---------------------------------|
| `read_register`   | send `(addr<<1)|0x80`, read  | write addr, repeated-start read  | send `addr|0x80` (LSB-first)    |
| `write_register`  | send `(addr<<1)&0x7E`+data   | write addr+data                  | send `addr&0x7F`+data           |
| `read_burst`      | addr + N read bytes          | addr + N read bytes              | addr + N read bytes             |
| `write_burst`     | addr + N write bytes         | addr + N write bytes             | addr + N write bytes            |

The exact framing rules come from the NXP MFRC522 datasheet §8.1; they are
documented per interface in `docs/spi.md`, `docs/i2c.md` and `docs/uart.md`.

---

## 3. The reader handle

Everything lives inside one struct — no globals, no heap:

```c
typedef struct MFRC522_Handle {
    MFRC522_Transport_t          transport;      /* type + params (addr/baud/speed) */
    const MFRC522_TransportOps_t *transport_ops; /* framing vtable (const/ROM)      */
    MFRC522_Platform_t           platform;       /* MCU vtable + opaque ctx         */
    MFRC522_Config_t             config;         /* runtime config copy             */
    MFRC522_State_t              state;          /* version/flags/last-error + async */
    const MFRC522_Debug_t       *debug;          /* optional log sink               */
    union { ... } platform_storage;              /* adapter private ctx (aligned)   */
} MFRC522_Handle_t;
```

Key properties:

- **Multiple instances** are trivially supported (each handle is self-contained).
- **ROM-efficient**: the two vtables are `const` and shared across instances;
  only the tiny mutable state differs.
- **No dynamic allocation**: the platform context buffer is embedded in the
  handle (size = `MFRC522_PLATFORM_CTX_SIZE`, checked at compile time by the
  adapter).

---

## 4. Thread safety & RTOS

The library keeps **no global mutable state**. All state is in the handle.
For RTOS use, the platform adapter supplies `lock`/`unlock` callbacks (e.g.
wrapping a FreeRTOS mutex). The core wraps each top-level operation in
`lock()/unlock()` when a lock is present, making high-level calls atomic on a
shared bus. When no lock is provided the code is simply single-threaded.

The non-blocking API (see below) is designed to be called from a single RTOS
task; it never blocks inside a lock.

---

## 5. Blocking vs non-blocking

- **Blocking** API (`MFRC522_ReadUID`, `MFRC522_ReadBlock`, ...) — simple,
  every wait loop has a configurable timeout.
- **Non-blocking** API (`MFRC522_StartReadUID` + `MFRC522_Process`) — a small
  cooperative state machine stored inside the handle (`state.async`). Enabled
  only when `MFRC522_ENABLE_NONBLOCKING == 1` so it costs zero RAM otherwise.
  The design deliberately reuses the blocking primitives one step at a time
  rather than duplicating protocol logic, which keeps RAM/Flash low.

---

## 6. Timeout system

Every blocking loop is bounded. The pattern is uniform:

```c
uint32_t deadline = platform->ops->get_tick_ms(platform->ctx) + timeout_ms;
do {
    ... poll status ...
    if (platform->ops->get_tick_ms(platform->ctx) >= deadline)
        return MFRC522_ERR_TIMEOUT;
} while (cond);
```

Timeouts are configurable per-call (0 = use `config.timeout_ms` default).

---

## 7. Error handling

A single `MFRC522_Status_t` enum is returned everywhere; `MFRC522_OK == 0`.
The last error is mirrored into `handle->state.last_error` for diagnostics.
`MFRC522_StatusToString()` maps codes to static strings for logging.

---

## 8. Logging

The core never calls `printf()`. Optional diagnostics go through a
`MFRC522_Debug_t` callback (`log(ctx, level, message)`), which the
application binds to a UART, ITM/SWO, semihosting or a ring buffer. Compiled
out entirely when `MFRC522_ENABLE_DEBUG == 0`.

---

## 9. Compile-time configuration

`include/mfrc522_config.h` gates every feature:

| Macro                         | Default | Effect                          |
|-------------------------------|---------|---------------------------------|
| `MFRC522_ENABLE_SPI/I2C/UART` | 1       | Include the matching transport  |
| `MFRC522_ENABLE_IRQ`          | 1       | IRQ API + dispatcher             |
| `MFRC522_ENABLE_MIFARE`       | 1       | MIFARE Classic/Ultralight layer  |
| `MFRC522_ENABLE_NONBLOCKING`  | 0       | Async state machine              |
| `MFRC522_ENABLE_DEBUG`        | 0       | Debug logging                    |

Disabling a feature removes its code at link time (the linker drops the
unreferenced object code), shrinking Flash/RAM.

---

## 10. MISRA-C:2012 & C standard

- Language: **C99** (widely available, `stdint.h`, designated init avoided in
  core where it hurts portability, no VLA, no recursion).
- Explicit-width integer types everywhere (`uint8_t`, `uint32_t`, ...).
- No dynamic allocation, no recursion, minimal global state, `const`
  correctness on all read-only pointers, `static` internal linkage for all
  non-public functions, enums/typedefs for state.
- Complex preprocessor logic is limited to the config/register headers and is
  fully parenthesized.

Deviations (documented, not hidden): see the "Deviations" table at the end of
this file.

---

## 11. Performance notes

- **SPI burst** reads/writes use a single full-duplex transaction (one CS
  assertion), avoiding per-byte CS toggling.
- **FIFO** drains use bulk `read_burst` from `FIFO_DATA` (address stays valid
  for auto-increment reads).
- The register driver inlines short register reads/writes (small, static,
  frequently used) to avoid call overhead on the hot path.
- Timeouts use the platform tick counter, not `delay_ms`, so they never block
  longer than necessary.
- On STM32H7, the adapter can use DMA (planned) without the core ever knowing;
  the platform vtable hides it.

---

## 12. Deviations & known limitations (MISRA / scope)

| Item | Deviation / note |
|------|------------------|
| `#include` of HAL header | Isolated to `platform/stm32/`; the core is HAL-free. |
| Function-pointer tables | C99/MISRA-clean vtables; no function pointers in data shared with ISR unless documented. |
| `MFRC522_PLATFORM_CTX_SIZE` union cast | The adapter casts the opaque buffer to its context type; guarded by a compile-time size assert. |
| UART host interface | Implemented as far as the silicon allows; LSB-first + DTRQ make it unusual — see `docs/uart.md`. |
| Value-block ops | Library-level (protocol) feature, not an MFRC522 hardware command. |

---

## 13. Capability boundaries (hardware vs library vs card)

| Capability | MFRC522 HW | Library | Card |
|------------|:----------:|:-------:|:----:|
| ISO/IEC 14443-A framing / parity / CRC | ✔ (digital logic) | ✔ | — |
| Crypto1 auth (MFAuthent command)       | ✔ (hardware)     | ✔ | MIFARE Classic |
| REQA/ANTICOLL/SELECT/HALT sequencing   | — (host driven)  | ✔ | — |
| Block read/write semantics             | — (host driven)  | ✔ | MIFARE Classic |
| Value increment/decrement/restore/transfer | — (host)     | ✔ | MIFARE Classic value blocks |
| MIFARE Ultralight / NTAG read          | — (host driven)  | ✔ | Ultralight/NTAG |
| ISO 14443-4 (T=CL) / DESFire / Plus    | — (host driven)  | ✘ (out of scope) | — |
| NTAG password auth / ECC               | —                | ✘ | — |
