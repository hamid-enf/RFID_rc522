/**
 * @file    mfrc522_mifare.h
 * @brief   MIFARE Classic / Ultralight high-level API.
 *
 * This header is compiled only when MFRC522_ENABLE_MIFARE == 1. It provides
 * the sector/block oriented operations that are actually supported by the
 * MFRC522 silicon (MIFARE Classic auth + read/write, and the raw command
 * path used by MIFARE Ultralight / NTAG).
 *
 * IMPORTANT — capability boundaries:
 *   - The MFRC522 is a *transparent* frontend: it implements ISO/IEC 14443-A
 *     framing, CRC and the Crypto1 authentication command (MFAuthent) in
 *     hardware. Everything above that (block/sector semantics, value-block
 *     formatting) is *protocol*, handled by this library, NOT by the IC.
 *   - MIFARE DESFire / Plus / ISO 14443-4 (T=CL) command sets are outside the
 *     scope of this library; only the plaintext 14443-3 / MIFARE command set
 *     is provided.
 *   - Value operations (Increment/Decrement/Restore/Transfer) apply only to
 *     MIFARE Classic "value blocks" (format defined below). They are library
 *     features built on top of the MFAuthent + TRANSCEIVE primitives.
 */

#ifndef MFRC522_MIFARE_H
#define MFRC522_MIFARE_H

#include <stdint.h>
#include "mfrc522_types.h"
#include "mfrc522_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MFRC522_Handle MFRC522_Handle_t;

#if MFRC522_ENABLE_MIFARE

/* ================================================================== */
/*  MIFARE command bytes                                              */
/* ================================================================== */
#define MFRC522_MF_AUTH_KEY_A       (0x60u)  /**< Authenticate with key A.      */
#define MFRC522_MF_AUTH_KEY_B       (0x61u)  /**< Authenticate with key B.      */
#define MFRC522_MF_READ             (0x30u)  /**< Read 16-byte block.           */
#define MFRC522_MF_WRITE            (0xA0u)  /**< Write 16-byte block.          */
#define MFRC522_MF_DECREMENT        (0xC0u)  /**< Decrement value block.        */
#define MFRC522_MF_INCREMENT        (0xC1u)  /**< Increment value block.        */
#define MFRC522_MF_RESTORE          (0xC2u)  /**< Restore value block.          */
#define MFRC522_MF_TRANSFER         (0xB0u)  /**< Transfer to value block.      */

/* ================================================================== */
/*  High-level constants (card geometry)                             */
/* ================================================================== */
#define MFRC522_MIFARE_1K_SECTORS   (16u)
#define MFRC522_MIFARE_4K_SECTORS   (40u)
#define MFRC522_MIFARE_BLOCKS_PER_SECTOR_SMALL (4u)
#define MFRC522_MIFARE_BLOCKS_PER_SECTOR_BIG   (16u)

/* ================================================================== */
/*  Authentication                                                    */
/* ================================================================== */

/**
 * @brief Authenticate against a MIFARE Classic sector.
 *
 * Runs the hardware MFAuthent command (Crypto1). The key must be known by
 * the application; this library stores no keys and performs no key-diversity.
 *
 * @param handle    Reader handle.
 * @param key_type  Key A or Key B.
 * @param key       The 6-byte key.
 * @param block     Block address within the sector (any block of the sector).
 * @param uid       Card UID (needed by the Crypto1 state).
 * @param uid_len   UID length.
 * @return          MFRC522_OK, MFRC522_ERR_AUTH, or transport error.
 */
MFRC522_Status_t MFRC522_Authenticate(MFRC522_Handle_t *handle,
                                      MFRC522_KeyType_t key_type,
                                      const MFRC522_Key_t *key,
                                      uint8_t block,
                                      const uint8_t *uid, uint32_t uid_len);

/**
 * @brief Convenience: authenticate with Key A.
 */
MFRC522_Status_t MFRC522_AuthKeyA(MFRC522_Handle_t *handle,
                                  const MFRC522_Key_t *key,
                                  uint8_t block,
                                  const uint8_t *uid, uint32_t uid_len);

/**
 * @brief Convenience: authenticate with Key B.
 */
MFRC522_Status_t MFRC522_AuthKeyB(MFRC522_Handle_t *handle,
                                  const MFRC522_Key_t *key,
                                  uint8_t block,
                                  const uint8_t *uid, uint32_t uid_len);

