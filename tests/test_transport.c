/**
 * @file    test_transport.c
 * @brief   Host-side test for the MFRC522 register driver and the three
 *          transports (SPI / I2C / UART).
 *
 * A mocked MFRC522 register file emulates the device at the byte level for
 * each host interface, so the transport framing (SPI address byte, I2C
 * device/register frames, UART LSB-first bit reversal) is exercised exactly
 * as it would be on real hardware.
 *
 * Build:  cc -std=c99 -I../include test_transport.c ../src/mfrc522.c \
 *             ../src/mfrc522_registers.c ../src/mfrc522_crc.c \
 *             ../interface/mfrc522_spi.c ../interface/mfrc522_i2c.c \
 *             ../interface/mfrc522_uart.c -o test_transport
 */

#include <stdio.h>
#include <string.h>
#include "mfrc522.h"

/* ================================================================== */
/*  Mock device                                                       */
/* ================================================================== */

/* Card state machine (emulated ISO/IEC 14443-A PICC).
 *
 * Request semantics mirror real MIFARE cards:
 *   - REQA (0x26): answered in IDLE (wake) and in ACTIVE (selected/READY,
 *     where it acts as the RTSA command). Ignored in HALT and AUTH.
 *   - WUPA (0x52): answered in IDLE (wake) and HALT (wake).
 *     Ignored in ACTIVE and AUTH.
 *   - MFAuthent moves an ACTIVE card to AUTH (it then ignores requests
 *     until HALT'ed). */
#define CARD_IDLE   0u
#define CARD_ACTIVE 1u   /* selected / READY (answers MIFARE commands)      */
#define CARD_HALT   2u
#define CARD_AUTH   3u   /* Crypto1 active (post-MFAuthent)                 */

typedef struct {
    uint8_t  reg[64];        /* register file                          */
    uint8_t  fifo[64];       /* FIFO ring buffer                       */
    uint8_t  fifo_head;
    uint8_t  fifo_tail;
    uint8_t  fifo_count;
    uint8_t  pending_addr;   /* SPI/UART read-after-address latch      */
    uint32_t tick;
    /* emulated PICC */
    uint8_t  card_uid[4];    /* 4-byte UID (MIFARE 1K)                 */
    uint8_t  card_sak;
    uint8_t  card_atqa[2];
    uint8_t  card_state;
    uint8_t  card_present;    /* 0 = no card in the field at all         */
    uint8_t  card_blocks[8][MFRC522_BLOCK_SIZE]; /* a few blocks        */
    uint8_t  card_auth;      /* 1 = Crypto1 active                     */
    uint8_t  card_pending_write;      /* 1 = next frame is write data  */
    uint8_t  card_pending_block;      /* target block for pending write*/
} mock_t;

static uint32_t mock_get_tick_ms(void *ctx)
{
    mock_t *m = (mock_t *)ctx;
    return ++m->tick;
}

static void mock_delay_us(void *ctx, uint32_t us)  { (void)ctx; (void)us; }
static void mock_delay_ms(void *ctx, uint32_t ms)  { (void)ctx; (void)ms; }
static void mock_cs(void *ctx)                     { (void)ctx; }
static void mock_reset(void *ctx)                  { (void)ctx; }
static uint8_t mock_irq_read(void *ctx)            { (void)ctx; return 0u; }

static void mock_reset_all(mock_t *m)
{
    memset(m, 0, sizeof(*m));
    m->reg[MFRC522_REG_VERSION] = MFRC522_VERSION_V2_0;
    m->reg[MFRC522_REG_MODE]    = 0x3Fu;   /* reset default */
    /* Default card: MIFARE Classic 1K, UID DE AD BE EF, SAK 0x08. */
    m->card_uid[0] = 0xDEu;
    m->card_uid[1] = 0xADu;
    m->card_uid[2] = 0xBEu;
    m->card_uid[3] = 0xEFu;
    m->card_sak    = 0x08u;
    m->card_atqa[0] = 0x04u;
    m->card_atqa[1] = 0x00u;
    m->card_state  = CARD_IDLE;
    m->card_present = 1u;
    m->card_auth   = 0u;
    /* Initialize block 4 with the classic demo pattern. */
    {
        uint8_t i;
        for (i = 0u; i < 16u; i++) {
            m->card_blocks[4][i] = (uint8_t)(0x11u + (uint16_t)i * 0x11u);
        }
    }
}

static void mock_fifo_push(mock_t *m, uint8_t v);
static uint8_t mock_fifo_pop(mock_t *m);

