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

/* ------------------------------------------------------------------ */
/* Internal (unlocked) primitives used by the higher layers.          */
/* ------------------------------------------------------------------ */

/**
 * @brief Hardware CRC coprocessor, unlocked variant (see MFRC522_CalcCRC).
 */
MFRC522_Status_t mfrc522_calc_crc(MFRC522_Handle_t *h,
                                  const uint8_t *data, uint32_t len,
                                  uint16_t *crc);

/**
 * @brief Low-level transceive with full bit-framing control.
 *
 * @param tx, tx_len       Data to transmit.
 * @param rx, rx_len       In: capacity; out: bytes received.
 * @param valid_bits       Out: number of valid bits in the last received byte.
 * @param rx_align         Bit position for the first received bit (0..7).
 * @param tx_last_bits     Number of valid bits in the last TX byte (0..7).
 * @param check_crc        Validate a trailing CRC_A on the response.
 */
MFRC522_Status_t mfrc522_transceive(MFRC522_Handle_t *h,
                                    const uint8_t *tx, uint32_t tx_len,
                                    uint8_t *rx, uint32_t *rx_len,
                                    uint8_t *valid_bits,
                                    uint8_t rx_align, uint8_t tx_last_bits,
                                    uint8_t check_crc, uint32_t timeout_ms);

/**
 * @brief Full anti-collision + select (discovery): resolve the card UID.
 */
MFRC522_Status_t mfrc522_select_full(MFRC522_Handle_t *h,
                                     uint8_t *uid, uint32_t *uid_len,
                                     uint8_t *sak);

/**
 * @brief REQA/WUPA with an explicit timeout (used by card-present checks).
 */
MFRC522_Status_t mfrc522_reqa_or_wupa(MFRC522_Handle_t *h,
                                      uint8_t command,
                                      uint8_t *atqa, uint32_t *atqa_len,
                                      uint32_t timeout_ms);

/**
 * @brief Two-step card request: REQA first (answers IDLE and already-selected
 *        READY cards), WUPA as fallback (wakes HALT'ed cards).
 *
 * Covers every card state that can answer a request; see
 * mfrc522_request_card() in mfrc522_protocol.c for the ISO 14443-3
 * rationale. Returns MFRC522_OK with the ATQA if any request was answered,
 * MFRC522_ERR_TIMEOUT if nothing answered.
 */
MFRC522_Status_t mfrc522_request_card(MFRC522_Handle_t *h,
                                      uint8_t *atqa, uint32_t *atqa_len,
                                      uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* MFRC522_INTERNAL_H */
