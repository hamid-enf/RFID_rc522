# I2C host interface

The MFRC522 supports the I2C-bus in Fast mode (**400 kbit/s**) and High-speed
mode (**3.4 Mbit/s**). It is selected by tying pin **I2C = 1** (and EA = 1 or
EA = 0 depending on the desired address mode).

## Signal wiring

| MFRC522 pin | STM32H743  | Notes                              |
|-------------|------------|------------------------------------|
| SDA         | I2Cx_SDA   | e.g. I2C1_SDA = PB7 (pull-ups req.)|
| SCL (D7)    | I2Cx_SCL   | e.g. I2C1_SCL = PB6 (pull-ups req.)|
| RST (NRSTPD)| GPIO       | Optional                           |
| IRQ         | GPIO (EXTI)| Optional                           |
| 3.3V / GND  | 3.3V / GND |                                    |

External pull-up resistors on SDA/SCL are required (typically 4.7 kΩ).

## Slave address (NXP datasheet §8.1.4.5)

The 7-bit address is latched at the release of reset, based on pin **EA** and
the **ADR_x** pins (which are shared with the SPI/UART pins):

- **EA = 0**: upper 4 bits fixed to `0101b`, lower 3 bits from ADR_2..ADR_0.
  Address = `0x28 | (ADR_2 << 2) | (ADR_1 << 1) | ADR_0`  → range `0x28..0x2F`.
- **EA = 1**: all 6 bits ADR_5..ADR_0 configurable; ADR_6 = 0. The address must
  avoid the I2C reserved addresses.

| Pin (I2C mode) | ADR bit |
|----------------|---------|
| D6 / MOSI      | ADR_0   |
| D5 / SCK       | ADR_1   |
| D4             | ADR_2   |
| D3             | ADR_3   |
| D2             | ADR_4   |
| D1             | ADR_5   |

**Default: `0x28`** (EA low, all ADR pins low) — `MFRC522_I2C_DEFAULT_ADDR`.
Most MFRC522 breakout boards use this.

## Framing (NXP datasheet §8.1.4.6 / §8.1.4.7)

- **Write**: START, device address (W), register address, data bytes…, STOP.
  Data bytes write to consecutive (auto-incrementing) registers.
- **Read**: write the register address (no data), then a repeated START with
  the read address, then read the data bytes.

The adapter implements the read through `HAL_I2C_Mem_Read` (which produces the
required repeated START). The library's `write_read` platform primitive maps
1:1 to this.

## Adapter API

```c
MFRC522_Status_t MFRC522_STM32_I2C_Init(MFRC522_Handle_t *handle,
                                        I2C_HandleTypeDef *hi2c,
                                        uint8_t dev_addr,          /* 7-bit */
                                        GPIO_TypeDef *rst_port, uint16_t rst_pin,
                                        GPIO_TypeDef *irq_port, uint16_t irq_pin);
```

`dev_addr` is the **7-bit** address (e.g. `MFRC522_I2C_DEFAULT_ADDR`); the
adapter shifts it for the HAL (`<< 1`).

## Error handling / recovery

- **ACK/NACK**: a NACK from the device surfaces as `HAL_ERROR` → the adapter
  returns `MFRC522_ERR_COMM`.
- **Bus busy / arbitration lost / timeout**: surfaced as `MFRC522_ERR_COMM`
  (or `MFRC522_ERR_TIMEOUT` for waits). On STM32, the HAL reports these via
  its error code; the adapter maps any non-OK HAL result to `MFRC522_ERR_COMM`.
- **Recovery**: if the bus is stuck low, toggle SCL manually until SDA is
  released, then re-init the peripheral. This is an application-level action;
  the driver keeps the read/write paths stateless so a retry is safe.
