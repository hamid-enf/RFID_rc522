# SPI host interface

The SPI interface is the most common and highest-throughput way to talk to the
MFRC522 (up to **10 Mbit/s**). It is the recommended default.

## Signal wiring

| MFRC522 pin | STM32H743      | Notes                              |
|-------------|----------------|------------------------------------|
| SDA / NSS   | GPIO (CS)      | Any GPIO, driven by the adapter    |
| SCK  (D5)   | SPI_SCK        | e.g. SPI1_SCK  = PB3               |
| MOSI (D6)   | SPI_MOSI       | e.g. SPI1_MOSI = PB5               |
| MISO (D7)   | SPI_MISO       | e.g. SPI1_MISO = PB4               |
| RST  (NRSTPD)| GPIO          | Optional; enables hard reset       |
| IRQ         | GPIO (EXTI)    | Optional; for interrupt use        |
| 3.3V / GND  | 3.3V / GND     |                                     |

Interface selection: the MFRC522 auto-detects SPI when pin **I2C = 0** and pin
**EA = 1** (many breakout boards hard-wire this).

## Framing (NXP datasheet §8.1.2)

The first byte of every transaction is the **address byte**:

```
bit 7         : mode — 0 = write, 1 = read   (MSB)
bits 6..1     : 6-bit register address (raw address, NOT pre-shifted)
bit 0         : unused (0)
```

- **Write**: address byte `(addr << 1)` followed by the data byte(s).
- **Read**: address byte `(addr << 1) | 0x80` followed by N dummy bytes; the
  device clocks out the register contents on MISO. The register address
  **auto-increments** across burst accesses (except `FIFO_DATA`, which is a
  non-incrementing window into the 64-byte FIFO).

`NSS` must remain low for the entire transaction (the adapter's `cs_assert` /
`cs_deassert` handle this).

## Electrical / timing

- SPI mode 0 (CPOL = 0, CPHA = 0), MSB first, 8-bit frames.
- Maximum clock: **10 MHz**. The library's `MFRC522_SpiSpeed_t` selects among
  three presets (≈1 MHz "low", ≈4 MHz "medium", up to 10 MHz "high"). Medium
  (4 MHz) is the pragmatic default; use "low" for long/degraded wiring.

## Adapter API

```c
MFRC522_Status_t MFRC522_STM32_SPI_Init(MFRC522_Handle_t *handle,
                                        SPI_HandleTypeDef *hspi,
                                        GPIO_TypeDef *cs_port, uint16_t cs_pin,
                                        GPIO_TypeDef *rst_port, uint16_t rst_pin,
                                        GPIO_TypeDef *irq_port, uint16_t irq_pin);

void MFRC522_STM32_SPI_SetSpeed(MFRC522_Handle_t *handle, uint8_t speed);
```

The adapter maps the library's `transmit`/`receive` primitives onto
`HAL_SPI_Transmit` / `HAL_SPI_Receive` under a manually driven CS. Polling,
interrupt and DMA SPI are all usable (configure the HAL handle as usual); the
core does not care which transfer method the adapter uses.

## Error handling

- `MFRC522_ERR_COMM` is returned when the HAL transfer fails (bus error, etc.).
- `MFRC522_ERR_TIMEOUT` is returned by the higher layers when the device does
  not answer; SPI itself is a master-driven protocol, so timeouts are enforced
  by the command/IRQ waits in the register driver, not by SPI.