/* Emulate the MFRC522 transceive command against the card state machine. */
static void mock_handle_transceive(mock_t *m)
{
    uint8_t tx[64];
    uint8_t n = m->fifo_count;
    uint8_t cmd;
    uint8_t i;
    uint16_t crc;

    /* Drain the TX bytes out of the FIFO. */
    for (i = 0u; i < n; i++) {
        tx[i] = mock_fifo_pop(m);
    }
    if (n == 0u) {
        return;
    }
    if (!m->card_present) {
        /* No card in the field: nothing answers any command. */
        m->reg[MFRC522_REG_COM_IRQ] |= MFRC522_IRQ_TIMER;
        return;
    }
    cmd = tx[0];

    if ((cmd == MFRC522_PICC_REQA) || (cmd == MFRC522_PICC_WUPA)) {
        /* Real-hardware semantics: REQA answers IDLE (wake) and ACTIVE
         * (selected/READY, as RTSA); WUPA answers IDLE (wake) and HALT
         * (wake). An already-selected card ignores WUPA. */
        if ((cmd == MFRC522_PICC_REQA &&
             (m->card_state == CARD_IDLE || m->card_state == CARD_ACTIVE)) ||
            (cmd == MFRC522_PICC_WUPA &&
             (m->card_state == CARD_IDLE || m->card_state == CARD_HALT))) {
            mock_fifo_push(m, m->card_atqa[0]);
            mock_fifo_push(m, m->card_atqa[1]);
            m->card_state = CARD_ACTIVE;
            m->reg[MFRC522_REG_COM_IRQ] |= (MFRC522_IRQ_RX | MFRC522_IRQ_IDLE);
        } else {
            m->reg[MFRC522_REG_COM_IRQ] |= MFRC522_IRQ_TIMER;  /* no answer */
        }
        return;
    }

    if (cmd == MFRC522_PICC_HALT) {
        if ((m->card_state == CARD_ACTIVE) || (m->card_state == CARD_AUTH)) {
            m->card_state = CARD_HALT;
        }
        m->reg[MFRC522_REG_COM_IRQ] |= MFRC522_IRQ_TIMER;      /* no answer */
        return;
    }

    if ((cmd == MFRC522_PICC_SELECT_TAG_1) || (cmd == MFRC522_PICC_SELECT_TAG_2) ||
        (cmd == MFRC522_PICC_SELECT_TAG_3)) {
        if (n >= 2u && (tx[1] & 0x70u) == 0x70u) {
            /* SELECT: reply SAK + CRC_A(SAK). */
            MFRC522_CRC_A(&m->card_sak, 1u, &crc);
            mock_fifo_push(m, m->card_sak);
            mock_fifo_push(m, (uint8_t)(crc & 0xFFu));
            mock_fifo_push(m, (uint8_t)(crc >> 8));
            m->card_state = CARD_ACTIVE;
        } else {
            /* ANTICOLLISION: reply with the 4-byte UID. */
            for (i = 0u; i < 4u; i++) {
                mock_fifo_push(m, m->card_uid[i]);
            }
        }
        m->reg[MFRC522_REG_COM_IRQ] |= (MFRC522_IRQ_RX | MFRC522_IRQ_IDLE);
        return;
    }

    if (cmd == MFRC522_MF_READ) {
        /* READ: reply 16 data bytes + CRC_A. */
        uint8_t blk = tx[1];
        uint8_t j;
        for (j = 0u; j < 16u; j++) {
            mock_fifo_push(m, m->card_blocks[blk & 0x07u][j]);
        }
        MFRC522_CRC_A(m->card_blocks[blk & 0x07u], 16u, &crc);
        mock_fifo_push(m, (uint8_t)(crc & 0xFFu));
        mock_fifo_push(m, (uint8_t)(crc >> 8));
        m->reg[MFRC522_REG_CONTROL] = 0x00u;   /* full bytes */
        m->reg[MFRC522_REG_COM_IRQ] |= (MFRC522_IRQ_RX | MFRC522_IRQ_IDLE);
        return;
    }

    /* Pending write data (step 2 of MIFARE WRITE: 16 data bytes + CRC). */
    if (m->card_pending_write != 0u) {
        uint8_t blk = m->card_pending_block;
        uint8_t j;
        m->card_pending_write = 0u;
        for (j = 0u; j < 16u; j++) {
            m->card_blocks[blk & 0x07u][j] = tx[j];
        }
        mock_fifo_push(m, MFRC522_MF_ACK);
        m->reg[MFRC522_REG_CONTROL] = 0x04u;   /* 4-bit nibble */
        m->reg[MFRC522_REG_COM_IRQ] |= (MFRC522_IRQ_RX | MFRC522_IRQ_IDLE);
        return;
    }

    if (cmd == MFRC522_MF_WRITE) {
        /* WRITE step 1 (cmd + block): latch the target, reply ACK. */
        m->card_pending_write = 1u;
        m->card_pending_block = tx[1];
        mock_fifo_push(m, MFRC522_MF_ACK);
        m->reg[MFRC522_REG_CONTROL] = 0x04u;   /* 4-bit nibble */
        m->reg[MFRC522_REG_COM_IRQ] |= (MFRC522_IRQ_RX | MFRC522_IRQ_IDLE);
        return;
    }

    if ((cmd == MFRC522_MF_INCREMENT) || (cmd == MFRC522_MF_DECREMENT) ||
        (cmd == MFRC522_MF_RESTORE) || (cmd == MFRC522_MF_TRANSFER)) {
        /* Value ops: acknowledge the (first) step. */
        mock_fifo_push(m, MFRC522_MF_ACK);
        m->reg[MFRC522_REG_CONTROL] = 0x04u;   /* 4-bit nibble */
        m->reg[MFRC522_REG_COM_IRQ] |= (MFRC522_IRQ_RX | MFRC522_IRQ_IDLE);
        return;
    }
}

