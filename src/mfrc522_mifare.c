/**
 * @file    mfrc522_mifare.c
 * @brief   MIFARE Classic / Ultralight high-level API.
 *
 * Built on top of the protocol layer. The MFRC522 contributes the Crypto1
 * authentication command (MFAuthent) in hardware; everything else here is
 * host-driven command sequencing:
 *   - Authentication (KeyA/KeyB) via the MFAuthent command.
 *   - 16-byte block read/write with ACK/NAK and CRC_A validation.
 *   - Sector-level helpers (never touch the trailer block).
 *   - Value-block protocol (Increment/Decrement/Restore/Transfer) and the
 *     value-block formatting helper.
 *
 * References:
 *   - NXP MFRC522 data sheet §10.3.1.9 (MFAuthent)
 *   - MF1S50yyX data sheet §10.1 (MIFARE Classic auth / commands)
 */

#include "mfrc522_internal.h"

#if MFRC522_ENABLE_MIFARE

/* ================================================================== */
/*  Low-level MIFARE command transceive                               */
/* ================================================================== */

/**
 * @brief Send a MIFARE command (with appended CRC_A) and check for the
 *        4-bit ACK/NAK reply.
 *
 * @param accept_timeout  When true, a timeout is treated as success (used by
 *                        the value operations whose final reply is optional).
 */