/**
 * @brief Drop the Crypto1 authentication state (Status2Reg.Crypto1On -> 0).
 */
MFRC522_Status_t MFRC522_StopCrypto1(MFRC522_Handle_t *handle);

/* ================================================================== */
/*  Block read / write                                               */
/* ================================================================== */

/**
 * @brief Read one 16-byte block (MIFARE READ, 0x30 + block + CRC).
 *
 * The block must already be authenticated. The reply is checked against the
 * CRC_A supplied by the card.
 *
 * @param data  Receives 16 bytes. Must be at least 16 bytes.
 */
MFRC522_Status_t MFRC522_ReadBlock(MFRC522_Handle_t *handle,
                                   uint8_t block, uint8_t *data);

/**
 * @brief Write one 16-byte block (MIFARE WRITE, 0xA0 + block + data + CRC).
 *
 * The block must already be authenticated. The card's ACK/NAK (0x0A / 0x00)
 * is checked.
 *
 * @param data  16 bytes to write.
 */
MFRC522_Status_t MFRC522_WriteBlock(MFRC522_Handle_t *handle,
                                    uint8_t block, const uint8_t *data);

/* ================================================================== */
/*  Sector operations                                                */
/* ================================================================== */

/**
 * @brief Authenticate every block of a sector (handles 4-block and 16-block
 *        sectors transparently).
 */
MFRC522_Status_t MFRC522_AuthenticateSector(MFRC522_Handle_t *handle,
                                            uint8_t sector,
                                            MFRC522_KeyType_t key_type,
                                            const MFRC522_Key_t *key,
                                            const uint8_t *uid, uint32_t uid_len);

/**
 * @brief Read a whole sector into a buffer (auth + read all data blocks).
 * @param data  Receives 64 (small sector) or 256 (big sector) bytes.
 * @param data_len  In: capacity; out: bytes read (excluding trailer).
 */
MFRC522_Status_t MFRC522_ReadSector(MFRC522_Handle_t *handle,
                                    uint8_t sector,
                                    MFRC522_KeyType_t key_type,
                                    const MFRC522_Key_t *key,
                                    const uint8_t *uid, uint32_t uid_len,
                                    uint8_t *data, uint32_t *data_len);

/**
 * @brief Write a whole sector (auth + write all data blocks).
 *
 * @warning Never writes the sector trailer; the key/access-bits block is
 *          left untouched to avoid bricking the sector.
 */
MFRC522_Status_t MFRC522_WriteSector(MFRC522_Handle_t *handle,
                                     uint8_t sector,
                                     MFRC522_KeyType_t key_type,
                                     const MFRC522_Key_t *key,
                                     const uint8_t *uid, uint32_t uid_len,
                                     const uint8_t *data, uint32_t data_len);

/* ================================================================== */
/*  Value block operations (MIFARE Classic)                          */
/* ================================================================== */

/**
 * @brief MIFARE Classic value-block layout (16 bytes):
 *          bytes 0..3  : value, signed 32-bit, little-endian
 *          bytes 4..7  : ~value (inverted)
 *          bytes 8..11 : value (redundant copy)
 *          bytes 12    : address byte (may hold a one-byte address)
 *          bytes 13..15: address inverted, repeated
 */

/**
 * @brief Build a value-block from a 32-bit value and 1-byte address.
 * @param block  16-byte output buffer.
 */
void MFRC522_FormatValueBlock(uint8_t block[MFRC522_BLOCK_SIZE],
                              int32_t value, uint8_t address);

/**
 * @brief Increment a value block (0xC1).
 */
MFRC522_Status_t MFRC522_Increment(MFRC522_Handle_t *handle, uint8_t block,
                                   int32_t value);

/**
 * @brief Decrement a value block (0xC0).
 */
MFRC522_Status_t MFRC522_Decrement(MFRC522_Handle_t *handle, uint8_t block,
                                   int32_t value);

/**
 * @brief Restore a value block into the internal buffer (0xC2).
 */
MFRC522_Status_t MFRC522_Restore(MFRC522_Handle_t *handle, uint8_t block);

/**
 * @brief Transfer the internal buffer to a value block (0xB0).
 */
MFRC522_Status_t MFRC522_Transfer(MFRC522_Handle_t *handle, uint8_t block);

#endif /* MFRC522_ENABLE_MIFARE */

#ifdef __cplusplus
}
#endif

#endif /* MFRC522_MIFARE_H */