static void mock_fifo_push(mock_t *m, uint8_t v)
{
    if (m->fifo_count < MFRC522_FIFO_SIZE) {
        m->fifo[m->fifo_tail] = v;
        m->fifo_tail = (uint8_t)((m->fifo_tail + 1u) & 63u);
        m->fifo_count++;
    }
}

static uint8_t mock_fifo_pop(mock_t *m)
{
    uint8_t v = 0u;
    if (m->fifo_count > 0u) {
        v = m->fifo[m->fifo_head];
        m->fifo_head = (uint8_t)((m->fifo_head + 1u) & 63u);
        m->fifo_count--;
    }
    return v;
}

static void mock_write_reg(mock_t *m, uint8_t addr, uint8_t val)
{
    if (addr == MFRC522_REG_FIFO_DATA) {
        mock_fifo_push(m, val);
        return;
    }
    if (addr == MFRC522_REG_FIFO_LEVEL) {
        if ((val & MFRC522_FIFO_LEVEL_FLUSH) != 0u) {
            m->fifo_head = 0u;
            m->fifo_tail = 0u;
            m->fifo_count = 0u;
        }
        return;
    }
    if ((addr == MFRC522_REG_COM_IRQ) || (addr == MFRC522_REG_DIV_IRQ)) {
        /* Write-1-to-clear semantics. */
        m->reg[addr] &= (uint8_t)~val;
        return;
    }
    if (addr == MFRC522_REG_COMMAND) {
        m->reg[addr] = val;
        if (val == MFRC522_CMD_SOFT_RESET) {
            m->reg[MFRC522_REG_COMMAND] = 0x00u;
        } else if (val == MFRC522_CMD_CALC_CRC) {
            uint16_t crc;
            MFRC522_CRC_A(m->fifo, m->fifo_count, &crc);
            m->reg[MFRC522_REG_CRC_RESULT_LSB] = (uint8_t)(crc & 0xFFu);
            m->reg[MFRC522_REG_CRC_RESULT_MSB] = (uint8_t)(crc >> 8);
            m->reg[MFRC522_REG_DIV_IRQ] |= MFRC522_DIV_IRQ_CRC;
        } else if (val == MFRC522_CMD_TRANSCEIVE) {
            mock_handle_transceive(m);
        } else if (val == MFRC522_CMD_MF_AUTHENT) {
            m->card_auth = 1u;
            m->card_state = CARD_AUTH;   /* Crypto1 active: requests ignored */
            m->reg[MFRC522_REG_STATUS2] |= MFRC522_STATUS2_CRYPTO1_ON;
            m->reg[MFRC522_REG_COM_IRQ] |= MFRC522_IRQ_IDLE;
        }
        return;
    }
    m->reg[addr] = val;
}

static uint8_t mock_read_reg(mock_t *m, uint8_t addr)
{
    if (addr == MFRC522_REG_FIFO_DATA) {
        return mock_fifo_pop(m);
    }
    if (addr == MFRC522_REG_FIFO_LEVEL) {
        return m->fifo_count;
    }
    return m->reg[addr];
}

/* Auto-incrementing access, except FIFO_DATA which is a non-incrementing
 * window register (every byte goes to / comes from the FIFO). */
static void mock_write_auto(mock_t *m, uint8_t base, const uint8_t *data,
                            uint32_t len)
{
    uint32_t i;
    if (base == MFRC522_REG_FIFO_DATA) {
        for (i = 0u; i < len; i++) {
            mock_write_reg(m, MFRC522_REG_FIFO_DATA, data[i]);
        }
    } else {
        for (i = 0u; i < len; i++) {
            mock_write_reg(m, (uint8_t)(base + i), data[i]);
        }
    }
}

static void mock_read_auto(mock_t *m, uint8_t base, uint8_t *data, uint32_t len)
{
    uint32_t i;
    if (base == MFRC522_REG_FIFO_DATA) {
        for (i = 0u; i < len; i++) {
            data[i] = mock_read_reg(m, MFRC522_REG_FIFO_DATA);
        }
    } else {
        for (i = 0u; i < len; i++) {
            data[i] = mock_read_reg(m, (uint8_t)(base + i));
        }
    }
}

/* ---- SPI wire behaviour (mode bit = bit 7 of the address byte) --- */
static MFRC522_Status_t spi_transmit(void *ctx, const uint8_t *tx, uint32_t len)
{
    mock_t *m = (mock_t *)ctx;
    uint8_t ab = tx[0];

    if ((ab & 0x80u) != 0u) {
        m->pending_addr = (uint8_t)((ab >> 1) & 0x3Fu);   /* read */
    } else {
        mock_write_auto(m, (uint8_t)((ab >> 1) & 0x3Fu), &tx[1], len - 1u);
    }
    return MFRC522_OK;
}

