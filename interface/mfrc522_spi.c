/**
 * @file    mfrc522_spi.c
 * @brief   SPI host-interface transport for the MFRC522.
 *
 * SPI framing (NXP datasheet 8.1.2):
 *   - The first byte is the "address byte". The 6-bit register address is
 *     shifted left by one; bit 0 is the R/W flag (0 = write, 1 = read).
 *         write address byte = (addr << 1) & 0x7E
 *         read  address byte = (addr << 1) | 0x80
 *   - Write: address byte followed by one or more data bytes.
 *   - Read:  address byte followed by N dummy bytes; the device clocks out
 *     the register contents on MISO. The register address auto-increments
 *     for burst transfers.
 *   - NSS (chip select) must stay low for the whole transaction.
 *
 * The SPI mode is CPOL=0 / CPHA=0 (SPI mode 0), MSB first, 8-bit frames.
 * The MFRC522 supports SPI clocks up to 10 MHz.
 */

#include "mfrc522_transport.h"

/* ================================================================== */
/*  Register framing                                                  */
/* ================================================================== */

static MFRC522_Status_t spi_read_register(const MFRC522_Transport_t *t,
                                          const MFRC522_Platform_t *p,
                                          uint8_t addr, uint8_t *value)
{
    MFRC522_Status_t status;
    uint8_t address = (uint8_t)((addr << 1) | 0x80u);

    (void)t;

    p->ops->cs_assert(p->ctx);
    status = p->ops->transmit(p->ctx, &address, 1u);
    if (status == MFRC522_OK) {
        status = p->ops->receive(p->ctx, value, 1u);
    }
    p->ops->cs_deassert(p->ctx);
    return status;
}

static MFRC522_Status_t spi_write_register(const MFRC522_Transport_t *t,
                                           const MFRC522_Platform_t *p,
                                           uint8_t addr, uint8_t value)
{
    MFRC522_Status_t status;
    uint8_t frame[2];

    (void)t;

    frame[0] = (uint8_t)((addr << 1) & 0x7Eu);
    frame[1] = value;

    p->ops->cs_assert(p->ctx);
    status = p->ops->transmit(p->ctx, frame, 2u);
    p->ops->cs_deassert(p->ctx);
    return status;
}

static MFRC522_Status_t spi_read_burst(const MFRC522_Transport_t *t,
                                       const MFRC522_Platform_t *p,
                                       uint8_t addr, uint8_t *data, uint32_t len)
{
    MFRC522_Status_t status;
    uint8_t address = (uint8_t)((addr << 1) | 0x80u);

    (void)t;

    if ((len == 0u) || (len > MFRC522_FIFO_SIZE)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    p->ops->cs_assert(p->ctx);
    status = p->ops->transmit(p->ctx, &address, 1u);
    if (status == MFRC522_OK) {
        status = p->ops->receive(p->ctx, data, len);
    }
    p->ops->cs_deassert(p->ctx);
    return status;
}

static MFRC522_Status_t spi_write_burst(const MFRC522_Transport_t *t,
                                        const MFRC522_Platform_t *p,
                                        uint8_t addr, const uint8_t *data,
                                        uint32_t len)
{
    MFRC522_Status_t status;
    uint8_t frame[MFRC522_FIFO_SIZE + 1u];
    uint32_t i;

    (void)t;

    if ((len == 0u) || (len > MFRC522_FIFO_SIZE)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    frame[0] = (uint8_t)((addr << 1) & 0x7Eu);
    for (i = 0u; i < len; i++) {
        frame[i + 1u] = data[i];
    }

    p->ops->cs_assert(p->ctx);
    status = p->ops->transmit(p->ctx, frame, len + 1u);
    p->ops->cs_deassert(p->ctx);
    return status;
}

const MFRC522_TransportOps_t MFRC522_SPI_TransportOps = {
    .read_register  = spi_read_register,
    .write_register = spi_write_register,
    .read_burst     = spi_read_burst,
    .write_burst    = spi_write_burst
};
