/**
 * @file    mfrc522_i2c.c
 * @brief   I2C host-interface transport for the MFRC522.
 *
 * I2C framing (NXP datasheet 8.1.4):
 *   - The device address is a 7-bit address (default 0x28 with EA low and all
 *     ADR pins low). The adapter prepends the address+R/W bit and manages the
 *     bus; this transport only supplies the register address + data.
 *   - Write: device-address (W), register address, data bytes (auto-inc).
 *   - Read:  write the register address, then a (repeated) start with the
 *     read address, then read the data bytes (auto-inc).
 *
 * The read sequence is expressed through the platform `write_read` primitive
 * (HAL_I2C_Mem_Read on STM32), which produces the required repeated start.
 * Falls back to transmit+receive when `write_read` is not provided.
 *
 * The MFRC522 supports Fast mode (400 kbit/s) and High-speed mode (3.4 Mbit/s).
 */

#include "mfrc522_transport.h"

/* ================================================================== */
/*  Register framing                                                  */
/* ================================================================== */

static MFRC522_Status_t i2c_read_register(const MFRC522_Transport_t *t,
                                          const MFRC522_Platform_t *p,
                                          uint8_t addr, uint8_t *value)
{
    const uint8_t reg = addr;
    (void)t;

    if (p->ops->write_read != NULL) {
        return p->ops->write_read(p->ctx, &reg, 1u, value, 1u);
    }

    /* Fallback: set the address pointer, then read one byte. */
    {
        MFRC522_Status_t status = p->ops->transmit(p->ctx, &reg, 1u);
        if (status != MFRC522_OK) {
            return status;
        }
        return p->ops->receive(p->ctx, value, 1u);
    }
}

static MFRC522_Status_t i2c_write_register(const MFRC522_Transport_t *t,
                                           const MFRC522_Platform_t *p,
                                           uint8_t addr, uint8_t value)
{
    uint8_t frame[2];
    (void)t;

    frame[0] = addr;
    frame[1] = value;
    return p->ops->transmit(p->ctx, frame, 2u);
}

static MFRC522_Status_t i2c_read_burst(const MFRC522_Transport_t *t,
                                       const MFRC522_Platform_t *p,
                                       uint8_t addr, uint8_t *data, uint32_t len)
{
    const uint8_t reg = addr;
    (void)t;

    if ((len == 0u) || (len > MFRC522_FIFO_SIZE)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    if (p->ops->write_read != NULL) {
        return p->ops->write_read(p->ctx, &reg, 1u, data, len);
    }

    {
        MFRC522_Status_t status = p->ops->transmit(p->ctx, &reg, 1u);
        if (status != MFRC522_OK) {
            return status;
        }
        return p->ops->receive(p->ctx, data, len);
    }
}

static MFRC522_Status_t i2c_write_burst(const MFRC522_Transport_t *t,
                                        const MFRC522_Platform_t *p,
                                        uint8_t addr, const uint8_t *data,
                                        uint32_t len)
{
    uint8_t frame[MFRC522_FIFO_SIZE + 1u];
    uint32_t i;
    (void)t;

    if ((len == 0u) || (len > MFRC522_FIFO_SIZE)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    frame[0] = addr;
    for (i = 0u; i < len; i++) {
        frame[i + 1u] = data[i];
    }
    return p->ops->transmit(p->ctx, frame, len + 1u);
}

const MFRC522_TransportOps_t MFRC522_I2C_TransportOps = {
    .read_register  = i2c_read_register,
    .write_register = i2c_write_register,
    .read_burst     = i2c_read_burst,
    .write_burst    = i2c_write_burst
};