static MFRC522_Status_t spi_receive(void *ctx, uint8_t *rx, uint32_t len)
{
    mock_t *m = (mock_t *)ctx;
    mock_read_auto(m, m->pending_addr, rx, len);
    return MFRC522_OK;
}

/* ---- I2C wire behaviour ------------------------------------------ */
static MFRC522_Status_t i2c_transmit(void *ctx, const uint8_t *tx, uint32_t len)
{
    mock_t *m = (mock_t *)ctx;
    mock_write_auto(m, tx[0], &tx[1], len - 1u);
    return MFRC522_OK;
}

static MFRC522_Status_t i2c_write_read(void *ctx, const uint8_t *tx,
                                       uint32_t tx_len, uint8_t *rx, uint32_t rx_len)
{
    mock_t *m = (mock_t *)ctx;
    (void)tx_len;
    mock_read_auto(m, tx[0], rx, rx_len);
    return MFRC522_OK;
}

/* ---- UART wire behaviour (LSB-first) ----------------------------- */
static uint8_t test_rev8(uint8_t b)
{
    b = (uint8_t)(((b & 0xF0u) >> 4) | ((b & 0x0Fu) << 4));
    b = (uint8_t)(((b & 0xCCu) >> 2) | ((b & 0x33u) << 2));
    b = (uint8_t)(((b & 0xAAu) >> 1) | ((b & 0x55u) << 1));
    return b;
}

static MFRC522_Status_t uart_transmit(void *ctx, const uint8_t *tx, uint32_t len)
{
    mock_t *m = (mock_t *)ctx;
    uint8_t ab = test_rev8(tx[0]);
    uint8_t tmp[64];
    uint32_t i;

    if ((ab & 0x80u) != 0u) {
        m->pending_addr = (uint8_t)(ab & 0x3Fu);   /* read */
    } else {
        for (i = 1u; i < len; i++) {
            tmp[i - 1u] = test_rev8(tx[i]);
        }
        mock_write_auto(m, (uint8_t)(ab & 0x3Fu), tmp, len - 1u);
    }
    return MFRC522_OK;
}

static MFRC522_Status_t uart_receive(void *ctx, uint8_t *rx, uint32_t len)
{
    mock_t *m = (mock_t *)ctx;
    uint32_t i;

    mock_read_auto(m, m->pending_addr, rx, len);
    for (i = 0u; i < len; i++) {
        rx[i] = test_rev8(rx[i]);
    }
    return MFRC522_OK;
}

/* ================================================================== */
/*  Platform ops tables                                               */
/* ================================================================== */

static const MFRC522_PlatformOps_t SPI_OPS = {
    mock_delay_us, mock_delay_ms, mock_get_tick_ms,
    mock_cs, mock_cs, mock_reset, mock_reset, mock_irq_read,
    spi_transmit, spi_receive, NULL,
    NULL, NULL, NULL
};

static const MFRC522_PlatformOps_t I2C_OPS = {
    mock_delay_us, mock_delay_ms, mock_get_tick_ms,
    mock_cs, mock_cs, mock_reset, mock_reset, mock_irq_read,
    i2c_transmit, NULL, NULL,
    i2c_write_read, NULL, NULL
};

static const MFRC522_PlatformOps_t UART_OPS = {
    mock_delay_us, mock_delay_ms, mock_get_tick_ms,
    mock_cs, mock_cs, mock_reset, mock_reset, mock_irq_read,
    uart_transmit, uart_receive, NULL,
    NULL, NULL, NULL
};

/* ================================================================== */
/*  Test harness                                                      */
/* ================================================================== */

static int g_failures = 0;

#define CHECK(cond) do {                                                   \
    if (!(cond)) {                                                         \
        printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);           \
        g_failures++;                                                      \
    }                                                                      \
} while (0)

