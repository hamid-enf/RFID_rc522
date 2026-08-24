/**
 * @file    mfrc522_irq.c
 * @brief   MFRC522 interrupt (IRQ) service API.
 *
 * The MFRC522 signals events (timer, error, FIFO alerts, idle, RX, TX) on the
 * ComIrqReg (0x04) flags and mirrors them onto the IRQ pin. This file provides
 * a transport-agnostic dispatcher: the application (or the platform EXTI
 * handler) calls MFRC522_ProcessIRQ(), which reads and clears the latched
 * flags and invokes a registered callback with the source bit-mask.
 *
 * The actual GPIO/EXTI wiring lives in the platform adapter; the core has no
 * knowledge of the interrupt controller.
 */

#include "mfrc522_internal.h"

#if MFRC522_ENABLE_IRQ

MFRC522_Status_t MFRC522_AttachIRQCallback(MFRC522_Handle_t *handle,
                                           MFRC522_IrqCallback_t callback,
                                           void *user)
{
    if (handle == NULL) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    handle->state.irq_callback = callback;
    handle->state.irq_user     = user;
    return MFRC522_OK;
}

MFRC522_Status_t MFRC522_ProcessIRQ(MFRC522_Handle_t *handle)
{
    MFRC522_Status_t status;
    uint8_t irq;

    if (handle == NULL) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    status = MFRC522_ReadRegister(handle, MFRC522_REG_COM_IRQ, &irq);
    if (status != MFRC522_OK) {
        return status;
    }

    /* Only service the real IRQ bits (mask off the Set1 marker). */
    irq &= MFRC522_IRQ_ALL;
    if (irq == 0u) {
        return MFRC522_OK;
    }

    /* Clear the latched interrupt request bits (write 1 to clear). */
    status = MFRC522_WriteRegister(handle, MFRC522_REG_COM_IRQ, irq);
    if (status != MFRC522_OK) {
        return status;
    }

    if (handle->state.irq_callback != NULL) {
        handle->state.irq_callback(handle, irq, handle->state.irq_user);
    }

    return MFRC522_OK;
}

#endif /* MFRC522_ENABLE_IRQ */
