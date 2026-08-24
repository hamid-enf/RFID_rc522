/**
 * @file    mfrc522_registers.c
 * @brief   MFRC522 register driver: register I/O, bit access, FIFO and the
 *          hardware CRC coprocessor.
 *
 * Everything here goes through the transport ops table, so this layer is
 * completely host-interface and MCU agnostic. All blocking waits are bounded
 * by the configured timeout.
 */

#include "mfrc522_internal.h"

/* ================================================================== */
/*  Register read / write                                             */
/* ================================================================== */

MFRC522_Status_t MFRC522_ReadRegister(MFRC522_Handle_t *handle,
                                      uint8_t addr, uint8_t *value)
{
    if ((handle == NULL) || (value == NULL) ||
        (handle->transport_ops == NULL) ||
        (handle->transport_ops->read_register == NULL)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    return handle->transport_ops->read_register(&handle->transport,
                                                &handle->platform,
                                                addr, value);
}

MFRC522_Status_t MFRC522_WriteRegister(MFRC522_Handle_t *handle,
                                       uint8_t addr, uint8_t value)
{
    if ((handle == NULL) || (handle->transport_ops == NULL) ||
        (handle->transport_ops->write_register == NULL)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    return handle->transport_ops->write_register(&handle->transport,
                                                 &handle->platform,
                                                 addr, value);
}

MFRC522_Status_t MFRC522_SetBits(MFRC522_Handle_t *handle,
                                 uint8_t addr, uint8_t mask)
{
    MFRC522_Status_t status;
    uint8_t value;

    status = MFRC522_ReadRegister(handle, addr, &value);
    if (status != MFRC522_OK) {
        return status;
    }
    value = (uint8_t)(value | mask);

    return MFRC522_WriteRegister(handle, addr, value);
}

MFRC522_Status_t MFRC522_ClearBits(MFRC522_Handle_t *handle,
                                   uint8_t addr, uint8_t mask)
{
    MFRC522_Status_t status;
    uint8_t value;

    status = MFRC522_ReadRegister(handle, addr, &value);
    if (status != MFRC522_OK) {
        return status;
    }
    value = (uint8_t)(value & (uint8_t)~mask);

    return MFRC522_WriteRegister(handle, addr, value);
}

/* ================================================================== */
/*  FIFO                                                              */
/* ================================================================== */

MFRC522_Status_t MFRC522_ReadFIFO(MFRC522_Handle_t *handle,
                                  uint8_t *data, uint32_t len)
{
    if ((handle == NULL) || (data == NULL) ||
        (len > MFRC522_FIFO_SIZE) || (handle->transport_ops == NULL) ||
        (handle->transport_ops->read_burst == NULL)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    return handle->transport_ops->read_burst(&handle->transport,
                                             &handle->platform,
                                             MFRC522_REG_FIFO_DATA,
                                             data, len);
}

MFRC522_Status_t MFRC522_WriteFIFO(MFRC522_Handle_t *handle,
                                   const uint8_t *data, uint32_t len)
{
    if ((handle == NULL) || (data == NULL) ||
        (len > MFRC522_FIFO_SIZE) || (handle->transport_ops == NULL) ||
        (handle->transport_ops->write_burst == NULL)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    return handle->transport_ops->write_burst(&handle->transport,
                                              &handle->platform,
                                              MFRC522_REG_FIFO_DATA,
                                              data, len);
}

MFRC522_Status_t mfrc522_flush_fifo(MFRC522_Handle_t *h)
{
    return MFRC522_WriteRegister(h, MFRC522_REG_FIFO_LEVEL,
                                 MFRC522_FIFO_LEVEL_FLUSH);
}

MFRC522_Status_t mfrc522_get_fifo_level(MFRC522_Handle_t *h, uint8_t *level)
{
    MFRC522_Status_t status;
    uint8_t value;

    status = MFRC522_ReadRegister(h, MFRC522_REG_FIFO_LEVEL, &value);
    if (status != MFRC522_OK) {
        return status;
    }
    *level = (uint8_t)(value & MFRC522_FIFO_LEVEL_MASK);
    return MFRC522_OK;
}

/* ================================================================== */
/*  Command control & IRQ polling                                     */
/* ================================================================== */

MFRC522_Status_t mfrc522_write_command(MFRC522_Handle_t *h, uint8_t cmd)
{
    return MFRC522_WriteRegister(h, MFRC522_REG_COMMAND, cmd);
}

MFRC522_Status_t mfrc522_wait_irq(MFRC522_Handle_t *h, uint8_t mask,
                                  uint8_t *irq, uint32_t timeout_ms)
{
    MFRC522_Status_t status;
    uint8_t value = 0u;
    uint32_t deadline;
    uint32_t now;

    if (timeout_ms == 0u) {
        timeout_ms = h->config.timeout_ms;
    }
    if (timeout_ms == 0u) {
        timeout_ms = MFRC522_DEFAULT_TIMEOUT_MS;
    }

    deadline = mfrc522_tick_ms(h) + timeout_ms;

    for (;;) {
        status = MFRC522_ReadRegister(h, MFRC522_REG_COM_IRQ, &value);
        if (status != MFRC522_OK) {
            return status;
        }

        if ((value & mask) != 0u) {
            break;
        }

        /* TimerIRq without our mask bit set => the internal timer expired. */
        if ((value & MFRC522_IRQ_TIMER) != 0u) {
            return MFRC522_ERR_TIMEOUT;
        }

        now = mfrc522_tick_ms(h);
        if ((int32_t)(deadline - now) <= 0) {
            return MFRC522_ERR_TIMEOUT;
        }
    }

    if (irq != NULL) {
        *irq = value;
    }
    return MFRC522_OK;
}

MFRC522_Status_t mfrc522_wait_idle(MFRC522_Handle_t *h, uint32_t timeout_ms)
{
    return mfrc522_wait_irq(h, MFRC522_IRQ_IDLE, NULL, timeout_ms);
}

/* ================================================================== */
/*  Hardware CRC coprocessor                                          */
/* ================================================================== */

MFRC522_Status_t MFRC522_CalcCRC(MFRC522_Handle_t *handle,
                                 const uint8_t *data, uint32_t len,
                                 uint16_t *crc)
{
    MFRC522_Status_t status;
    uint8_t irq;
    uint8_t msb;
    uint8_t lsb;
    uint32_t deadline;

    if ((handle == NULL) || (crc == NULL) || (data == NULL) ||
        (len == 0u) || (len > MFRC522_FIFO_SIZE)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    mfrc522_lock(handle);

    /* Stop any active command and clear the CRC-complete IRQ. */
    status = mfrc522_write_command(handle, MFRC522_CMD_IDLE);
    if (status != MFRC522_OK) goto done;
    status = MFRC522_WriteRegister(handle, MFRC522_REG_DIV_IRQ,
                                   MFRC522_DIV_IRQ_CRC);
    if (status != MFRC522_OK) goto done;
    status = mfrc522_flush_fifo(handle);
    if (status != MFRC522_OK) goto done;
    status = MFRC522_WriteFIFO(handle, data, len);
    if (status != MFRC522_OK) goto done;

    /* Start the CRC calculation. */
    status = mfrc522_write_command(handle, MFRC522_CMD_CALC_CRC);
    if (status != MFRC522_OK) goto done;

    /* Poll DivIrqReg.CRCIRq (the CRC-complete flag is in DivIrqReg). */
    deadline = mfrc522_tick_ms(handle) + handle->config.timeout_ms;
    for (;;) {
        status = MFRC522_ReadRegister(handle, MFRC522_REG_DIV_IRQ, &irq);
        if (status != MFRC522_OK) goto done;
        if ((irq & MFRC522_DIV_IRQ_CRC) != 0u) {
            break;
        }
        if ((int32_t)(deadline - mfrc522_tick_ms(handle)) <= 0) {
            status = MFRC522_ERR_TIMEOUT;
            goto done;
        }
    }

    status = mfrc522_write_command(handle, MFRC522_CMD_IDLE);
    if (status != MFRC522_OK) goto done;

    status = MFRC522_ReadRegister(handle, MFRC522_REG_CRC_RESULT_LSB, &lsb);
    if (status != MFRC522_OK) goto done;
    status = MFRC522_ReadRegister(handle, MFRC522_REG_CRC_RESULT_MSB, &msb);
    if (status != MFRC522_OK) goto done;

    *crc = (uint16_t)(((uint16_t)msb << 8) | (uint16_t)lsb);

done:
    mfrc522_unlock(handle);
    return status;
}