static void run_transport_tests(MFRC522_TransportType_t type,
                                const MFRC522_TransportOps_t *t_ops,
                                const MFRC522_PlatformOps_t *p_ops,
                                const char *name)
{
    mock_t mock;
    MFRC522_Handle_t handle;
    MFRC522_Version_t version;
    uint8_t value;
    uint8_t buf[8];
    uint16_t crc;
    MFRC522_Status_t s;

    printf("== %s transport ==\n", name);

    mock_reset_all(&mock);
    memset(&handle, 0, sizeof(handle));
    handle.transport.type = type;
    handle.transport.uart_baud = MFRC522_UART_BAUD_115200;
    handle.transport.i2c_addr = MFRC522_I2C_DEFAULT_ADDR;
    handle.transport.spi_speed = MFRC522_SPI_SPEED_LOW;
    handle.transport_ops = t_ops;
    handle.platform.ops = p_ops;
    handle.platform.ctx = &mock;

    /* Full init against the mock. */
    s = MFRC522_Init(&handle);
    CHECK(s == MFRC522_OK);
    CHECK((handle.state.flags & MFRC522_FLAG_INITIALIZED) != 0u);

    /* Version detection: 0x92 is silicon v2.0 (high nibble = device
     * family, low nibble = version). */
    s = MFRC522_GetVersion(&handle, &version);
    CHECK(s == MFRC522_OK);
    CHECK(version.raw == 0x92u);
    CHECK(version.major == 2u);
    CHECK(version.minor == 0u);

    /* Register round-trip. */
    s = MFRC522_WriteRegister(&handle, 0x25u, 0xA5u);
    CHECK(s == MFRC522_OK);
    s = MFRC522_ReadRegister(&handle, 0x25u, &value);
    CHECK(s == MFRC522_OK);
    CHECK(value == 0xA5u);

    /* Set / clear bits (read-modify-write). */
    s = MFRC522_SetBits(&handle, 0x25u, 0x0Fu);
    CHECK(s == MFRC522_OK);
    CHECK(mock.reg[0x25u] == 0xAFu);
    s = MFRC522_ClearBits(&handle, 0x25u, 0x0Fu);
    CHECK(s == MFRC522_OK);
    CHECK(mock.reg[0x25u] == 0xA0u);

    /* FIFO write / level / read. */
    buf[0] = 0x11u; buf[1] = 0x22u; buf[2] = 0x33u; buf[3] = 0x44u;
    s = MFRC522_WriteFIFO(&handle, buf, 4u);
    CHECK(s == MFRC522_OK);
    CHECK(mock.fifo_count == 4u);

    s = MFRC522_ReadFIFO(&handle, buf, 4u);
    CHECK(s == MFRC522_OK);
    CHECK(buf[0] == 0x11u && buf[1] == 0x22u && buf[2] == 0x33u && buf[3] == 0x44u);
    CHECK(mock.fifo_count == 0u);

    /* Hardware CRC coprocessor (emulated): CRC_A{0x50,0x00} == 0xCD57. */
    buf[0] = 0x50u; buf[1] = 0x00u;
    s = MFRC522_CalcCRC(&handle, buf, 2u, &crc);
    CHECK(s == MFRC522_OK);
    CHECK(crc == 0xCD57u);

    /* Antenna on/off. */
    s = MFRC522_AntennaOn(&handle);
    CHECK(s == MFRC522_OK);
    CHECK((mock.reg[MFRC522_REG_TX_CONTROL] & MFRC522_TX_CONTROL_RF_EN_MASK)
          == MFRC522_TX_CONTROL_RF_EN_MASK);
    CHECK(MFRC522_IsAntennaOn(&handle) == 1u);
    s = MFRC522_AntennaOff(&handle);
    CHECK(s == MFRC522_OK);
    CHECK(MFRC522_IsAntennaOn(&handle) == 0u);
}

static void run_crc_tests(void)
{
    uint8_t data[6] = { 0x93u, 0x20u, 0x88u, 0x04u, 0xABu, 0xCDu };
    uint8_t with_crc[8];
    uint16_t crc;
    uint16_t residue;

    printf("== software CRC_A ==\n");

    /* Known reference: MIFARE HALT frame 50 00 -> CRC 0xCD57 (0x57 0xCD on air). */
    MFRC522_CRC_A((const uint8_t *)"\x50\x00", 2u, &crc);
    CHECK(crc == 0xCD57u);

    /* Empty input yields the initial value. */
    MFRC522_CRC_A(NULL, 0u, &crc);
    CHECK(crc == 0x6363u);

    /* Self-check: appending the CRC (LSB first) yields residue 0x0000. */
    MFRC522_CRC_A(data, 6u, &crc);
    memcpy(with_crc, data, 6u);
    with_crc[6] = (uint8_t)(crc & 0xFFu);   /* LSB first */
    with_crc[7] = (uint8_t)(crc >> 8);
    MFRC522_CRC_A(with_crc, 8u, &residue);
    CHECK(residue == 0x0000u);
}

static void run_protocol_tests(void)
{
    mock_t mock;
    MFRC522_Handle_t handle;
    MFRC522_UID_t uid;
    MFRC522_CardInfo_t info;
    MFRC522_Status_t s;

    printf("== protocol (card emulation) ==\n");

    mock_reset_all(&mock);
    memset(&handle, 0, sizeof(handle));
    handle.transport.type = MFRC522_TRANSPORT_SPI;
    handle.transport_ops = &MFRC522_SPI_TransportOps;
    handle.platform.ops = &SPI_OPS;
    handle.platform.ctx = &mock;

    CHECK(MFRC522_Init(&handle) == MFRC522_OK);

    /* Card present (REQA wakes the IDLE card; the card is now selected). */
    CHECK(MFRC522_IsCardPresent(&handle) == MFRC522_OK);
    CHECK(mock.card_state == CARD_ACTIVE);

    /* Read the UID (select leaves the card ACTIVE). */
    memset(&uid, 0, sizeof(uid));
    s = MFRC522_ReadUID(&handle, &uid);
    CHECK(s == MFRC522_OK);
    CHECK(uid.length == 4u);
    CHECK(uid.bytes[0] == 0xDEu && uid.bytes[1] == 0xADu &&
          uid.bytes[2] == 0xBEu && uid.bytes[3] == 0xEFu);
    CHECK(uid.sak == 0x08u);
    CHECK(mock.card_state == CARD_ACTIVE);

    /* Halt the card. */
    CHECK(MFRC522_HaltTag(&handle) == MFRC522_OK);
    CHECK(mock.card_state == CARD_HALT);

    /* GetCardInfo: WUPA wakes the HALTed card, captures ATQA + select. */
    memset(&info, 0, sizeof(info));
    s = MFRC522_GetCardInfo(&handle, &info);
    CHECK(s == MFRC522_OK);
    CHECK(info.atqa[0] == 0x04u && info.atqa[1] == 0x00u);
    CHECK(info.uid_length == 4u);
    CHECK(info.sak == 0x08u);
    CHECK(info.type == MFRC522_CARD_MIFARE_1K);
    CHECK(info.uid[0] == 0xDEu && info.uid[3] == 0xEFu);

    /* No card in the field: IsCardPresent must report NO_CARD. */
    mock.card_present = 0u;
    CHECK(MFRC522_IsCardPresent(&handle) == MFRC522_ERR_NO_CARD);
    mock.card_present = 1u;
}

