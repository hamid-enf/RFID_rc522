/**
 * @file    main.c
 * @brief   Example 01 — basic initialization.
 *
 * Attaches the SPI adapter, initializes the reader, reads the firmware
 * version, runs the self-test and confirms the antenna is on.
 *
 * Wiring: see docs/stm32h743.md (SPI table).
 */

#include "mfrc522_demo.h"

MFRC522_Handle_t rfid = {0};

/**
 * @brief Dump the raw register file over the console (hardware diagnostics).
 *
 * Run when MFRC522_Init() fails with MFRC522_ERR_DEVICE: it shows what the
 * chip is actually answering over SPI, independent of the version check.
 *
 * Interpretation:
 *   - Healthy chip:  0x0B=0x40 (WaterLevel), 0x19=0x4D (Demod),
 *     0x1C=0x62 (MfTx), 0x24=0x26 (ModWidth),
 *     0x37=0x92 (v2.0) / 0x91 (v1.0) / 0x90 (v0.0); 0x88 = FM17522 clone,
 *     0xB2 = MFRC522-compatible clone revision.
 *   - Everything 0x00: the chip is not answering at all -> check NRSTPD
 *     (must be HIGH), CS wiring/mode, VCC=3.3 V and common GND.
 *   - Everything 0xFF: MISO line not connected / missing pull-up.
 *   - Other garbage:   check SCK speed, SPI mode 0, wiring.
 */
static void demo_hw_diag(void)
{
    uint8_t addr, value;

    demo_printf("\n--- HW diagnostic: raw register scan ---\n");
    for (addr = 0x01u; addr <= 0x3Bu; addr++) {
        if (MFRC522_ReadRegister(&rfid, addr, &value) == MFRC522_OK) {
            demo_printf("0x%02X: 0x%02X\n", addr, value);
        } else {
            demo_printf("0x%02X: READ FAIL\n", addr);
        }
    }
    demo_printf("Expect: 0x0B=40 0x19=4D 0x1C=62 0x24=26 0x37=92/91/90/88/B2\n");
    demo_printf("All 00 -> chip not answering (NRSTPD? CS? power?)\n");
    demo_printf("All FF -> MISO not connected\n");
    demo_printf("---- end of scan ----\n");
}

/**
 * @brief Raw SPI probe: one manual VersionReg (0x37) read around the HAL
 *        calls, printing the exact HAL status codes.
 *
 * Interpretation (tx/rx: 0=OK, 1=ERROR, 2=BUSY, 3=TIMEOUT):
 *   - tx or rx = 3 (TIMEOUT): the SPI transfer never completes -> check the
 *     SPI1 clock source / BSY flag.
 *   - tx or rx = 1 (ERROR) with HAL error = 1 (OVR): RX overrun -> the RX
 *     FIFO is not being drained (see notes in docs/troubleshooting.md).
 *   - tx or rx = 2 (BUSY): the HAL handle is stuck -> something corrupted
 *     the SPI state before this point.
 *   - tx = rx = 0 and value = 0x92/0x91/0x88: the raw SPI works; the failure
 *     comes from the driver's transfer sequence, report it upstream.
 *
 * Run it TWICE to localize the fault: once right after MX_SPI1_Init()
 * (clean state) and once after a failed MFRC522_Init().
 */
static void demo_spi_raw_probe(void)
{
    uint8_t addr = (uint8_t)((0x37u << 1) | 0x80u);
    uint8_t value = 0u;
    HAL_StatusTypeDef s_tx, s_rx;

    demo_printf("\n--- raw SPI probe (VersionReg 0x37) ---\n");
    demo_printf("SPI State before: %u\n", (unsigned int)hspi1.State);

    HAL_GPIO_WritePin(RFID_CS_GPIO_Port, RFID_CS_Pin, GPIO_PIN_RESET);
    HAL_Delay(1u);
    s_tx = HAL_SPI_Transmit(&hspi1, &addr, 1u, 100u);
    s_rx = HAL_SPI_Receive(&hspi1, &value, 1u, 100u);
    HAL_GPIO_WritePin(RFID_CS_GPIO_Port, RFID_CS_Pin, GPIO_PIN_SET);

    demo_printf("tx=%u rx=%u value=0x%02X\n",
                (unsigned int)s_tx, (unsigned int)s_rx, value);
    demo_printf("SPI_SR=0x%08lX\n", (unsigned long)hspi1.Instance->SR);
    demo_printf("HAL error=0x%02X (1=OVR 2=MODF 4=CRC 8=FRE)\n",
                (unsigned int)HAL_SPI_GetError(&hspi1));
    demo_printf("SPI State after: %u\n", (unsigned int)hspi1.State);
    demo_printf("---- end of probe ----\n");
}

