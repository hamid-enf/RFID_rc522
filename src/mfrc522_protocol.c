/**
 * @file    mfrc522_protocol.c
 * @brief   ISO/IEC 14443-A protocol layer.
 *
 * Implements the host-driven command sequencing the MFRC522 does NOT do on
 * its own: the generic transceive primitive, REQA/WUPA (7-bit short frames),
 * the cascade anti-collision + select loop (with cascade tags and BCC), and
 * HALT. The digital logic of the MFRC522 handles framing/parity/CRC on the
 * air interface; this layer manages the command/response byte flow.
 *
 * References:
 *   - NXP MFRC522 data sheet, §8.6/§10.3 (transceive, MFAuthent)
 *   - ISO/IEC 14443-3, §6 (anti-collision, cascade levels, BCC)
 */

#include "mfrc522_internal.h"

/* ================================================================== */
/*  Generic transceive                                                */
/* ================================================================== */

MFRC522_Status_t mfrc522_transceive(MFRC522_Handle_t *h,
                                    const uint8_t *tx, uint32_t tx_len,
                                    uint8_t *rx, uint32_t *rx_len,
                                    uint8_t *valid_bits,
                                    uint8_t rx_align, uint8_t tx_last_bits,
                                    uint8_t check_crc, uint32_t timeout_ms)
{
    MFRC522_Status_t status;
    uint8_t framing;
    uint8_t error;
    uint8_t fifo_level;
    uint8_t rx_last_bits = 0u;
    uint8_t irq;

    if ((h == NULL) || (tx == NULL) || (tx_len == 0u) || (tx_len > MFRC522_FIFO_SIZE)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    framing = (uint8_t)(((uint8_t)(rx_align & 0x07u) << 4) | (tx_last_bits & 0x07u));

    /* Prepare and start the transceive. */
    status = mfrc522_write_command(h, MFRC522_CMD_IDLE);
    if (status != MFRC522_OK) return status;
    status = MFRC522_WriteRegister(h, MFRC522_REG_COM_IRQ, MFRC522_IRQ_ALL);
    if (status != MFRC522_OK) return status;
    status = mfrc522_flush_fifo(h);
    if (status != MFRC522_OK) return status;
    status = MFRC522_WriteFIFO(h, tx, tx_len);
    if (status != MFRC522_OK) return status;
    status = MFRC522_WriteRegister(h, MFRC522_REG_BIT_FRAMING, framing);
    if (status != MFRC522_OK) return status;
    status = mfrc522_write_command(h, MFRC522_CMD_TRANSCEIVE);
    if (status != MFRC522_OK) return status;
    status = MFRC522_SetBits(h, MFRC522_REG_BIT_FRAMING,
                             MFRC522_BITFRAMING_START_SEND);
    if (status != MFRC522_OK) return status;

    /* Wait for RX and/or idle; the internal timer fires if nothing comes. */
    status = mfrc522_wait_irq(h, MFRC522_IRQ_RX | MFRC522_IRQ_IDLE, &irq,
                              timeout_ms);
    if (status != MFRC522_OK) {
        return status;
    }

    /* Check for fatal errors (buffer overflow / parity / protocol). */
    status = MFRC522_ReadRegister(h, MFRC522_REG_ERROR, &error);
    if (status != MFRC522_OK) {
        return status;
    }
    if ((error & MFRC522_ERR_FATAL_MASK) != 0u) {
        return MFRC522_ERR_COMM;
    }

    /* Read the response back (if requested). */
    if ((rx != NULL) && (rx_len != NULL)) {
        status = mfrc522_get_fifo_level(h, &fifo_level);
        if (status != MFRC522_OK) {
            return status;
        }
        if (fifo_level > *rx_len) {
            return MFRC522_ERR_OVERFLOW;
        }
        status = MFRC522_ReadFIFO(h, rx, fifo_level);
        if (status != MFRC522_OK) {
            return status;
        }
        *rx_len = fifo_level;

        status = MFRC522_ReadRegister(h, MFRC522_REG_CONTROL, &rx_last_bits);
        if (status != MFRC522_OK) {
            return status;
        }
        rx_last_bits &= MFRC522_CONTROL_RX_LAST_BITS_MASK;
        if (valid_bits != NULL) {
            *valid_bits = rx_last_bits;
        }
    }

    /* Report collisions (after the response has been drained). */
    if ((error & MFRC522_ERR_COLL) != 0u) {
        return MFRC522_ERR_COLLISION;
    }

    /* Optional CRC_A validation of the response. */
    if (check_crc && (rx != NULL) && (rx_len != NULL)) {
        uint16_t crc;

        /* A 4-bit response of 0x0A is a MIFARE Classic ACK; 0x00 is a NAK. */
        if ((*rx_len == 1u) && (rx_last_bits == 4u)) {
            return MFRC522_ERR_PROTOCOL;   /* NAK / 4-bit reply */
        }
        if ((*rx_len < 2u) || (rx_last_bits != 0u)) {
            return MFRC522_ERR_CRC;
        }
        status = mfrc522_calc_crc(h, rx, *rx_len - 2u, &crc);
        if (status != MFRC522_OK) {
            return status;
        }
        if ((rx[*rx_len - 2u] != (uint8_t)(crc & 0xFFu)) ||
            (rx[*rx_len - 1u] != (uint8_t)(crc >> 8))) {
            return MFRC522_ERR_CRC;
        }
    }

    return MFRC522_OK;
}

MFRC522_Status_t MFRC522_TransceiveData(MFRC522_Handle_t *handle,
                                        const uint8_t *tx, uint32_t tx_len,
                                        uint8_t *rx, uint32_t *rx_len,
                                        uint32_t timeout_ms)
{
    return mfrc522_transceive(handle, tx, tx_len, rx, rx_len, NULL,
                              0u /* rx_align */, 0u /* tx_last_bits */,
                              0u /* check_crc */, timeout_ms);
}

/* ================================================================== */
/*  REQA / WUPA (7-bit short frames)                                  */
/* ================================================================== */

MFRC522_Status_t mfrc522_reqa_or_wupa(MFRC522_Handle_t *h,
                                      uint8_t command,
                                      uint8_t *atqa, uint32_t *atqa_len,
                                      uint32_t timeout_ms)
{
    MFRC522_Status_t status;
    uint8_t rx[4];
    uint32_t rx_len = sizeof(rx);
    uint8_t valid_bits = 0u;

    if ((atqa == NULL) || (atqa_len == NULL) || (*atqa_len < 2u)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    /* Bits received after a collision are cleared. */
    status = MFRC522_ClearBits(h, MFRC522_REG_COLL, MFRC522_COLL_VALUES_AFTER_COLL);
    if (status != MFRC522_OK) return status;

    /* REQA/WUPA use the short (7-bit) frame format. */
    status = mfrc522_transceive(h, &command, 1u, rx, &rx_len, &valid_bits,
                                0u /* rx_align */, 7u /* tx_last_bits */,
                                0u /* check_crc */, timeout_ms);
    if (status != MFRC522_OK) {
        return status;
    }

    if ((rx_len != 2u) || (valid_bits != 0u)) {
        return MFRC522_ERR_PROTOCOL;   /* ATQA must be exactly 16 bits */
    }

    atqa[0] = rx[0];
    atqa[1] = rx[1];
    *atqa_len = 2u;
    return MFRC522_OK;
}

MFRC522_Status_t mfrc522_request_card(MFRC522_Handle_t *h,
                                      uint8_t *atqa, uint32_t *atqa_len,
                                      uint32_t timeout_ms)
{
    MFRC522_Status_t status;

    if ((atqa == NULL) || (atqa_len == NULL) || (*atqa_len < 2u)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    /* ISO/IEC 14443-3 type A request semantics:
     *   - REQA (0x26) is answered by cards in the IDLE state and by cards
     *     that are already selected (READY), where it acts as the RTSA
     *     command. A READY card IGNORES WUPA.
     *   - WUPA (0x52) additionally wakes cards in the HALT state.
     * Sending REQA first keeps continuous polling of a non-halted card
     * working; the WUPA fallback then also detects halted cards. */
    status = mfrc522_reqa_or_wupa(h, MFRC522_PICC_REQA, atqa, atqa_len,
                                  timeout_ms);
    if (status == MFRC522_OK) {
        return MFRC522_OK;
    }
    if (status != MFRC522_ERR_TIMEOUT) {
        return status;   /* protocol / device error: a retry is pointless */
    }

    return mfrc522_reqa_or_wupa(h, MFRC522_PICC_WUPA, atqa, atqa_len,
                                timeout_ms);
}

MFRC522_Status_t MFRC522_REQA(MFRC522_Handle_t *handle,
                              uint8_t *atqa, uint32_t *atqa_len)
{
    return mfrc522_reqa_or_wupa(handle, MFRC522_PICC_REQA, atqa, atqa_len,
                                handle->config.timeout_ms);
}

MFRC522_Status_t MFRC522_WUPA(MFRC522_Handle_t *handle,
                              uint8_t *atqa, uint32_t *atqa_len)
{
    return mfrc522_reqa_or_wupa(handle, MFRC522_PICC_WUPA, atqa, atqa_len,
                                handle->config.timeout_ms);
}

/* ================================================================== */
/*  Anti-collision / select (cascade)                                 */
/* ================================================================== */

/**
 * @brief Anti-collision + select for one cascade level.
 *
 * Faithful implementation of the ISO/IEC 14443-3 anti-collision loop:
 * the SEL command is sent with an incrementally extended known-UID prefix;
 * on collision the MFRC522's CollReg reports the first diverging bit, which
 * is forced to 1; when all 32 bits of the level are known a SELECT with BCC
 * and CRC_A is sent and the SAK (+CRC_A) is checked.
 */
static MFRC522_Status_t mfrc522_select_level(MFRC522_Handle_t *h,
                                             uint8_t cascade,
                                             uint8_t *out_fragment,
                                             uint8_t *out_fragment_len,
                                             uint8_t *out_sak)
{
    uint8_t sel_cmd;
    uint8_t buffer[9];
    uint8_t buffer_used;
    uint8_t rx_align;
    uint8_t tx_last_bits;
    uint32_t response_len;
    uint8_t current_level_known_bits;
    uint8_t index;
    uint8_t count;
    uint8_t check_bit;
    uint8_t collision_pos;
    uint8_t coll_reg;
    uint8_t select_done = 0u;
    uint8_t loop;
    uint16_t crc;
    MFRC522_Status_t status;

    switch (cascade) {
        case 0:  sel_cmd = MFRC522_PICC_SELECT_TAG_1; break;
        case 1:  sel_cmd = MFRC522_PICC_SELECT_TAG_2; break;
        default: sel_cmd = MFRC522_PICC_SELECT_TAG_3; break;
    }

    /* Bits received after a collision are cleared. */
    status = MFRC522_ClearBits(h, MFRC522_REG_COLL, MFRC522_COLL_VALUES_AFTER_COLL);
    if (status != MFRC522_OK) return status;

    buffer[0] = sel_cmd;
    current_level_known_bits = 0u;

    for (loop = 0u; (loop < 32u) && (select_done == 0u); loop++) {
        if (current_level_known_bits >= 32u) {
            /* SELECT: all UID bits of this level are known. */
            buffer[1] = 0x70u;                       /* NVB = 7 whole bytes   */
            buffer[6] = (uint8_t)(buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5]);
            status = mfrc522_calc_crc(h, buffer, 7u, &crc);
            if (status != MFRC522_OK) return status;
            buffer[7] = (uint8_t)(crc & 0xFFu);
            buffer[8] = (uint8_t)(crc >> 8);
            tx_last_bits = 0u;
            buffer_used = 9u;
            response_len = 3u;                       /* SAK + CRC_A          */
        } else {
            /* ANTICOLLISION: send the known prefix. */
            tx_last_bits = (uint8_t)(current_level_known_bits % 8u);
            count = (uint8_t)(current_level_known_bits / 8u);
            index = (uint8_t)(2u + count);
            buffer[1] = (uint8_t)((index << 4) | tx_last_bits);
            buffer_used = (uint8_t)(index + ((tx_last_bits != 0u) ? 1u : 0u));
            response_len = (uint8_t)(sizeof(buffer) - index);
        }

        rx_align = tx_last_bits;
        status = MFRC522_WriteRegister(h, MFRC522_REG_BIT_FRAMING,
                                       (uint8_t)((rx_align << 4) | tx_last_bits));
        if (status != MFRC522_OK) return status;

        /* Transceive; the response lands in buffer[2..] (or buffer[6..] for
         * the SELECT, where SAK+CRC overwrite BCC+CRC). */
        status = mfrc522_transceive(h, buffer, buffer_used,
                                    (current_level_known_bits >= 32u) ? &buffer[6] : &buffer[2],
                                    &response_len, &tx_last_bits,
                                    rx_align, tx_last_bits, 0u, h->config.timeout_ms);

        if (status == MFRC522_ERR_COLLISION) {
            status = MFRC522_ReadRegister(h, MFRC522_REG_COLL, &coll_reg);
            if (status != MFRC522_OK) return status;
            if ((coll_reg & MFRC522_COLL_POS_NOT_VALID) != 0u) {
                return MFRC522_ERR_COLLISION;
            }
            collision_pos = (uint8_t)(coll_reg & MFRC522_COLL_POS_MASK);
            if (collision_pos == 0u) {
                collision_pos = 32u;
            }
            if (collision_pos <= current_level_known_bits) {
                return MFRC522_ERR_INTERNAL;   /* no progress */
            }
            /* Force the colliding bit to 1 and retry. */
            current_level_known_bits = collision_pos;
            count = (uint8_t)(current_level_known_bits % 8u);
            check_bit = (uint8_t)((current_level_known_bits - 1u) % 8u);
            index = (uint8_t)(1u + (current_level_known_bits / 8u) + ((count != 0u) ? 1u : 0u));
            buffer[index] |= (uint8_t)(1u << check_bit);
        } else if (status != MFRC522_OK) {
            return status;
        } else {
            if (current_level_known_bits >= 32u) {
                select_done = 1u;
            } else {
                current_level_known_bits = 32u;
            }
        }
    }

    if (select_done == 0u) {
        return MFRC522_ERR_TIMEOUT;
    }

    /* Copy the resolved UID fragment (strip a leading cascade tag). */
    if (buffer[2] == MFRC522_PICC_CT) {
        out_fragment[0] = buffer[3];
        out_fragment[1] = buffer[4];
        out_fragment[2] = buffer[5];
        *out_fragment_len = 3u;
    } else {
        out_fragment[0] = buffer[2];
        out_fragment[1] = buffer[3];
        out_fragment[2] = buffer[4];
        out_fragment[3] = buffer[5];
        *out_fragment_len = 4u;
    }

    /* Validate the SAK response (SAK + CRC_A). */
    if ((response_len != 3u) || (tx_last_bits != 0u)) {
        return MFRC522_ERR_PROTOCOL;
    }
    status = mfrc522_calc_crc(h, &buffer[6], 1u, &crc);
    if (status != MFRC522_OK) return status;
    if ((buffer[7] != (uint8_t)(crc & 0xFFu)) ||
        (buffer[8] != (uint8_t)(crc >> 8))) {
        return MFRC522_ERR_CRC;
    }

    *out_sak = buffer[6];
    return MFRC522_OK;
}

MFRC522_Status_t mfrc522_select_full(MFRC522_Handle_t *h,
                                     uint8_t *uid, uint32_t *uid_len,
                                     uint8_t *sak)
{
    MFRC522_Status_t status;
    uint8_t fragment[4];
    uint8_t fragment_len;
    uint8_t level_sak;
    uint8_t cascade;
    uint8_t done = 0u;
    uint32_t used = 0u;
    uint8_t i;

    if ((h == NULL) || (uid == NULL) || (uid_len == NULL) || (sak == NULL)) {
        return MFRC522_ERR_INVALID_PARAM;
    }
    if (*uid_len < 4u) {
        return MFRC522_ERR_OVERFLOW;
    }

    for (cascade = 0u; (cascade < 3u) && (done == 0u); cascade++) {
        status = mfrc522_select_level(h, cascade, fragment, &fragment_len,
                                      &level_sak);
        if (status != MFRC522_OK) {
            return status;
        }

        if ((used + fragment_len) > *uid_len) {
            return MFRC522_ERR_OVERFLOW;
        }
        for (i = 0u; i < fragment_len; i++) {
            uid[used + i] = fragment[i];
        }
        used += fragment_len;

        if ((level_sak & 0x04u) != 0u) {
            /* Cascade bit set: UID continues in the next level. */
            done = 0u;
        } else {
            done = 1u;
            *sak = level_sak;
        }
    }

    if (done == 0u) {
        return MFRC522_ERR_PROTOCOL;   /* UID longer than 10 bytes */
    }

    *uid_len = used;
    return MFRC522_OK;
}

MFRC522_Status_t MFRC522_Anticollision(MFRC522_Handle_t *handle,
                                       uint8_t cascade,
                                       uint8_t *uid, uint32_t *uid_len,
                                       uint8_t *sak)
{
    uint8_t fragment[4];
    uint8_t fragment_len;
    uint8_t level_sak;
    uint8_t i;
    MFRC522_Status_t status;

    if ((handle == NULL) || (uid == NULL) || (uid_len == NULL)) {
        return MFRC522_ERR_INVALID_PARAM;
    }
    if (cascade > 2u) {
        return MFRC522_ERR_INVALID_PARAM;
    }
    if (*uid_len < 4u) {
        return MFRC522_ERR_OVERFLOW;
    }

    status = mfrc522_select_level(handle, cascade, fragment, &fragment_len,
                                  &level_sak);
    if (status != MFRC522_OK) {
        return status;
    }

    for (i = 0u; i < fragment_len; i++) {
        uid[i] = fragment[i];
    }
    *uid_len = fragment_len;
    if (sak != NULL) {
        *sak = level_sak;
    }
    return MFRC522_OK;
}

MFRC522_Status_t MFRC522_SelectCard(MFRC522_Handle_t *handle,
                                    uint8_t *uid, uint32_t *uid_len,
                                    uint8_t *sak)
{
    return mfrc522_select_full(handle, uid, uid_len, sak);
}

/* ================================================================== */
/*  HALT                                                              */
/* ================================================================== */

MFRC522_Status_t MFRC522_HaltTag(MFRC522_Handle_t *handle)
{
    uint8_t buffer[4];
    uint16_t crc;
    MFRC522_Status_t status;

    if (handle == NULL) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    buffer[0] = MFRC522_PICC_HALT;
    buffer[1] = 0x00u;
    status = mfrc522_calc_crc(handle, buffer, 2u, &crc);
    if (status != MFRC522_OK) {
        return status;
    }
    buffer[2] = (uint8_t)(crc & 0xFFu);
    buffer[3] = (uint8_t)(crc >> 8);

    /* A successful HALT gets no answer: a timeout is the expected outcome. */
    status = MFRC522_TransceiveData(handle, buffer, 4u, NULL, NULL,
                                    handle->config.timeout_ms);
    if (status == MFRC522_ERR_TIMEOUT) {
        return MFRC522_OK;
    }
    if (status == MFRC522_OK) {
        return MFRC522_ERR_PROTOCOL;   /* card answered: not acknowledged */
    }
    return status;
}