static MFRC522_Status_t mfrc522_mifare_transceive(MFRC522_Handle_t *h,
                                                  const uint8_t *data, uint32_t len,
                                                  uint8_t accept_timeout,
                                                  uint32_t timeout_ms)
{
    uint8_t frame[MFRC522_BLOCK_SIZE + 2u];
    uint8_t rx[2];
    uint32_t rx_len = sizeof(rx);
    uint8_t valid_bits = 0u;
    uint16_t crc;
    uint32_t i;
    MFRC522_Status_t status;

    if ((data == NULL) || (len == 0u) || (len > MFRC522_BLOCK_SIZE)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    /* Append CRC_A (LSB first on the air interface). */
    status = mfrc522_calc_crc(h, data, len, &crc);
    if (status != MFRC522_OK) {
        return status;
    }
    for (i = 0u; i < len; i++) {
        frame[i] = data[i];
    }
    frame[len]     = (uint8_t)(crc & 0xFFu);
    frame[len + 1u] = (uint8_t)(crc >> 8);

    status = mfrc522_transceive(h, frame, len + 2u, rx, &rx_len, &valid_bits,
                                0u, 0u, 0u /* check_crc */, timeout_ms);
    if ((accept_timeout != 0u) && (status == MFRC522_ERR_TIMEOUT)) {
        return MFRC522_OK;
    }
    if (status != MFRC522_OK) {
        return status;
    }

    /* The PICC must answer with a 4-bit ACK (0x0A) or NAK (0x00). */
    if ((rx_len != 1u) || (valid_bits != 4u)) {
        return MFRC522_ERR_PROTOCOL;
    }
    if (rx[0] == MFRC522_MF_ACK) {
        return MFRC522_OK;
    }
    return MFRC522_ERR_AUTH;   /* NAK: card rejected the command */
}

/* ================================================================== */
/*  Authentication                                                    */
/* ================================================================== */

/**
 * @brief Run the hardware MFAuthent command (Crypto1) and wait for completion.
 */
static MFRC522_Status_t mfrc522_authent_command(MFRC522_Handle_t *h,
                                                const uint8_t *tx, uint32_t tx_len,
                                                uint32_t timeout_ms)
{
    MFRC522_Status_t status;
    uint8_t irq;

    status = mfrc522_write_command(h, MFRC522_CMD_IDLE);
    if (status != MFRC522_OK) return status;
    status = MFRC522_WriteRegister(h, MFRC522_REG_COM_IRQ, MFRC522_IRQ_ALL);
    if (status != MFRC522_OK) return status;
    status = mfrc522_flush_fifo(h);
    if (status != MFRC522_OK) return status;
    status = MFRC522_WriteFIFO(h, tx, tx_len);
    if (status != MFRC522_OK) return status;
    status = MFRC522_WriteRegister(h, MFRC522_REG_BIT_FRAMING, 0x00u);
    if (status != MFRC522_OK) return status;
    status = mfrc522_write_command(h, MFRC522_CMD_MF_AUTHENT);
    if (status != MFRC522_OK) return status;

    /* MFAuthent finishes with IdleIRq; a wrong key yields a timeout. */
    status = mfrc522_wait_irq(h, MFRC522_IRQ_IDLE, &irq, timeout_ms);
    if (status == MFRC522_ERR_TIMEOUT) {
        return MFRC522_ERR_AUTH;
    }
    return status;
}

MFRC522_Status_t MFRC522_Authenticate(MFRC522_Handle_t *handle,
                                      MFRC522_KeyType_t key_type,
                                      const MFRC522_Key_t *key,
                                      uint8_t block,
                                      const uint8_t *uid, uint32_t uid_len)
{
    uint8_t send[12];
    uint8_t i;
    MFRC522_Status_t status;

    if ((handle == NULL) || (key == NULL) || (uid == NULL) || (uid_len < 4u)) {
        return MFRC522_ERR_INVALID_PARAM;
    }
    if ((key_type != MFRC522_KEY_A) && (key_type != MFRC522_KEY_B)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    send[0] = (key_type == MFRC522_KEY_A) ? MFRC522_MF_AUTH_KEY_A
                                          : MFRC522_MF_AUTH_KEY_B;
    send[1] = block;
    for (i = 0u; i < 6u; i++) {
        send[2u + i] = key->key[i];
    }
    /* The last 4 bytes of the UID form the Crypto1 challenge. */
    for (i = 0u; i < 4u; i++) {
        send[8u + i] = uid[uid_len - 4u + i];
    }

    mfrc522_lock(handle);
    status = mfrc522_authent_command(handle, send, 12u, handle->config.timeout_ms);
    mfrc522_unlock(handle);
    return status;
}

MFRC522_Status_t MFRC522_AuthKeyA(MFRC522_Handle_t *handle,
                                  const MFRC522_Key_t *key,
                                  uint8_t block,
                                  const uint8_t *uid, uint32_t uid_len)
{
    return MFRC522_Authenticate(handle, MFRC522_KEY_A, key, block, uid, uid_len);
}

MFRC522_Status_t MFRC522_AuthKeyB(MFRC522_Handle_t *handle,
                                  const MFRC522_Key_t *key,
                                  uint8_t block,
                                  const uint8_t *uid, uint32_t uid_len)
{
    return MFRC522_Authenticate(handle, MFRC522_KEY_B, key, block, uid, uid_len);
}

MFRC522_Status_t MFRC522_StopCrypto1(MFRC522_Handle_t *handle)
{
    if (handle == NULL) {
        return MFRC522_ERR_INVALID_PARAM;
    }
    return MFRC522_ClearBits(handle, MFRC522_REG_STATUS2,
                             MFRC522_STATUS2_CRYPTO1_ON);
}

/* ================================================================== */
/*  Block read / write                                                */
/* ================================================================== */

MFRC522_Status_t MFRC522_ReadBlock(MFRC522_Handle_t *handle,
                                   uint8_t block, uint8_t *data)
{
    uint8_t frame[4];
    uint8_t rx[MFRC522_BLOCK_SIZE + 2u];
    uint32_t rx_len = sizeof(rx);
    uint16_t crc;
    uint8_t i;
    MFRC522_Status_t status;

    if ((handle == NULL) || (data == NULL)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    frame[0] = MFRC522_MF_READ;
    frame[1] = block;
    status = mfrc522_calc_crc(handle, frame, 2u, &crc);
    if (status != MFRC522_OK) return status;
    frame[2] = (uint8_t)(crc & 0xFFu);
    frame[3] = (uint8_t)(crc >> 8);

    mfrc522_lock(handle);
    status = mfrc522_transceive(handle, frame, 4u, rx, &rx_len, NULL,
                                0u, 0u, 1u /* check_crc */, handle->config.timeout_ms);
    mfrc522_unlock(handle);
    if (status != MFRC522_OK) {
        return status;
    }

    /* 16 data bytes + 2 CRC bytes expected. */
    if (rx_len != (MFRC522_BLOCK_SIZE + 2u)) {
        return MFRC522_ERR_PROTOCOL;
    }
    for (i = 0u; i < MFRC522_BLOCK_SIZE; i++) {
        data[i] = rx[i];
    }
    return MFRC522_OK;
}

MFRC522_Status_t MFRC522_WriteBlock(MFRC522_Handle_t *handle,
                                    uint8_t block, const uint8_t *data)
{
    uint8_t cmd[2];
    MFRC522_Status_t status;

    if ((handle == NULL) || (data == NULL)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    mfrc522_lock(handle);

    /* Step 1: announce the write target. */
    cmd[0] = MFRC522_MF_WRITE;
    cmd[1] = block;
    status = mfrc522_mifare_transceive(handle, cmd, 2u, 0u, handle->config.timeout_ms);
    if (status != MFRC522_OK) {
        mfrc522_unlock(handle);
        return status;
    }

    /* Step 2: transfer the 16 data bytes. */
    status = mfrc522_mifare_transceive(handle, data, MFRC522_BLOCK_SIZE, 0u,
                                       handle->config.timeout_ms);
    mfrc522_unlock(handle);
    return status;
}

/* ================================================================== */
/*  Sector operations                                                 */
/* ================================================================== */

/**
 * @brief Resolve a sector number into its first block and block count.
 * @return MFRC522_OK or MFRC522_ERR_INVALID_PARAM for out-of-range sectors.
 */
static MFRC522_Status_t mfrc522_sector_geometry(uint8_t sector,
                                                uint8_t *first_block,
                                                uint8_t *block_count)
{
    if (sector < 32u) {
        *first_block = (uint8_t)(sector * 4u);
        *block_count = 4u;
    } else if (sector < 40u) {
        *first_block = (uint8_t)(128u + (sector - 32u) * 16u);
        *block_count = 16u;
    } else {
        return MFRC522_ERR_INVALID_PARAM;   /* no MIFARE Classic card has >40 sectors */
    }
    return MFRC522_OK;
}

MFRC522_Status_t MFRC522_AuthenticateSector(MFRC522_Handle_t *handle,
                                            uint8_t sector,
                                            MFRC522_KeyType_t key_type,
                                            const MFRC522_Key_t *key,
                                            const uint8_t *uid, uint32_t uid_len)
{
    uint8_t first_block;
    uint8_t block_count;
    MFRC522_Status_t status;

    if ((handle == NULL) || (key == NULL) || (uid == NULL)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    status = mfrc522_sector_geometry(sector, &first_block, &block_count);
    if (status != MFRC522_OK) {
        return status;
    }
    (void)block_count;

    /* Authenticating any block of a sector authenticates the whole sector. */
    return MFRC522_Authenticate(handle, key_type, key, first_block, uid, uid_len);
}

MFRC522_Status_t MFRC522_ReadSector(MFRC522_Handle_t *handle,
                                    uint8_t sector,
                                    MFRC522_KeyType_t key_type,
                                    const MFRC522_Key_t *key,
                                    const uint8_t *uid, uint32_t uid_len,
                                    uint8_t *data, uint32_t *data_len)
{
    uint8_t first_block;
    uint8_t block_count;
    uint8_t data_blocks;
    uint8_t b;
    uint32_t used = 0u;
    MFRC522_Status_t status;

    if ((handle == NULL) || (key == NULL) || (uid == NULL) ||
        (data == NULL) || (data_len == NULL)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    status = mfrc522_sector_geometry(sector, &first_block, &block_count);
    if (status != MFRC522_OK) return status;
    data_blocks = (uint8_t)(block_count - 1u);   /* exclude the trailer */

    if (*data_len < ((uint32_t)data_blocks * MFRC522_BLOCK_SIZE)) {
        return MFRC522_ERR_OVERFLOW;
    }

    status = MFRC522_AuthenticateSector(handle, sector, key_type, key, uid, uid_len);
    if (status != MFRC522_OK) return status;

    for (b = 0u; b < data_blocks; b++) {
        status = MFRC522_ReadBlock(handle, (uint8_t)(first_block + b),
                                   &data[used]);
        if (status != MFRC522_OK) {
            MFRC522_StopCrypto1(handle);
            return status;
        }
        used += MFRC522_BLOCK_SIZE;
    }

    MFRC522_StopCrypto1(handle);
    *data_len = used;
    return MFRC522_OK;
}

MFRC522_Status_t MFRC522_WriteSector(MFRC522_Handle_t *handle,
                                     uint8_t sector,
                                     MFRC522_KeyType_t key_type,
                                     const MFRC522_Key_t *key,
                                     const uint8_t *uid, uint32_t uid_len,
                                     const uint8_t *data, uint32_t data_len)
{
    uint8_t first_block;
    uint8_t block_count;
    uint8_t data_blocks;
    uint8_t b;
    uint32_t used = 0u;
    MFRC522_Status_t status;

    if ((handle == NULL) || (key == NULL) || (uid == NULL) || (data == NULL)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    status = mfrc522_sector_geometry(sector, &first_block, &block_count);
    if (status != MFRC522_OK) return status;
    data_blocks = (uint8_t)(block_count - 1u);   /* exclude the trailer */

    if (data_len < ((uint32_t)data_blocks * MFRC522_BLOCK_SIZE)) {
        return MFRC522_ERR_OVERFLOW;
    }

    status = MFRC522_AuthenticateSector(handle, sector, key_type, key, uid, uid_len);
    if (status != MFRC522_OK) return status;

    /* Never write the trailer block: it holds the keys / access bits. */
    for (b = 0u; b < data_blocks; b++) {
        status = MFRC522_WriteBlock(handle, (uint8_t)(first_block + b),
                                    &data[used]);
        if (status != MFRC522_OK) {
            MFRC522_StopCrypto1(handle);
            return status;
        }
        used += MFRC522_BLOCK_SIZE;
    }

    MFRC522_StopCrypto1(handle);
    return MFRC522_OK;
}

/* ================================================================== */
/*  Value blocks                                                      */
/* ================================================================== */

void MFRC522_FormatValueBlock(uint8_t block[MFRC522_BLOCK_SIZE],
                              int32_t value, uint8_t address)
{
    uint32_t v = (uint32_t)value;

    block[0] = (uint8_t)(v & 0xFFu);
    block[1] = (uint8_t)((v >> 8) & 0xFFu);
    block[2] = (uint8_t)((v >> 16) & 0xFFu);
    block[3] = (uint8_t)((v >> 24) & 0xFFu);

    block[4] = (uint8_t)~block[0];
    block[5] = (uint8_t)~block[1];
    block[6] = (uint8_t)~block[2];
    block[7] = (uint8_t)~block[3];

    block[8]  = block[0];
    block[9]  = block[1];
    block[10] = block[2];
    block[11] = block[3];

    block[12] = address;
    block[13] = (uint8_t)~address;
    block[14] = address;
    block[15] = (uint8_t)~address;
}

/**
 * @brief Two-step value command (Increment/Decrement/Restore).
 */
static MFRC522_Status_t mfrc522_value_two_step(MFRC522_Handle_t *handle,
                                               uint8_t command, uint8_t block,
                                               int32_t value)
{
    uint8_t cmd[2];
    uint8_t data[4];
    MFRC522_Status_t status;

    cmd[0] = command;
    cmd[1] = block;

    mfrc522_lock(handle);

    /* Step 1: announce command + block, expect ACK. */
    status = mfrc522_mifare_transceive(handle, cmd, 2u, 0u, handle->config.timeout_ms);
    if (status != MFRC522_OK) {
        mfrc522_unlock(handle);
        return status;
    }

    /* Step 2: transfer the operand (timeout is acceptable here). */
    data[0] = (uint8_t)((uint32_t)value & 0xFFu);
    data[1] = (uint8_t)(((uint32_t)value >> 8) & 0xFFu);
    data[2] = (uint8_t)(((uint32_t)value >> 16) & 0xFFu);
    data[3] = (uint8_t)(((uint32_t)value >> 24) & 0xFFu);
    status = mfrc522_mifare_transceive(handle, data, 4u, 1u /* accept timeout */,
                                       handle->config.timeout_ms);
    mfrc522_unlock(handle);
    return status;
}

MFRC522_Status_t MFRC522_Increment(MFRC522_Handle_t *handle, uint8_t block,
                                   int32_t value)
{
    if (handle == NULL) return MFRC522_ERR_INVALID_PARAM;
    return mfrc522_value_two_step(handle, MFRC522_MF_INCREMENT, block, value);
}

MFRC522_Status_t MFRC522_Decrement(MFRC522_Handle_t *handle, uint8_t block,
                                   int32_t value)
{
    if (handle == NULL) return MFRC522_ERR_INVALID_PARAM;
    return mfrc522_value_two_step(handle, MFRC522_MF_DECREMENT, block, value);
}

MFRC522_Status_t MFRC522_Restore(MFRC522_Handle_t *handle, uint8_t block)
{
    if (handle == NULL) return MFRC522_ERR_INVALID_PARAM;
    /* Restore transfers 0 in the second step (per the MIFARE data sheet). */
    return mfrc522_value_two_step(handle, MFRC522_MF_RESTORE, block, 0);
}

MFRC522_Status_t MFRC522_Transfer(MFRC522_Handle_t *handle, uint8_t block)
{
    uint8_t cmd[2];
    MFRC522_Status_t status;

    if (handle == NULL) return MFRC522_ERR_INVALID_PARAM;

    cmd[0] = MFRC522_MF_TRANSFER;
    cmd[1] = block;

    mfrc522_lock(handle);
    status = mfrc522_mifare_transceive(handle, cmd, 2u, 0u, handle->config.timeout_ms);
    mfrc522_unlock(handle);
    return status;
}

#endif /* MFRC522_ENABLE_MIFARE */
