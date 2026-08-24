/**
 * @file    mfrc522_stm32_i2c.c
 * @brief   STM32 HAL adapter for the MFRC522 I2C host interface.
 *
 * The MFRC522 must be strapped to I2C mode (pin I2C = HIGH). The 7-bit slave
 * address defaults to 0x28 (EA low, all ADR pins low); see docs/i2c.md.
 *
 * Register reads are expressed through HAL_I2C_Mem_Read (device address +
 * register address + repeated start + read), which matches the MFRC522's
 * memory-mapped read framing. Writes prepend the register address and use
 * HAL_I2C_Master_Transmit.
 */

#include "mfrc522_stm32_internal.h"
#include "mfrc522_stm32_i2c.h"

MFRC522_STM32_CT_ASSERT(sizeof(MFRC522_STM32_I2C_Context_t) <=
                        MFRC522_PLATFORM_CTX_SIZE);

/* ------------------------------------------------------------------ */
/*  Raw byte I/O                                                      */
/* ------------------------------------------------------------------ */

static uint16_t i2c_dev_addr(void *ctx)
{
    MFRC522_STM32_I2C_Context_t *c = (MFRC522_STM32_I2C_Context_t *)ctx;
    /* HAL wants the 8-bit address: 7-bit address shifted left by one. */
    return (uint16_t)((uint16_t)c->dev_addr << 1);
}

static MFRC522_Status_t i2c_transmit(void *ctx, const uint8_t *tx, uint32_t len)
{
    MFRC522_STM32_I2C_Context_t *c = (MFRC522_STM32_I2C_Context_t *)ctx;

    if (HAL_I2C_Master_Transmit(c->hi2c, i2c_dev_addr(ctx),
                                (uint8_t *)tx, (uint16_t)len,
                                MFRC522_STM32_IO_TIMEOUT) != HAL_OK) {
        return MFRC522_ERR_COMM;
    }
    return MFRC522_OK;
}

static MFRC522_Status_t i2c_write_read(void *ctx, const uint8_t *tx,
                                       uint32_t tx_len, uint8_t *rx,
                                       uint32_t rx_len)
{
    MFRC522_STM32_I2C_Context_t *c = (MFRC522_STM32_I2C_Context_t *)ctx;

    if ((tx_len != 1u) || (tx == NULL)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    /* Mem_Read: write the register address, repeated start, then read. */
    if (HAL_I2C_Mem_Read(c->hi2c, i2c_dev_addr(ctx), tx[0],
                         I2C_MEMADD_SIZE_8BIT, rx, (uint16_t)rx_len,
                         MFRC522_STM32_IO_TIMEOUT) != HAL_OK) {
        return MFRC522_ERR_COMM;
    }
    return MFRC522_OK;
}

/* ------------------------------------------------------------------ */
/*  Platform ops table                                                */
/* ------------------------------------------------------------------ */

static const MFRC522_PlatformOps_t I2C_PLATFORM_OPS = {
    /* timing */
    mfrc522_stm32_delay_us,
    mfrc522_stm32_delay_ms,
    mfrc522_stm32_get_tick_ms,
    /* gpio (CS unused for I2C) */
    NULL,
    NULL,
    mfrc522_stm32_reset_assert,
    mfrc522_stm32_reset_deassert,
    mfrc522_stm32_irq_read,
    /* byte I/O */
    i2c_transmit,
    NULL,                 /* receive:       not used by I2C framing */
    NULL,                 /* transmit_receive */
    i2c_write_read,       /* write_read:    memory-mapped register read */
    /* locking */
    NULL,
    NULL
};

/* ------------------------------------------------------------------ */
/*  Init                                                              */
/* ------------------------------------------------------------------ */

MFRC522_Status_t MFRC522_STM32_I2C_Init(MFRC522_Handle_t *handle,
                                        I2C_HandleTypeDef *hi2c,
                                        uint8_t dev_addr,
                                        GPIO_TypeDef *rst_port, uint16_t rst_pin,
                                        GPIO_TypeDef *irq_port, uint16_t irq_pin)
{
    MFRC522_STM32_I2C_Context_t *c;

    if ((handle == NULL) || (hi2c == NULL)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    c = (MFRC522_STM32_I2C_Context_t *)handle->platform_storage.bytes;

    c->gpio.rst_port = rst_port;
    c->gpio.rst_pin  = rst_pin;
    c->gpio.irq_port = irq_port;
    c->gpio.irq_pin  = irq_pin;
    c->hi2c          = hi2c;
    c->dev_addr      = dev_addr;

    handle->transport.type     = MFRC522_TRANSPORT_I2C;
    handle->transport.i2c_addr = dev_addr;
#if MFRC522_ENABLE_I2C
    handle->transport_ops = &MFRC522_I2C_TransportOps;
#else
    return MFRC522_ERR_NOT_SUPPORTED;
#endif
    handle->platform.ops = &I2C_PLATFORM_OPS;
    handle->platform.ctx = (void *)c;

    mfrc522_stm32_time_init();
    return MFRC522_OK;
}
