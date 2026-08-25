/**
 * @file    mfrc522.c
 * @brief   Core lifecycle, configuration and utility API of the MFRC522
 *          driver (initialization, reset, version, self-test, antenna,
 *          low-power, debug and status helpers).
 */

#include "mfrc522_internal.h"

/* ------------------------------------------------------------------ */
/* Self-test reference vectors (NXP datasheet Rev 3.8, 16.1.1).       */
/* ------------------------------------------------------------------ */
static const uint8_t MFRC522_SELFTEST_REF_V0_0[64] = {
    0x00, 0x87, 0x98, 0x0F, 0x49, 0xFF, 0x07, 0x19,
    0xBF, 0x22, 0x30, 0x49, 0x59, 0x63, 0xAD, 0xCA,
    0x7F, 0xE3, 0x4E, 0x03, 0x5C, 0x4E, 0x49, 0x50,
    0x47, 0x9A, 0x37, 0x61, 0xE7, 0xE2, 0xC6, 0x2E,
    0x75, 0x5A, 0xED, 0x04, 0x3D, 0x02, 0x4B, 0x78,
    0x32, 0xFF, 0x58, 0x3B, 0x7C, 0xE9, 0x00, 0x94,
    0xB4, 0x4A, 0x59, 0x5B, 0xFD, 0xC9, 0x29, 0xDF,
    0x35, 0x96, 0x98, 0x9E, 0x4F, 0x30, 0x32, 0x8D
};

static const uint8_t MFRC522_SELFTEST_REF_V1_0[64] = {
    0x00, 0xC6, 0x37, 0xD5, 0x32, 0xB7, 0x57, 0x5C,
    0xC2, 0xD8, 0x7C, 0x4D, 0xD9, 0x70, 0xC7, 0x73,
    0x10, 0xE6, 0xD2, 0xAA, 0x5E, 0xA1, 0x3E, 0x5A,
    0x14, 0xAF, 0x30, 0x61, 0xC9, 0x70, 0xDB, 0x2E,
    0x64, 0x22, 0x72, 0xB5, 0xBD, 0x65, 0xF4, 0xEC,
    0x22, 0xBC, 0xD3, 0x72, 0x35, 0xCD, 0xAA, 0x41,
    0x1F, 0xA7, 0xF3, 0x53, 0x14, 0xDE, 0x7E, 0x02,
    0xD9, 0x0F, 0xB5, 0x5E, 0x25, 0x1D, 0x29, 0x79
};

static const uint8_t MFRC522_SELFTEST_REF_V2_0[64] = {
    0x00, 0xEB, 0x66, 0xBA, 0x57, 0xBF, 0x23, 0x95,
    0xD0, 0xE3, 0x0D, 0x3D, 0x27, 0x89, 0x5C, 0xDE,
    0x9D, 0x3B, 0xA7, 0x00, 0x21, 0x5B, 0x89, 0x82,
    0x51, 0x3A, 0xEB, 0x02, 0x0C, 0xA5, 0x00, 0x49,
    0x7C, 0x84, 0x4D, 0xB3, 0xCC, 0xD2, 0x1B, 0x81,
    0x5D, 0x48, 0x76, 0xD5, 0x71, 0x61, 0x21, 0xA9,
    0x86, 0x96, 0x83, 0x38, 0xCF, 0x9D, 0x5B, 0x6D,
    0xDC, 0x15, 0xBA, 0x3E, 0x7D, 0x95, 0x3B, 0x2F
};

/* ------------------------------------------------------------------ */
/* Status strings                                                     */
/* ------------------------------------------------------------------ */

static const char *const MFRC522_STATUS_STRINGS[MFRC522_STATUS_COUNT] = {
    "OK", "INVALID_PARAM", "BUSY", "TIMEOUT", "COMM", "CRC",
    "COLLISION", "AUTH", "NO_CARD", "PROTOCOL", "DEVICE", "FIFO",
    "OVERFLOW", "NOT_SUPPORTED", "INTERNAL"
};

const char *MFRC522_StatusToString(MFRC522_Status_t status)
{
    if (((int)status < 0) || (status >= MFRC522_STATUS_COUNT)) {
        return "UNKNOWN";
    }
    return MFRC522_STATUS_STRINGS[status];
}

static const char *const MFRC522_CARD_TYPE_STRINGS[MFRC522_CARD_COUNT] = {
    "UNKNOWN",
    "MIFARE Mini",
    "MIFARE 1K",
    "MIFARE 4K",
    "MIFARE Ultralight",
    "MIFARE Ultralight C",
    "ISO/IEC 14443-4"
};