/**
 * @brief Raw canary scan: read registers with fixed reset values directly
 *        through the HAL, bypassing the driver.
 *
 * Canary registers (datasheet reset values, confirmed on real silicon):
 *   0x0B WaterLevelReg = 0x40, 0x19 DemodReg = 0x4D, 0x1C MfTxReg = 0x62,
 *   0x24 ModWidthReg = 0x26, 0x37 VersionReg = 0x92/0x91/0x90/0x88/0xB2.
 *
 * Separates "wrong silicon" from "corrupt data path":
 *   - If the canaries read their datasheet values but 0x37 is an unknown
 *     version, the data path is fine and the chip is a different/clone
 *     revision -> report the 0x37 value so it can be added to the accepted
 *     list.
 *   - If ALL values are garbage, the data path is corrupt (SCK too fast for
 *     the board's signal quality, MISO routing) -> lower the SPI clock
 *     (e.g. prescaler 128/256) and re-run.
 */
static void demo_spi_raw_scan(void)
{
    static const uint8_t regs[] = { 0x0Bu, 0x19u, 0x1Cu, 0x24u, 0x37u };
    uint8_t i;

    demo_printf("\n--- raw SPI canary scan ---\n");
    for (i = 0u; i < (uint8_t)(sizeof(regs) / sizeof(regs[0])); i++) {
        uint8_t addr = (uint8_t)((regs[i] << 1) | 0x80u);
        uint8_t value = 0u;
        HAL_StatusTypeDef s_tx, s_rx;

        HAL_GPIO_WritePin(RFID_CS_GPIO_Port, RFID_CS_Pin, GPIO_PIN_RESET);
        HAL_Delay(1u);
        s_tx = HAL_SPI_Transmit(&hspi1, &addr, 1u, 100u);
        s_rx = HAL_SPI_Receive(&hspi1, &value, 1u, 100u);
        HAL_GPIO_WritePin(RFID_CS_GPIO_Port, RFID_CS_Pin, GPIO_PIN_SET);

        demo_printf("[0x%02X] tx=%u rx=%u -> 0x%02X\n",
                    regs[i], (unsigned int)s_tx, (unsigned int)s_rx, value);
    }
    demo_printf("Expect: 0x40, 0x4D, 0x62, 0x26, 0x92/91/90/88/B2\n");
    demo_printf("---- end of canary scan ----\n");
}

int main(void)
{
    MFRC522_Version_t version;

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART3_UART_Init();

    /* 1. Attach the STM32 SPI adapter to the handle. */
    if (MFRC522_STM32_SPI_Init(&rfid, &hspi1,
                               RFID_CS_GPIO_Port,  RFID_CS_Pin,
                               RFID_RST_GPIO_Port, RFID_RST_Pin,
                               RFID_IRQ_GPIO_Port, RFID_IRQ_Pin) != MFRC522_OK) {
        Error_Handler();
    }

    /* 2. Initialize the reader (reset + version check + antenna on). */
    if (MFRC522_Init(&rfid) != MFRC522_OK) {
        demo_printf("Reader init failed: %s\n",
                    MFRC522_StatusToString(MFRC522_GetLastError(&rfid)));
        demo_spi_raw_probe();
        demo_spi_raw_scan();
        demo_hw_diag();
        Error_Handler();
    }

    /* 3. Report the firmware version. */
    if (MFRC522_GetVersion(&rfid, &version) == MFRC522_OK) {
        demo_printf("MFRC522 firmware: %u.%u (raw 0x%02X)\n",
                    version.major, version.minor, version.raw);
    }

    /* 4. Run the digital self-test. */
    if (MFRC522_SelfTest(&rfid) == MFRC522_OK) {
        demo_printf("Self-test: PASS\n");
    } else {
        demo_printf("Self-test: FAIL\n");
    }

    demo_printf("Antenna: %s\n", MFRC522_IsAntennaOn(&rfid) ? "ON" : "OFF");

    while (1) {
        /* Idle. */
    }
}