/* ================================================================== */
/*  Datasheet value guards                                             */
/* ================================================================== */

/**
 * @brief The mock in this file is self-consistent with the driver, so a
 *        wrong command code or register address is never caught by the
 *        behavioural tests (it "works" against the mock and silently
 *        breaks real hardware). These checks pin the values to the NXP
 *        MFRC522 datasheet (see docs/register_map.md) so a typo fails
 *        the host test run.
 */
static void run_datasheet_value_tests(void)
{
    printf("== datasheet value guards ==\n");

    /* CommandReg command codes (NXP "Command" table). */
    CHECK(MFRC522_CMD_IDLE == 0x00u);
    CHECK(MFRC522_CMD_MEM == 0x01u);
    CHECK(MFRC522_CMD_GENERATE_RANDOM_ID == 0x02u);
    CHECK(MFRC522_CMD_CALC_CRC == 0x03u);
    CHECK(MFRC522_CMD_TRANSMIT == 0x04u);
    CHECK(MFRC522_CMD_NO_CMD_CHANGE == 0x07u);
    CHECK(MFRC522_CMD_RECEIVE == 0x08u);
    CHECK(MFRC522_CMD_TRANSCEIVE == 0x0Cu);
    CHECK(MFRC522_CMD_MF_AUTHENT == 0x0Eu);
    CHECK(MFRC522_CMD_SOFT_RESET == 0x0Fu);

    /* Key register addresses (NXP register overview). */
    CHECK(MFRC522_REG_COMMAND == 0x01u);
    CHECK(MFRC522_REG_COM_IRQ == 0x04u);
    CHECK(MFRC522_REG_FIFO_DATA == 0x09u);
    CHECK(MFRC522_REG_FIFO_LEVEL == 0x0Au);
    CHECK(MFRC522_REG_CONTROL == 0x0Cu);
    CHECK(MFRC522_REG_BIT_FRAMING == 0x0Du);
    CHECK(MFRC522_REG_MODE == 0x11u);
    CHECK(MFRC522_REG_TX_CONTROL == 0x14u);
    CHECK(MFRC522_REG_TX_ASK == 0x15u);
    CHECK(MFRC522_REG_SERIAL_SPEED == 0x1Fu);
    CHECK(MFRC522_REG_CRC_RESULT_MSB == 0x21u);
    CHECK(MFRC522_REG_CRC_RESULT_LSB == 0x22u);
    CHECK(MFRC522_REG_MOD_WIDTH == 0x24u);
    CHECK(MFRC522_REG_T_MODE == 0x2Au);
    CHECK(MFRC522_REG_AUTO_TEST == 0x36u);
    CHECK(MFRC522_REG_VERSION == 0x37u);
}

/* ================================================================== */
/*  Version gate tests                                                 */
/* ================================================================== */

/**
 * @brief MFRC522_Init must accept the NXP silicon versions and the known
 *        compatible-clone revisions, and reject anything else.
 */
static void run_version_gate_tests(void)
{
    mock_t mock;
    MFRC522_Handle_t handle;
    static const uint8_t accepted[] =
        { 0x90u, 0x91u, 0x92u, 0x88u, 0xB2u };
    uint8_t i;

    printf("== version gate ==\n");

    for (i = 0u; i < (uint8_t)(sizeof(accepted) / sizeof(accepted[0])); i++) {
        mock_reset_all(&mock);
        mock.reg[MFRC522_REG_VERSION] = accepted[i];
        memset(&handle, 0, sizeof(handle));
        handle.transport.type = MFRC522_TRANSPORT_SPI;
        handle.transport_ops = &MFRC522_SPI_TransportOps;
        handle.platform.ops = &SPI_OPS;
        handle.platform.ctx = &mock;
        CHECK(MFRC522_Init(&handle) == MFRC522_OK);
    }

    /* Unknown version -> ERR_DEVICE. */
    mock_reset_all(&mock);
    mock.reg[MFRC522_REG_VERSION] = 0x00u;
    memset(&handle, 0, sizeof(handle));
    handle.transport.type = MFRC522_TRANSPORT_SPI;
    handle.transport_ops = &MFRC522_SPI_TransportOps;
    handle.platform.ops = &SPI_OPS;
    handle.platform.ctx = &mock;
    CHECK(MFRC522_Init(&handle) == MFRC522_ERR_DEVICE);
}