const char *MFRC522_CardTypeToString(MFRC522_CardType_t type)
{
    if (((int)type < 0) || (type >= MFRC522_CARD_COUNT)) {
        return "UNKNOWN";
    }
    return MFRC522_CARD_TYPE_STRINGS[type];
}

MFRC522_Status_t MFRC522_GetLastError(const MFRC522_Handle_t *handle)
{
    if (handle == NULL) {
        return MFRC522_ERR_INVALID_PARAM;
    }
    return handle->state.last_error;
}

void MFRC522_AttachDebug(MFRC522_Handle_t *handle, const MFRC522_Debug_t *debug)
{
    if (handle != NULL) {
        handle->debug = debug;
    }
}

#if MFRC522_ENABLE_DEBUG
void mfrc522_log(MFRC522_Handle_t *handle, MFRC522_LogLevel_t level,
                 const char *message)
{
    if ((handle != NULL) && (handle->debug != NULL) &&
        (handle->debug->log != NULL)) {
        handle->debug->log(handle->debug->context, level, message);
    }
}
#endif /* MFRC522_ENABLE_DEBUG */

/* ------------------------------------------------------------------ */
/* Default configuration                                              */
/* ------------------------------------------------------------------ */

static void mfrc522_set_default_config(MFRC522_Config_t *cfg)
{
    cfg->timeout_ms       = MFRC522_DEFAULT_TIMEOUT_MS;
    cfg->reset_wait_ms    = MFRC522_RESET_WAIT_MS;
    cfg->antenna_settle_us = MFRC522_ANTENNA_SETTLE_US;
    cfg->irq_enabled      = 0u;
    cfg->reserved[0]      = 0u;
    cfg->reserved[1]      = 0u;
    cfg->reserved[2]      = 0u;
}

/* ------------------------------------------------------------------ */
/* Reset                                                              */
/* ------------------------------------------------------------------ */

MFRC522_Status_t MFRC522_SoftReset(MFRC522_Handle_t *handle)
{
    MFRC522_Status_t status;
    uint8_t cmd;
    uint32_t deadline;

    if (handle == NULL) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    status = mfrc522_write_command(handle, MFRC522_CMD_SOFT_RESET);
    if (status != MFRC522_OK) {
        return status;
    }

    /* Wait for the PowerDown bit in CommandReg to clear. */
    deadline = mfrc522_tick_ms(handle) + handle->config.reset_wait_ms * 3u;
    for (;;) {
        status = MFRC522_ReadRegister(handle, MFRC522_REG_COMMAND, &cmd);
        if (status != MFRC522_OK) {
            return status;
        }
        if ((cmd & MFRC522_COMMAND_POWER_DOWN) == 0u) {
            break;
        }
        if ((int32_t)(deadline - mfrc522_tick_ms(handle)) <= 0) {
            return MFRC522_ERR_TIMEOUT;
        }
    }

    return MFRC522_OK;
}

