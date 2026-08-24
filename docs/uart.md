# UART host interface

> ⚠️ **Read this before using UART.** The MFRC522 serial UART is the least
> common and most error-prone of the three interfaces. It is implemented here
> for completeness, but SPI is strongly recommended for new designs.

## Key facts & limitations

- The interface is a **logic-level UART**, *not* RS-232: 8 data bits,
  **LSB first**, no parity, 1 stop bit. Voltage levels follow the pad supply
  (3.3 V logic).
- Every byte — address *and* data — is transmitted **least-significant bit
  first**, so all bytes must be bit-reversed. The library does this internally;
  callers must not.
- **Half-duplex by construction**: the address byte chooses read or write, then
  the data flows one way only. There is no full-duplex TX+RX.
- The device drives a **DTRQ** flow-control line (pin D5) that requests data.
  The library performs a simple timed receive and does **not** require DTRQ;
  if you use DTRQ, wire it to a GPIO and treat the receive timeout as the
  fallback.
- Baud rates are limited to a fixed set (below), selected via `SerialSpeedReg`.

Interface selection: tie pin **I2C = 0** and pin **EA = 0**. Pin SDA = RX,
pin D7 = TX (see the datasheet pin-function table).

## Signal wiring

| MFRC522 pin | STM32H743  | Notes                              |
|-------------|------------|------------------------------------|
| SDA (RX)    | UARTx_TX   | Host TX → MFRC522 RX               |
| D7  (TX)    | UARTx_RX   | MFRC522 TX → host RX               |
| D5  (DTRQ)  | GPIO (opt.)| Flow control (optional)            |
| RST (NRSTPD)| GPIO       | Optional                           |
| IRQ         | GPIO (EXTI)| Optional                           |
| 3.3V / GND  | 3.3V / GND |                                    |

## Framing (NXP datasheet §8.1.3.3)

The first byte is the **address byte** (sent LSB first):

```
bit 7      : mode — 1 = read, 0 = write   (MSB)
bit 6      : reserved (0)
bits 5..0  : 6-bit register address
```

- **Write**: `addr` (mode 0), then the data bytes (auto-incrementing).
- **Read**: `0x80 | addr` (mode 1), then the device replies with the data.

## Baud rates (NXP datasheet, SerialSpeedReg = 0x1F)

| Index | Baud rate | SerialSpeedReg |
|------:|----------:|---------------:|
| 0     | 9600      | 0xEB           |
| 1     | 14400     | 0xDA           |
| 2     | 19200     | 0xCB           |
| 3     | 38400     | 0xAB           |
| 4     | 57600     | 0x9A           |
| 5     | 115200    | 0x7A           |
| 6     | 128000    | 0x74           |
| 7     | 230400    | 0x5A           |
| 8     | 460800    | 0x3A           |
| 9     | 921600    | 0x1C           |
| 10    | 1228800   | 0x15           |

`MFRC522_UART_ApplyBaud()` writes the matching divisor; the MCU UART must then
be configured to the same baud rate.

## Adapter API

```c
MFRC522_Status_t MFRC522_STM32_UART_Init(MFRC522_Handle_t *handle,
                                         UART_HandleTypeDef *huart,
                                         uint8_t baud,               /* index */
                                         GPIO_TypeDef *rst_port, uint16_t rst_pin,
                                         GPIO_TypeDef *irq_port, uint16_t irq_pin);
```

Configure the HAL UART as 8N1 at the chosen baud rate, then pass the matching
index to the adapter and let `MFRC522_Init()` apply `MFRC522_UART_ApplyBaud()`.

## Error handling

- `MFRC522_ERR_TIMEOUT` — the device did not respond within the configured
  timeout (the most common UART failure).
- `MFRC522_ERR_COMM` — HAL UART error (framing/noise/overrun).
- There is no built-in byte-level error recovery; a read that times out should
  be retried by the caller after a short idle.