/* ================================================================== */
/*  Presence / polling regression tests                                */
/* ================================================================== */

/**
 * @brief Regression tests for the ISO/IEC 14443-A request semantics.
 *
 * These guard against the "WUPA-only polling" bug: a card in the READY
 * (selected, not halted) state ignores WUPA, so presence detection must
 * send REQA first and fall back to WUPA. Covers the flows of examples 02,
 * 03, 04 and 15.
 */
static void run_presence_regression_tests(void)
{
    mock_t mock;
    MFRC522_Handle_t handle;
    MFRC522_UID_t uid;
    MFRC522_CardInfo_t info;
    MFRC522_Status_t s;

    printf("== presence / polling (ISO 14443-A state machine) ==\n");

    mock_reset_all(&mock);
    memset(&handle, 0, sizeof(handle));
    handle.transport.type = MFRC522_TRANSPORT_SPI;
    handle.transport_ops = &MFRC522_SPI_TransportOps;
    handle.platform.ops = &SPI_OPS;
    handle.platform.ctx = &mock;

    CHECK(MFRC522_Init(&handle) == MFRC522_OK);

    /* IDLE card: detected, and now selected (READY). */
    CHECK(MFRC522_IsCardPresent(&handle) == MFRC522_OK);
    CHECK(mock.card_state == CARD_ACTIVE);

    /* A selected, non-halted card must STILL be detected (REQA/RTSA;
     * WUPA alone would report NO_CARD here). */
    CHECK(MFRC522_IsCardPresent(&handle) == MFRC522_OK);

    /* Example 02 flow: blocking wait right after a detection must
     * succeed while the same card is in the field. */
    CHECK(MFRC522_WaitForCard(&handle, 500u) == MFRC522_OK);

    /* Example 04/15 flow: full card info after a UID read must work
     * on the already-selected card. */
    memset(&uid, 0, sizeof(uid));
    CHECK(MFRC522_ReadUID(&handle, &uid) == MFRC522_OK);
    memset(&info, 0, sizeof(info));
    s = MFRC522_GetCardInfo(&handle, &info);
    CHECK(s == MFRC522_OK);
    CHECK(info.atqa[0] == 0x04u && info.atqa[1] == 0x00u);
    CHECK(info.uid_length == 4u);
    CHECK(info.sak == 0x08u);
    CHECK(info.type == MFRC522_CARD_MIFARE_1K);
    CHECK(info.uid[0] == 0xDEu && info.uid[3] == 0xEFu);

    /* Halted card: REQA fails, the WUPA fallback wakes it again. */
    CHECK(MFRC522_HaltTag(&handle) == MFRC522_OK);
    CHECK(mock.card_state == CARD_HALT);
    CHECK(MFRC522_IsCardPresent(&handle) == MFRC522_OK);
    CHECK(mock.card_state == CARD_ACTIVE);

    /* Documented limitation: an authenticated (post-MFAuthent) card
     * ignores requests until it leaves that state. */
    mock.card_state = CARD_AUTH;
    CHECK(MFRC522_IsCardPresent(&handle) == MFRC522_ERR_NO_CARD);

    /* No card at all in the field. */
    mock.card_state = CARD_IDLE;
    mock.card_present = 0u;
    CHECK(MFRC522_IsCardPresent(&handle) == MFRC522_ERR_NO_CARD);
    mock.card_present = 1u;
    CHECK(MFRC522_IsCardPresent(&handle) == MFRC522_OK);
}

