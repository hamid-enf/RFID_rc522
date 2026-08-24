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

typedef struct {
    uint8_t  reg[64];        /* register file                          */
    uint8_t  fifo[64];       /* FIFO ring buffer                       */
    uint8_t  fifo_head;
    uint8_t  fifo_tail;
    uint8_t  fifo_count;
    uint8_t  pending_addr;   /* SPI/UART read-after-address latch      */
    uint32_t tick;
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

    /* Version detection. */
    s = MFRC522_GetVersion(&handle, &version);
    CHECK(s == MFRC522_OK);
    CHECK(version.raw == 0x92u);
    CHECK(version.major == 9u);
    CHECK(version.minor == 2u);

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

    run_crc_tests();

    if (g_failures == 0) {
        printf("\nALL TESTS PASSED\n");
        return 0;
    }
    printf("\n%d CHECK(S) FAILED\n", g_failures);
    return 1;
}
