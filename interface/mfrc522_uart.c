/**
 * @file    mfrc522_uart.c
 * @brief   Serial UART host-interface transport for the MFRC522.
 *
 * UART framing (NXP datasheet 8.1.3.3) — this interface is unusual and MUST
 * be understood before use (see docs/uart.md):
 *   - 8 data bits, LSB FIRST, no parity, 1 stop bit. Every byte (address AND
 *     data) is transmitted least-significant bit first, so all bytes must be
 *     bit-reversed before TX and after RX.
 *   - The first byte is the "address byte": bit 7 = mode (1 = read, 0 =
 *     write), bit 6 reserved, bits 5:0 = 6-bit register address.
 *         read  address byte = 0x80 | addr   (then bit-reversed)
 *         write address byte = 0x00 | addr   (then bit-reversed)
 *   - Write: address byte then data bytes.
 *   - Read:  address byte, then the device responds with the data bytes.
 *
 * The MFRC522 also exposes a DTRQ flow-control line; this transport performs
 * a simple timed receive and does not require DTRQ. The host UART baud rate
 * must match the MFRC522 SerialSpeedReg (see MFRC522_UART_ApplyBaud).
 */

#include "mfrc522.h"

/* ------------------------------------------------------------------ */
/* Baud-rate divisors for SerialSpeedReg (0x1F), NXP datasheet table. */
/* ------------------------------------------------------------------ */
static const uint8_t MFRC522_UART_BAUD_DIVISORS[MFRC522_UART_BAUD_COUNT] = {
    0xEBu,  /* 0:   9600    */
    0xDAu,  /* 1:  14400    */
    0xCBu,  /* 2:  19200    */
    0xABu,  /* 3:  38400    */
    0x9Au,  /* 4:  57600    */
    0x7Au,  /* 5: 115200    */
    0x74u,  /* 6: 128000    */
    0x5Au,  /* 7: 230400    */
    0x3Au,  /* 8: 460800    */
    0x1Cu,  /* 9: 921600    */
    0x15u   /* 10: 1228800  */
};

/* ------------------------------------------------------------------ */
/* LSB-first bit reversal (branchless).                               */
/* ------------------------------------------------------------------ */
static uint8_t mfrc522_reverse8(uint8_t b)
{
    b = (uint8_t)(((b & 0xF0u) >> 4) | ((b & 0x0Fu) << 4));
    b = (uint8_t)(((b & 0xCCu) >> 2) | ((b & 0x33u) << 2));
    b = (uint8_t)(((b & 0xAAu) >> 1) | ((b & 0x55u) << 1));
    return b;
}

/* ================================================================== */
/*  Register framing                                                  */
/* ================================================================== */

static MFRC522_Status_t uart_read_register(const MFRC522_Transport_t *t,
                                           const MFRC522_Platform_t *p,
                                           uint8_t addr, uint8_t *value)
{
    MFRC522_Status_t status;
    uint8_t address = mfrc522_reverse8((uint8_t)(0x80u | (addr & 0x3Fu)));

    (void)t;

    status = p->ops->transmit(p->ctx, &address, 1u);
    if (status != MFRC522_OK) {
        return status;
    }
    status = p->ops->receive(p->ctx, value, 1u);
    if (status != MFRC522_OK) {
        return status;
    }
    *value = mfrc522_reverse8(*value);
    return MFRC522_OK;
}

static MFRC522_Status_t uart_write_register(const MFRC522_Transport_t *t,
                                            const MFRC522_Platform_t *p,
                                            uint8_t addr, uint8_t value)
{
    uint8_t frame[2];
    (void)t;

    frame[0] = mfrc522_reverse8((uint8_t)(addr & 0x3Fu));
    frame[1] = mfrc522_reverse8(value);
    return p->ops->transmit(p->ctx, frame, 2u);
}

static MFRC522_Status_t uart_read_burst(const MFRC522_Transport_t *t,
                                        const MFRC522_Platform_t *p,
                                        uint8_t addr, uint8_t *data, uint32_t len)
{
    MFRC522_Status_t status;
    uint8_t address = mfrc522_reverse8((uint8_t)(0x80u | (addr & 0x3Fu)));
    uint32_t i;

    (void)t;

    if ((len == 0u) || (len > MFRC522_FIFO_SIZE)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    status = p->ops->transmit(p->ctx, &address, 1u);
    if (status != MFRC522_OK) {
        return status;
    }
    status = p->ops->receive(p->ctx, data, len);
    if (status != MFRC522_OK) {
        return status;
    }
    for (i = 0u; i < len; i++) {
        data[i] = mfrc522_reverse8(data[i]);
    }
    return MFRC522_OK;
}

static MFRC522_Status_t uart_write_burst(const MFRC522_Transport_t *t,
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

    frame[0] = mfrc522_reverse8((uint8_t)(addr & 0x3Fu));
    for (i = 0u; i < len; i++) {
        frame[i + 1u] = mfrc522_reverse8(data[i]);
    }
    return p->ops->transmit(p->ctx, frame, len + 1u);
}

/* ------------------------------------------------------------------ */
/* Baud-rate configuration                                            */
/* ------------------------------------------------------------------ */

MFRC522_Status_t MFRC522_UART_ApplyBaud(MFRC522_Handle_t *handle)
{
    uint8_t baud_index;

    if (handle == NULL) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    baud_index = handle->transport.uart_baud;
    if (baud_index >= MFRC522_UART_BAUD_COUNT) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    return MFRC522_WriteRegister(handle, MFRC522_REG_SERIAL_SPEED,
                                 MFRC522_UART_BAUD_DIVISORS[baud_index]);
}

const MFRC522_TransportOps_t MFRC522_UART_TransportOps = {
    .read_register  = uart_read_register,
    .write_register = uart_write_register,
    .read_burst     = uart_read_burst,
    .write_burst    = uart_write_burst
};
