/**
 * @file    mfrc522_protocol.h
 * @brief   ISO/IEC 14443-A contactless protocol layer.
 *
 * Implements the low-level building blocks the MFRC522 exposes in hardware:
 * REQA/WUPA, anti-collision, select, halt, CRC_A and the generic
 * "transceive" primitive. The high-level convenience wrappers (card
 * detection, UID read, card info) are declared in mfrc522.h and are built on
 * top of these primitives.
 *
 * The MFRC522 handles 14443-A framing, parity and the on-air CRC in its
 * digital logic; this layer only manages the command/response sequencing.
 */

#ifndef MFRC522_PROTOCOL_H
#define MFRC522_PROTOCOL_H

#include <stdint.h>
#include "mfrc522_types.h"
#include "mfrc522_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* MFRC522_Handle_t is forward-declared in mfrc522_types.h. */

/* ================================================================== */
/*  ISO/IEC 14443-A command bytes                                     */
/* ================================================================== */
#define MFRC522_PICC_REQA           (0x26u)  /**< Request command, type A.       */
#define MFRC522_PICC_WUPA           (0x52u)  /**< Wake-up command, type A.       */
#define MFRC522_PICC_SELECT_TAG_1   (0x93u)  /**< Select cascade level 1.        */
#define MFRC522_PICC_SELECT_TAG_2   (0x95u)  /**< Select cascade level 2.        */
#define MFRC522_PICC_SELECT_TAG_3   (0x97u)  /**< Select cascade level 3.        */
#define MFRC522_PICC_HALT           (0x50u)  /**< Halt command.                  */
#define MFRC522_PICC_CT             (0x88u)  /**< Cascade-tag value (AntiColl NVB=1). */

/** Default key used by the MFRC522 for the "no-auth" self-test. */
#define MFRC522_SELFTEST_KEY        { 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu }

/* ================================================================== */
/*  CRC_A                                                            */
/* ================================================================== */

/**
 * @brief Compute the ISO/IEC 14443-A CRC (CRC_A, poly 0x1021, init 0x6363).
 *
 * Pure software implementation. Does not touch the MFRC522 and is therefore
 * useful for validation, for host-side framing checks and for tests. For
 * on-the-fly card I/O the hardware CRC coprocessor (MFRC522_CalcCRC) is
 * preferred — see the register-driver API.
 *
 * @param data       Input bytes (may be NULL only if len == 0).
 * @param len        Number of input bytes.
 * @param crc_out    Receives the 16-bit CRC value (host byte order).
 */
void MFRC522_CRC_A(const uint8_t *data, uint32_t len, uint16_t *crc_out);

/* ================================================================== */
/*  Low-level transceive                                             */
/* ================================================================== */

/**
 * @brief Send a command to the PICC and receive its reply (blocking).
 *
 * Writes @p tx_len bytes to the FIFO, runs the TRANSCEIVE command and drains
 * the received bytes. Applies the configured timeout.
 *
 * @param handle     Reader handle.
 * @param tx         Bytes to transmit (NULL only if tx_len == 0).
 * @param tx_len     Number of transmit bytes.
 * @param rx         Receive buffer.
 * @param rx_len     In: capacity of rx; out: number of valid bytes.
 * @param timeout_ms Timeout (0 = use handle default).
 * @return           MFRC522_OK or an error code.
 */
MFRC522_Status_t MFRC522_TransceiveData(MFRC522_Handle_t *handle,
                                        const uint8_t *tx, uint32_t tx_len,
                                        uint8_t *rx, uint32_t *rx_len,
                                        uint32_t timeout_ms);

/* ================================================================== */
/*  ISO/IEC 14443-A commands                                         */
/* ================================================================== */

/**
 * @brief Send REQA (0x26) and capture ATQA.
 * @param atqa      Receives the 2-byte ATQA (may be NULL).
 * @param atqa_len  In: capacity; out: bytes stored (typically 2).
 */
MFRC522_Status_t MFRC522_REQA(MFRC522_Handle_t *handle,
                              uint8_t *atqa, uint32_t *atqa_len);

/**
 * @brief Send WUPA (0x52) and capture ATQA.
 */
MFRC522_Status_t MFRC522_WUPA(MFRC522_Handle_t *handle,
                              uint8_t *atqa, uint32_t *atqa_len);

/**
 * @brief Run the anti-collision loop for one cascade level.
 *
 * Performs the anti-collision + select of a single cascade level and returns
 * that level's UID fragment (3 or 4 bytes, cascade tag already stripped) and
 * the SAK. If the SAK has bit 2 set (0x04), the UID continues in the next
 * cascade level and the caller should call again with cascade+1.
 *
 * @param cascade   0..2  => SEL 0x93 / 0x95 / 0x97.
 * @param uid       Receives the level's UID fragment (3 or 4 bytes).
 * @param uid_len   In: capacity (>= 4); out: bytes stored.
 * @param sak       Receives the SAK of this level's select (may be NULL).
 * @return          MFRC522_OK, MFRC522_ERR_COLLISION (unresolvable), etc.
 */
MFRC522_Status_t MFRC522_Anticollision(MFRC522_Handle_t *handle,
                                       uint8_t cascade,
                                       uint8_t *uid, uint32_t *uid_len,
                                       uint8_t *sak);

/**
 * @brief Full anti-collision + select: resolve the complete card UID.
 *
 * Runs the cascade anti-collision across all levels (handling cascade tags
 * and BCC) and returns the complete 4/7/10-byte UID and the final SAK.
 * The card must be in READY state (after REQA/WUPA).
 *
 * @param uid       Receives the complete UID bytes.
 * @param uid_len   In: capacity (>= MFRC522_UID_MAX_LEN); out: 4/7/10.
 * @param sak       Receives the final SAK.
 */
MFRC522_Status_t MFRC522_SelectCard(MFRC522_Handle_t *handle,
                                    uint8_t *uid, uint32_t *uid_len,
                                    uint8_t *sak);

/**
 * @brief Put the card in HALT state (0x50 + CRC_A).
 */
MFRC522_Status_t MFRC522_HaltTag(MFRC522_Handle_t *handle);

#ifdef __cplusplus
}
#endif

#endif /* MFRC522_PROTOCOL_H */
