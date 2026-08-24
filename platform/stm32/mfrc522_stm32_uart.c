/**
 * @file    mfrc522_stm32_uart.c
 * @brief   STM32 HAL adapter for the MFRC522 serial UART host interface.
 *
 * The MFRC522 UART interface is logic-level, 8N1, LSB-first (the transport
 * layer performs the bit reversal). The MCU UART must be configured to a
 * matching baud rate from the MFRC522-supported set (see docs/uart.md).
 *
 * Reads are simple timed receives (HAL_UART_Receive); the DTRQ flow-control
 * line is optional and not required by this adapter.
 */

#include "mfrc522_stm32_internal.h"
#include "mfrc522_stm32_uart.h"

MFRC522_STM32_CT_ASSERT(sizeof(MFRC522_STM32_UART_Context_t) <=
                        MFRC522_PLATFORM_CTX_SIZE);

/* ------------------------------------------------------------------ */
/*  Raw byte I/O                                                      */
/* ------------------------------------------------------------------ */

static MFRC522_Status_t uart_transmit(void *ctx, const uint8_t *tx, uint32_t len)
{
    MFRC522_STM32_UART_Context_t *c = (MFRC522_STM32_UART_Context_t *)ctx;

    if (HAL_UART_Transmit(c->huart, (uint8_t *)tx, (uint16_t)len,
                          MFRC522_STM32_IO_TIMEOUT) != HAL_OK) {
        return MFRC522_ERR_COMM;
    }
    return MFRC522_OK;
}

static MFRC522_Status_t uart_receive(void *ctx, uint8_t *rx, uint32_t len)
{
    MFRC522_STM32_UART_Context_t *c = (MFRC522_STM32_UART_Context_t *)ctx;

    if (HAL_UART_Receive(c->huart, rx, (uint16_t)len,
                         MFRC522_STM32_IO_TIMEOUT) != HAL_OK) {
        return MFRC522_ERR_COMM;
    }
    return MFRC522_OK;
}

/* ------------------------------------------------------------------ */
/*  Platform ops table                                                */
/* ------------------------------------------------------------------ */

static const MFRC522_PlatformOps_t UART_PLATFORM_OPS = {
    /* timing */
    mfrc522_stm32_delay_us,
    mfrc522_stm32_delay_ms,
    mfrc522_stm32_get_tick_ms,
    /* gpio (CS unused for UART) */
    NULL,
    NULL,
    mfrc522_stm32_reset_assert,
    mfrc522_stm32_reset_deassert,
    mfrc522_stm32_irq_read,
    /* byte I/O */
    uart_transmit,
    uart_receive,
    NULL,                 /* transmit_receive: not used by UART framing */
    NULL,                 /* write_read:       not used by UART framing */
    /* locking */
    NULL,
    NULL
};

/* ------------------------------------------------------------------ */
/*  Init                                                              */
/* ------------------------------------------------------------------ */

MFRC522_Status_t MFRC522_STM32_UART_Init(MFRC522_Handle_t *handle,
                                         UART_HandleTypeDef *huart,
                                         uint8_t baud,
                                         GPIO_TypeDef *rst_port, uint16_t rst_pin,
                                         GPIO_TypeDef *irq_port, uint16_t irq_pin)
{
    MFRC522_STM32_UART_Context_t *c;

    if ((handle == NULL) || (huart == NULL)) {
        return MFRC522_ERR_INVALID_PARAM;
    }
    if (baud >= MFRC522_UART_BAUD_COUNT) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    c = (MFRC522_STM32_UART_Context_t *)handle->platform_storage.bytes;

    c->gpio.rst_port = rst_port;
    c->gpio.rst_pin  = rst_pin;
    c->gpio.irq_port = irq_port;
    c->gpio.irq_pin  = irq_pin;
    c->huart         = huart;

    handle->transport.type      = MFRC522_TRANSPORT_UART;
    handle->transport.uart_baud = baud;
#if MFRC522_ENABLE_UART
    handle->transport_ops = &MFRC522_UART_TransportOps;
#else
    return MFRC522_ERR_NOT_SUPPORTED;
#endif
    handle->platform.ops = &UART_PLATFORM_OPS;
    handle->platform.ctx = (void *)c;

    mfrc522_stm32_time_init();
    return MFRC522_OK;
}