MFRC522_Status_t MFRC522_HardReset(MFRC522_Handle_t *handle)
{
    if ((handle == NULL) || (mfrc522_platform_valid(handle) != MFRC522_OK) ||
        (handle->platform.ops->reset_assert == NULL) ||
        (handle->platform.ops->reset_deassert == NULL)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    handle->platform.ops->reset_assert(handle->platform.ctx);
    mfrc522_delay_us(handle, 10u);      /* >= 100 ns per datasheet 8.8.1 */
    handle->platform.ops->reset_deassert(handle->platform.ctx);
    mfrc522_delay_ms(handle, handle->config.reset_wait_ms);

    return MFRC522_OK;
}

/* ------------------------------------------------------------------ */
/* Antenna                                                            */
/* ------------------------------------------------------------------ */

MFRC522_Status_t MFRC522_AntennaOn(MFRC522_Handle_t *handle)
{
    MFRC522_Status_t status;
    uint8_t value;

    if (handle == NULL) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    status = MFRC522_ReadRegister(handle, MFRC522_REG_TX_CONTROL, &value);
    if (status != MFRC522_OK) {
        return status;
    }

    if ((value & MFRC522_TX_CONTROL_RF_EN_MASK) != MFRC522_TX_CONTROL_RF_EN_MASK) {
        status = MFRC522_WriteRegister(handle, MFRC522_REG_TX_CONTROL,
                                       (uint8_t)(value | MFRC522_TX_CONTROL_RF_EN_MASK));
        if (status != MFRC522_OK) {
            return status;
        }
    }

    mfrc522_delay_us(handle, handle->config.antenna_settle_us);
    handle->state.flags |= MFRC522_FLAG_ANTENNA_ON;
    return MFRC522_OK;
}

MFRC522_Status_t MFRC522_AntennaOff(MFRC522_Handle_t *handle)
{
    MFRC522_Status_t status;

    if (handle == NULL) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    status = MFRC522_ClearBits(handle, MFRC522_REG_TX_CONTROL,
                               MFRC522_TX_CONTROL_RF_EN_MASK);
    if (status != MFRC522_OK) {
        return status;
    }

    handle->state.flags &= (uint8_t)~MFRC522_FLAG_ANTENNA_ON;
    return MFRC522_OK;
}

uint8_t MFRC522_IsAntennaOn(const MFRC522_Handle_t *handle)
{
    if (handle == NULL) {
        return 0u;
    }
    return (uint8_t)((handle->state.flags & MFRC522_FLAG_ANTENNA_ON) != 0u);
}

/* ------------------------------------------------------------------ */
/* Version                                                            */
/* ------------------------------------------------------------------ */

MFRC522_Status_t MFRC522_GetVersion(MFRC522_Handle_t *handle,
                                    MFRC522_Version_t *version)
{
    MFRC522_Status_t status;
    uint8_t raw;

    if ((handle == NULL) || (version == NULL)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    status = MFRC522_ReadRegister(handle, MFRC522_REG_VERSION, &raw);
    if (status != MFRC522_OK) {
        return status;
    }

    version->raw   = raw;
    /* VersionReg layout: the high nibble marks the device family
     * (0x9 = MFRC522, 0x8 = FM17522 clone); the low nibble is the silicon
     * version (0x91 -> 1.0, 0x92 -> 2.0). */
    version->major = (uint8_t)(raw & 0x0Fu);
    version->minor = 0u;
    return MFRC522_OK;
}

/* ------------------------------------------------------------------ */
/* Initialization                                                     */
/* ------------------------------------------------------------------ */

static MFRC522_Status_t mfrc522_init_registers(MFRC522_Handle_t *handle)
{
    MFRC522_Status_t status;

    /* Reset data rates and modulation width. */
    status = MFRC522_WriteRegister(handle, MFRC522_REG_TX_MODE, 0x00u);
    if (status != MFRC522_OK) return status;
    status = MFRC522_WriteRegister(handle, MFRC522_REG_RX_MODE, 0x00u);
    if (status != MFRC522_OK) return status;
    status = MFRC522_WriteRegister(handle, MFRC522_REG_MOD_WIDTH, 0x26u);
    if (status != MFRC522_OK) return status;

    /* Timer: f_timer = 13.56 MHz / (2*169+1) ~ 40 kHz (25 us period),
     * reload 0x03E8 = 1000 => ~25 ms timeout. TAuto: start after TX. */
    status = MFRC522_WriteRegister(handle, MFRC522_REG_T_MODE,
                                   MFRC522_T_MODE_TAUTO);
    if (status != MFRC522_OK) return status;
    status = MFRC522_WriteRegister(handle, MFRC522_REG_T_PRESCALER, 0xA9u);
    if (status != MFRC522_OK) return status;
    status = MFRC522_WriteRegister(handle, MFRC522_REG_T_RELOAD_MSB, 0x03u);
    if (status != MFRC522_OK) return status;
    status = MFRC522_WriteRegister(handle, MFRC522_REG_T_RELOAD_LSB, 0xE8u);
    if (status != MFRC522_OK) return status;

    /* Force 100% ASK and select CRC preset 0x6363 for CalcCRC. */
    status = MFRC522_WriteRegister(handle, MFRC522_REG_TX_ASK,
                                   MFRC522_TX_ASK_FORCE_100_ASK);
    if (status != MFRC522_OK) return status;
    status = MFRC522_WriteRegister(handle, MFRC522_REG_MODE, 0x3Du);
    if (status != MFRC522_OK) return status;

    /* Configure the serial UART baud rate if the UART host is selected. */
#if MFRC522_ENABLE_UART
    if (handle->transport.type == MFRC522_TRANSPORT_UART) {
        status = MFRC522_UART_ApplyBaud(handle);
        if (status != MFRC522_OK) return status;
    }
#else
    (void)0;
#endif

    return MFRC522_AntennaOn(handle);
}

MFRC522_Status_t MFRC522_Init(MFRC522_Handle_t *handle)
{
    MFRC522_Status_t status;
    uint8_t version;

    if ((handle == NULL) || (mfrc522_platform_valid(handle) != MFRC522_OK) ||
        (handle->transport_ops == NULL)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    /* Apply the runtime defaults, then a soft reset (works even if no reset
     * GPIO is wired; a hard reset is available via MFRC522_HardReset()). */
    mfrc522_set_default_config(&handle->config);

    status = MFRC522_SoftReset(handle);
    if (status != MFRC522_OK) {
        handle->state.last_error = status;
        return status;
    }

    status = mfrc522_init_registers(handle);
    if (status != MFRC522_OK) {
        handle->state.last_error = status;
        return status;
    }

    /* Detect the device via the version register. */
    status = MFRC522_ReadRegister(handle, MFRC522_REG_VERSION, &version);
    if (status != MFRC522_OK) {
        handle->state.last_error = status;
        return status;
    }

    switch (version) {
        case MFRC522_VERSION_V0_0:
        case MFRC522_VERSION_V1_0:
        case MFRC522_VERSION_V2_0:
        case MFRC522_VERSION_FM17522:
        case MFRC522_VERSION_B2:
            break;
        default:
            handle->state.last_error = MFRC522_ERR_DEVICE;
            return MFRC522_ERR_DEVICE;
    }

    handle->state.version_raw = version;
    handle->state.flags |= MFRC522_FLAG_INITIALIZED;
    handle->state.last_error = MFRC522_OK;
    return MFRC522_OK;
}

MFRC522_Status_t MFRC522_Deinit(MFRC522_Handle_t *handle)
{
    if (handle == NULL) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    MFRC522_AntennaOff(handle);
    handle->state.flags &= (uint8_t)~MFRC522_FLAG_INITIALIZED;
    handle->state.last_error = MFRC522_OK;
    return MFRC522_OK;
}

/* ------------------------------------------------------------------ */
/* Self-test                                                          */
/* ------------------------------------------------------------------ */

MFRC522_Status_t MFRC522_SelfTest(MFRC522_Handle_t *handle)
{
    MFRC522_Status_t status;
    uint8_t zeroes[25] = { 0u };
    uint8_t result[64];
    uint8_t fifo_level;
    uint8_t version;
    const uint8_t *reference = NULL;
    uint32_t deadline;
    uint8_t i;

    if (handle == NULL) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    /* 1. Soft reset. */
    status = MFRC522_SoftReset(handle);
    if (status != MFRC522_OK) return status;

    /* 2. Clear the internal buffer: flush FIFO, write 25x 0x00, Mem. */
    status = mfrc522_flush_fifo(handle);
    if (status != MFRC522_OK) return status;
    status = MFRC522_WriteFIFO(handle, zeroes, 25u);
    if (status != MFRC522_OK) return status;
    status = mfrc522_write_command(handle, MFRC522_CMD_MEM);
    if (status != MFRC522_OK) return status;

    /* 3. Enable self-test. */
    status = MFRC522_WriteRegister(handle, MFRC522_REG_AUTO_TEST, 0x09u);
    if (status != MFRC522_OK) return status;

    /* 4. Write 0x00 to the FIFO. */
    status = MFRC522_WriteRegister(handle, MFRC522_REG_FIFO_DATA, 0x00u);
    if (status != MFRC522_OK) return status;

    /* 5. Start the self-test with CalcCRC. */
    status = mfrc522_write_command(handle, MFRC522_CMD_CALC_CRC);
    if (status != MFRC522_OK) return status;

    /* 6. Wait for 64 bytes to appear in the FIFO. */
    deadline = mfrc522_tick_ms(handle) + handle->config.timeout_ms;
    for (;;) {
        status = mfrc522_get_fifo_level(handle, &fifo_level);
        if (status != MFRC522_OK) {
            (void)mfrc522_write_command(handle, MFRC522_CMD_IDLE);
            return status;
        }
        if (fifo_level >= MFRC522_FIFO_SIZE) {
            break;
        }
        if ((int32_t)(deadline - mfrc522_tick_ms(handle)) <= 0) {
            (void)mfrc522_write_command(handle, MFRC522_CMD_IDLE);
            return MFRC522_ERR_TIMEOUT;
        }
    }

    status = mfrc522_write_command(handle, MFRC522_CMD_IDLE);
    if (status != MFRC522_OK) return status;

    /* 7. Read the 64 result bytes. */
    status = MFRC522_ReadFIFO(handle, result, 64u);
    if (status != MFRC522_OK) return status;

    /* Disable self-test again. */
    status = MFRC522_WriteRegister(handle, MFRC522_REG_AUTO_TEST, 0x00u);
    if (status != MFRC522_OK) return status;

    /* 8. Choose the reference vector for this firmware version. */
    status = MFRC522_ReadRegister(handle, MFRC522_REG_VERSION, &version);
    if (status != MFRC522_OK) return status;

    switch (version) {
        case MFRC522_VERSION_V0_0: reference = MFRC522_SELFTEST_REF_V0_0; break;
        case MFRC522_VERSION_V1_0: reference = MFRC522_SELFTEST_REF_V1_0; break;
        case MFRC522_VERSION_V2_0: reference = MFRC522_SELFTEST_REF_V2_0; break;
        default:
            return MFRC522_ERR_DEVICE;
    }

    for (i = 0u; i < 64u; i++) {
        if (result[i] != reference[i]) {
            return MFRC522_ERR_INTERNAL;   /* mismatch: self-test failed */
        }
    }

    /* Re-initialize the registers: the device is unusable until re-init. */
    return mfrc522_init_registers(handle);
}

/* ------------------------------------------------------------------ */
/* Low power                                                          */
/* ------------------------------------------------------------------ */

MFRC522_Status_t MFRC522_Sleep(MFRC522_Handle_t *handle)
{
    if (handle == NULL) {
        return MFRC522_ERR_INVALID_PARAM;
    }
    return MFRC522_SetBits(handle, MFRC522_REG_COMMAND,
                           MFRC522_COMMAND_POWER_DOWN);
}

MFRC522_Status_t MFRC522_WakeUp(MFRC522_Handle_t *handle)
{
    MFRC522_Status_t status;
    uint8_t cmd;
    uint32_t deadline;

    if (handle == NULL) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    status = MFRC522_ClearBits(handle, MFRC522_REG_COMMAND,
                               MFRC522_COMMAND_POWER_DOWN);
    if (status != MFRC522_OK) {
        return status;
    }

    /* Wait for the PowerDown bit to clear (end of the wake-up procedure). */
    deadline = mfrc522_tick_ms(handle) + 500u;
    for (;;) {
        status = MFRC522_ReadRegister(handle, MFRC522_REG_COMMAND, &cmd);
        if (status != MFRC522_OK) {
            return status;
        }
        if ((cmd & MFRC522_COMMAND_POWER_DOWN) == 0u) {
            break;
        }
        if ((int32_t)(deadline - mfrc522_tick_ms(handle)) <= 0) {
            return MFRC522_ERR_TIMEOUT;
        }
    }

    return MFRC522_OK;
}

MFRC522_Status_t MFRC522_PowerDown(MFRC522_Handle_t *handle)
{
    if ((handle == NULL) || (mfrc522_platform_valid(handle) != MFRC522_OK) ||
        (handle->platform.ops->reset_assert == NULL)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    /* Drive NRSTPD low: the device enters power-down (lowest consumption). */
    handle->platform.ops->reset_assert(handle->platform.ctx);
    handle->state.flags &= (uint8_t)~MFRC522_FLAG_INITIALIZED;
    return MFRC522_OK;
}

/* ------------------------------------------------------------------ */
/* Card detection / UID / card info                                   */
/* ------------------------------------------------------------------ */

/**
 * @brief Derive a high-level card type from the SAK byte.
 *
 * Mapping follows NXP AN10833 "Coding of Select Acknowledge (SAK)". The
 * 8th bit is ignored (ISO/IEC 14443 counts from LSB).
 */
static MFRC522_CardType_t mfrc522_card_type_from_sak(uint8_t sak)
{
    sak &= 0x7Fu;
    switch (sak) {
        case 0x09u: return MFRC522_CARD_MIFARE_MINI;
        case 0x08u: return MFRC522_CARD_MIFARE_1K;
        case 0x18u: return MFRC522_CARD_MIFARE_4K;
        case 0x00u: return MFRC522_CARD_MIFARE_ULTRALIGHT;
        case 0x20u:
        case 0x24u: return MFRC522_CARD_ISO14443_4;
        default:    return MFRC522_CARD_UNKNOWN;
    }
}

MFRC522_Status_t MFRC522_IsCardPresent(MFRC522_Handle_t *handle)
{
    MFRC522_Status_t status;
    uint8_t atqa[2];
    uint32_t atqa_len = 2u;

    if (handle == NULL) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    /* REQA detects IDLE cards and already-selected (READY) cards; the WUPA
     * fallback additionally wakes HALT'ed cards. A short timeout keeps this
     * check snappy. */
    status = mfrc522_request_card(handle, atqa, &atqa_len,
                                  MFRC522_CARD_POLL_TIMEOUT_MS);
    if (status == MFRC522_OK) {
        return MFRC522_OK;
    }
    if (status == MFRC522_ERR_COLLISION) {
        return MFRC522_OK;   /* more than one card: definitely present */
    }
    if (status == MFRC522_ERR_TIMEOUT) {
        return MFRC522_ERR_NO_CARD;
    }
    return status;
}

MFRC522_Status_t MFRC522_WaitForCard(MFRC522_Handle_t *handle,
                                     uint32_t timeout_ms)
{
    MFRC522_Status_t status;
    uint32_t deadline;
    uint32_t now;

    if (handle == NULL) {
        return MFRC522_ERR_INVALID_PARAM;
    }
    if (timeout_ms == 0u) {
        timeout_ms = handle->config.timeout_ms;
    }

    deadline = mfrc522_tick_ms(handle) + timeout_ms;
    for (;;) {
        status = MFRC522_IsCardPresent(handle);
        if (status == MFRC522_OK) {
            return MFRC522_OK;
        }
        if (status != MFRC522_ERR_NO_CARD) {
            return status;
        }

        now = mfrc522_tick_ms(handle);
        if ((int32_t)(deadline - now) <= 0) {
            return MFRC522_ERR_TIMEOUT;
        }
        mfrc522_delay_ms(handle, 10u);
    }
}

MFRC522_Status_t MFRC522_ReadUID(MFRC522_Handle_t *handle,
                                 MFRC522_UID_t *uid)
{
    MFRC522_Status_t status;
    uint8_t atqa[2];
    uint32_t atqa_len = 2u;
    uint32_t len = MFRC522_UID_MAX_LEN;
    uint8_t sak = 0u;

    if ((handle == NULL) || (uid == NULL)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    mfrc522_lock(handle);

    /* Bring the card to READY (best effort; an already-ready card simply
     * does not answer WUPA, which is fine). */
    (void)mfrc522_reqa_or_wupa(handle, MFRC522_PICC_WUPA, atqa, &atqa_len,
                               MFRC522_CARD_POLL_TIMEOUT_MS);

    status = mfrc522_select_full(handle, uid->bytes, &len, &sak);
    mfrc522_unlock(handle);

    if (status != MFRC522_OK) {
        return status;
    }

    uid->length = (uint8_t)len;
    uid->sak = sak;
    return MFRC522_OK;
}

MFRC522_Status_t MFRC522_GetCardInfo(MFRC522_Handle_t *handle,
                                     MFRC522_CardInfo_t *info)
{
    MFRC522_Status_t status;
    uint8_t atqa[2];
    uint32_t atqa_len = 2u;
    uint32_t len = MFRC522_UID_MAX_LEN;
    uint8_t sak = 0u;

    if ((handle == NULL) || (info == NULL)) {
        return MFRC522_ERR_INVALID_PARAM;
    }

    mfrc522_lock(handle);

    /* Request the card so the ATQA is captured: REQA answers IDLE and
     * already-selected (READY) cards, the WUPA fallback wakes HALT'ed
     * cards. */
    status = mfrc522_request_card(handle, atqa, &atqa_len,
                                  handle->config.timeout_ms);
    if (status != MFRC522_OK) {
        mfrc522_unlock(handle);
        if (status == MFRC522_ERR_TIMEOUT) {
            return MFRC522_ERR_NO_CARD;
        }
        return status;
    }

    status = mfrc522_select_full(handle, info->uid, &len, &sak);
    mfrc522_unlock(handle);
    if (status != MFRC522_OK) {
        return status;
    }

    info->atqa[0] = atqa[0];
    info->atqa[1] = atqa[1];
    info->uid_length = (uint8_t)len;
    info->sak = sak;
    info->type = mfrc522_card_type_from_sak(sak);
    return MFRC522_OK;
}