#if MFRC522_ENABLE_MIFARE
static void run_mifare_tests(void)
{
    mock_t mock;
    MFRC522_Handle_t handle;
    MFRC522_Key_t key;
    MFRC522_UID_t uid;
    uint8_t data[16];
    uint8_t block[16];
    int32_t value = 1234567;
    uint8_t i;
    MFRC522_Status_t s;

    printf("== MIFARE ==\n");

    mock_reset_all(&mock);
    memset(&handle, 0, sizeof(handle));
    handle.transport.type = MFRC522_TRANSPORT_SPI;
    handle.transport_ops = &MFRC522_SPI_TransportOps;
    handle.platform.ops = &SPI_OPS;
    handle.platform.ctx = &mock;

    CHECK(MFRC522_Init(&handle) == MFRC522_OK);

    /* Select the card, then authenticate with the factory key. */
    memset(&uid, 0, sizeof(uid));
    CHECK(MFRC522_ReadUID(&handle, &uid) == MFRC522_OK);

    for (i = 0; i < 6; i++) key.key[i] = 0xFFu;
    s = MFRC522_Authenticate(&handle, MFRC522_KEY_A, &key, 4u,
                             uid.bytes, uid.length);
    CHECK(s == MFRC522_OK);
    CHECK(mock.card_auth == 1u);

    /* Read block 4. */
    memset(data, 0, sizeof(data));
    s = MFRC522_ReadBlock(&handle, 4u, data);
    CHECK(s == MFRC522_OK);
    CHECK(data[0] == 0x11u);
    CHECK(data[1] == 0x22u);
    CHECK(data[15] == (uint8_t)(0x11u + 15u * 0x11u)); /* 0x11+0xFF=0x10? verify */

    /* Write block 5, then read it back. */
    for (i = 0; i < 16; i++) data[i] = (uint8_t)(0xA0u + i);
    s = MFRC522_WriteBlock(&handle, 5u, data);
    CHECK(s == MFRC522_OK);

    memset(data, 0, sizeof(data));
    s = MFRC522_ReadBlock(&handle, 5u, data);
    CHECK(s == MFRC522_OK);
    CHECK(data[0] == 0xA0u);
    CHECK(data[15] == 0xAFu);

    /* Stop Crypto1. */
    CHECK(MFRC522_StopCrypto1(&handle) == MFRC522_OK);

    /* Value-block formatting is deterministic. */
    MFRC522_FormatValueBlock(block, value, 0x05u);
    {
        uint8_t inv0 = (uint8_t)(block[0] ^ 0xFFu);
        uint8_t inv_addr = (uint8_t)(0x05u ^ 0xFFu);
        CHECK(block[0] == (uint8_t)(value & 0xFFu));
        CHECK(block[4] == inv0);
        CHECK(block[8] == block[0]);
        CHECK(block[12] == 0x05u);
        CHECK(block[13] == inv_addr);
    }

    /* Value ops run their two-step protocol against the mock. */
    CHECK(MFRC522_Increment(&handle, 5u, 1) == MFRC522_OK);
    CHECK(MFRC522_Decrement(&handle, 5u, 1) == MFRC522_OK);
    CHECK(MFRC522_Restore(&handle, 5u) == MFRC522_OK);
    CHECK(MFRC522_Transfer(&handle, 5u) == MFRC522_OK);
}
#endif

#if MFRC522_ENABLE_IRQ
static uint8_t g_irq_hits = 0;
static uint8_t g_irq_sources = 0;

static void irq_cb(MFRC522_Handle_t *handle, uint8_t irq_source, void *user)
{
    (void)handle;
    (void)user;
    g_irq_hits++;
    g_irq_sources = irq_source;
}

static void run_irq_test(void)
{
    mock_t mock;
    MFRC522_Handle_t handle;

    printf("== IRQ dispatcher ==\n");

    mock_reset_all(&mock);
    memset(&handle, 0, sizeof(handle));
    handle.transport.type = MFRC522_TRANSPORT_SPI;
    handle.transport_ops = &MFRC522_SPI_TransportOps;
    handle.platform.ops = &SPI_OPS;
    handle.platform.ctx = &mock;

    CHECK(MFRC522_Init(&handle) == MFRC522_OK);

    g_irq_hits = 0;
    g_irq_sources = 0;
    CHECK(MFRC522_AttachIRQCallback(&handle, irq_cb, NULL) == MFRC522_OK);

    /* Simulate an RX + Idle interrupt, then service it. */
    mock.reg[MFRC522_REG_COM_IRQ] = MFRC522_IRQ_RX | MFRC522_IRQ_IDLE;
    CHECK(MFRC522_ProcessIRQ(&handle) == MFRC522_OK);
    CHECK(g_irq_hits == 1u);
    CHECK(g_irq_sources == (MFRC522_IRQ_RX | MFRC522_IRQ_IDLE));
    /* The latched bits must have been cleared. */
    CHECK(mock.reg[MFRC522_REG_COM_IRQ] == 0u);
}
#endif

int main(void)
{
    printf("MFRC522 transport / register-driver tests\n");
    printf("-----------------------------------------\n");

#if MFRC522_ENABLE_SPI
    run_transport_tests(MFRC522_TRANSPORT_SPI, &MFRC522_SPI_TransportOps,
                        &SPI_OPS, "SPI");
#endif
#if MFRC522_ENABLE_I2C
    run_transport_tests(MFRC522_TRANSPORT_I2C, &MFRC522_I2C_TransportOps,
                        &I2C_OPS, "I2C");
#endif
#if MFRC522_ENABLE_UART
    run_transport_tests(MFRC522_TRANSPORT_UART, &MFRC522_UART_TransportOps,
                        &UART_OPS, "UART");
#endif

#if MFRC522_ENABLE_IRQ
    run_irq_test();
#endif

    run_datasheet_value_tests();
    run_protocol_tests();
    run_version_gate_tests();
    run_presence_regression_tests();
#if MFRC522_ENABLE_MIFARE
    run_mifare_tests();
#endif
    run_crc_tests();

    if (g_failures == 0) {
        printf("\nALL TESTS PASSED\n");
        return 0;
    }
    printf("\n%d CHECK(S) FAILED\n", g_failures);
    return 1;
}
