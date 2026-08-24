/**
 * @file    mfrc522_internal.h
 * @brief   Internal declarations shared across the core implementation.
 *
 * This header is NOT installed/exported: it is only included by the files in
 * src/ and interface/. Applications use only the public headers in include/.
 *
 * It provides:
 *   - small, always-inline platform accessors (ctx, ops, delay, tick, lock)
 *   - the internal debug-log sink (compiled out when MFRC522_ENABLE_DEBUG==0)
 *   - cross-file declarations for the register-level helpers.
 */

#ifndef MFRC522_INTERNAL_H
#define MFRC522_INTERNAL_H

#include "mfrc522.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Debug logging (no printf anywhere in the core)                     */
/* ------------------------------------------------------------------ */
#if MFRC522_ENABLE_DEBUG
void mfrc522_log(MFRC522_Handle_t *handle, MFRC522_LogLevel_t level,
                 const char *message);
#else
#define mfrc522_log(h, l, m) ((void)0)
#endif

/* ------------------------------------------------------------------ */
/* Platform accessors (tiny; the compiler inlines these)              */
/* ------------------------------------------------------------------ */

static inline MFRC522_Status_t mfrc522_platform_valid(const MFRC522_Handle_t *h)
{
    if (h->platform.ops == NULL) return MFRC522_ERR_INVALID_PARAM;
    return MFRC522_OK;
}

static inline void mfrc522_delay_us(MFRC522_Handle_t *h, uint32_t us)
{
    if (h->platform.ops->delay_us != NULL) {
        h->platform.ops->delay_us(h->platform.ctx, us);
    }
}

static inline void mfrc522_delay_ms(MFRC522_Handle_t *h, uint32_t ms)
{
    if (h->platform.ops->delay_ms != NULL) {
        h->platform.ops->delay_ms(h->platform.ctx, ms);
    }
}

static inline uint32_t mfrc522_tick_ms(const MFRC522_Handle_t *h)
{
    if (h->platform.ops->get_tick_ms != NULL) {
        return h->platform.ops->get_tick_ms(h->platform.ctx);
    }
    return 0u;
}

static inline void mfrc522_lock(MFRC522_Handle_t *h)
{
    if (h->platform.ops->lock != NULL) {
        h->platform.ops->lock(h->platform.ctx);
    }
}

static inline void mfrc522_unlock(MFRC522_Handle_t *h)
{
    if (h->platform.ops->unlock != NULL) {
        h->platform.ops->unlock(h->platform.ctx);
    }
}

/* ------------------------------------------------------------------ */
/* Register-level helpers (implemented in mfrc522_registers.c)        */
/* ------------------------------------------------------------------ */

/** Write a command code to CommandReg and (optionally) wait for idle. */
MFRC522_Status_t mfrc522_write_command(MFRC522_Handle_t *h, uint8_t cmd);

/** Poll ComIrqReg until any of `mask` bits is set or the timeout expires. */
MFRC522_Status_t mfrc522_wait_irq(MFRC522_Handle_t *h, uint8_t mask,
                                  uint8_t *irq, uint32_t timeout_ms);

/** Poll ComIrqReg until the IdleIRq bit is set (command finished). */
MFRC522_Status_t mfrc522_wait_idle(MFRC522_Handle_t *h, uint32_t timeout_ms);

/** Flush the FIFO (FIFOLevelReg.FlushBuffer). */
MFRC522_Status_t mfrc522_flush_fifo(MFRC522_Handle_t *h);

/** Read the number of bytes currently in the FIFO. */
MFRC522_Status_t mfrc522_get_fifo_level(MFRC522_Handle_t *h, uint8_t *level);

#ifdef __cplusplus
}
#endif

#endif /* MFRC522_INTERNAL_H */
