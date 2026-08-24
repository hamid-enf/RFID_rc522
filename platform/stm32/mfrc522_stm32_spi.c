/**
 * @file    mfrc522_stm32_spi.c
 * @brief   STM32 HAL adapter for the MFRC522 SPI host interface.
 *
 * Wires a STM32 SPI peripheral (SPI mode 0, MSB first, 8-bit) + software
 * chip-select into the library's platform abstraction. All private state
 * lives inside handle->platform_storage (no globals, no heap).
 *
 * The transport layer (interface/mfrc522_spi.c) performs the address-byte
 * framing; this adapter only provides the raw byte I/O primitives on top of
 * HAL_SPI_Transmit / HAL_SPI_Receive, with the CS held low across them.
 *
 * Polling is used by default. To use interrupt or DMA transfers, configure
 * the SPI_HandleTypeDef accordingly (blocking HAL calls still work and return
 * when the transfer completes); the core library is agnostic to this choice.
 */

#include "mfrc522_stm32_internal.h"
#include "mfrc522_stm32_spi.h"

MFRC522_STM32_CT_ASSERT(sizeof(MFRC522_STM32_SPI_Context_t) <=
                        MFRC522_PLATFORM_CTX_SIZE);

/* ------------------------------------------------------------------ */
/*  Raw byte I/O                                                      */
/* ------------------------------------------------------------------ */

static MFRC522_Status_t spi_transmit(void *ctx, const uint8_t *tx, uint32_t len)
{
    MFRC522_STM32_SPI_Context_t *c = (MFRC522_STM32_SPI_Context_t *)ctx;

    if (HAL_SPI_Transmit(c->hspi, (uint8_t *)tx, (uint16_t)len,
                         MFRC522_STM32_IO_TIMEOUT) != HAL_OK) {
        return MFRC522_ERR_COMM;
    }
    return MFRC522_OK;
}

static MFRC522_Status_t spi_receive(void *ctx, uint8_t *rx, uint32_t len)
{
    MFRC522_STM32_SPI_Context_t *c = (MFRC522_STM32_SPI_Context_t *)ctx;

    if (HAL_SPI_Receive(c->hspi, rx, (uint16_t)len,
                        MFRC522_STM32_IO_TIMEOUT) != HAL_OK) {
        return MFRC522_ERR_COMM;
    }
    return MFRC522_OK;
}

/* ------------------------------------------------------------------ */
/*  Chip-select                                                       */
/* ------------------------------------------------------------------ */

static void spi_cs_assert(void *ctx)
{
    MFRC522_STM32_SPI_Context_t *c = (MFRC522_STM32_SPI_Context_t *)ctx;
    HAL_GPIO_WritePin(c->cs_port, c->cs_pin, GPIO_PIN_RESET);
}

static void spi_cs_deassert(void *ctx)
{
    MFRC522_STM32_SPI_Context_t *c = (MFRC522_STM32_SPI_Context_t *)ctx;
    HAL_GPIO_WritePin(c->cs_port, c->cs_pin, GPIO_PIN_SET);
}

/* ------------------------------------------------------------------ */
/*  Platform ops table                                                */
/* ------------------------------------------------------------------ */

static const MFRC522_PlatformOps_t SPI_PLATFORM_OPS = {
    /* timing */
    mfrc522_stm32_delay_us,
    mfrc522_stm32_delay_ms,
    mfrc522_stm32_get_tick_ms,
    /* gpio */
    spi_cs_assert,
    spi_cs_deassert,
    mfrc522_stm32_reset_assert,
    mfrc522_stm32_reset_deassert,
    mfrc522_stm32_irq_read,
    /* byte I/O */
    spi_transmit,
    spi_receive,
    NULL,                 /* transmit_receive: not used by SPI framing */
    NULL,                 /* write_read:       not used by SPI framing */
    /* locking (single-threaded by default; RTOS users may override) */
    NULL,
    NULL
};

/* ------------------------------------------------------------------ */
/*  Init                                                              */
/* ------------------------------------------------------------------ */

MFRC522_Status_t MFRC522_STM32_SPI_Init(MFRC522_Handle_t *handle,
                                        SPI_HandleTypeDef *hspi,
                                        GPIO_TypeDef *cs_port, uint16_t cs_pin,
                                        GPIO_TypeDef *rst_port, uint16_t rst_pin,
                                        GPIO_TypeDef *irq_port, uint16_t irq_pin)
{
    MFRC522_STM32_SPI_Context_t *c;

    if ((handle == NULL) || (hspi == NULL) || (cs_port == NULL)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    c = (MFRC522_STM32_SPI_Context_t *)handle->platform_storage.bytes;

    c->gpio.rst_port  = rst_port;
    c->gpio.rst_pin   = rst_pin;
    c->gpio.irq_port  = irq_port;
    c->gpio.irq_pin   = irq_pin;
    c->hspi           = hspi;
    c->cs_port        = cs_port;
    c->cs_pin         = cs_pin;

    /* Drive CS idle (high). */
    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);

    handle->transport.type  = MFRC522_TRANSPORT_SPI;
    handle->transport.spi_speed = MFRC522_SPI_SPEED;
#if MFRC522_ENABLE_SPI
    handle->transport_ops  = &MFRC522_SPI_TransportOps;
#else
    return MFRC522_ERR_NOT_SUPPORTED;
#endif
    handle->platform.ops   = &SPI_PLATFORM_OPS;
    handle->platform.ctx   = (void *)c;

    mfrc522_stm32_time_init();
    return MFRC522_OK;
}

void MFRC522_STM32_SPI_SetSpeed(MFRC522_Handle_t *handle, uint8_t speed)
{
    if (handle != NULL) {
        handle->transport.spi_speed = speed;
    }
}
