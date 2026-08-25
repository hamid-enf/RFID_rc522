/**
 * @file    main.c
 * @brief   Example 02 — card detection.
 *
 * Polls MFRC522_IsCardPresent() (a short, bounded check) and also shows
 * MFRC522_WaitForCard() (blocking until a card appears or the timeout
 * expires). When no card is seen, a focused probe hunts the Transceive
 * stall on 0xB2 clone silicon.
 *
 * Field-verified facts (0xB2 clone, see docs/troubleshooting.md):
 *   - The command table is NXP-correct: 0x04 (Transmit) and 0x0E
 *     (MFAuthent) consume the FIFO and complete; 0x08 (Receive) engages.
 *   - 0x0C (Transceive) is accepted (CommandReg holds 0x0C) but never
 *     starts: the FIFO byte is not consumed, no Timer/Rx/Idle IRQ fires.
 *   - ComIrq bit 0x04 (HiAlert) is an always-on level indicator on this
 *     clone, not a clearable event.
 *
 * The remaining differences to the older (working) library are the
 * ComIEnReg value (the old library wrote 0x77|0x80 before every
 * transceive; this driver never touches ComIEn) and the timer
 * configuration. This probe tests both:
 *   - 0x0C with ComIEn = 0x00 / 0x77 / 0xF7
 *   - 0x0C with the old library's timer (0x8D/0x3E/0x001E) and a fast
 *     timer (0x80/0x00/0x0001)
 *   - 0x0C with the RF drivers explicitly on
 *
 * A config that consumes the FIFO byte (FIFO != 1) has fixed the stall;
 * FIFO = 2 with [0x04][0x00] means a card answered (ATQA).
 */

#include "mfrc522_demo.h"

MFRC522_Handle_t rfid = {0};

/**
 * @brief One 0x0C (Transceive) REQA probe with a given ComIEn value and
 *        timer configuration. Reports the IRQ bits and whether the FIFO
 *        byte was consumed (and, if not, its content).
 */
static void demo_probe_0c(uint8_t ien, uint8_t tmode, uint8_t tpresc,
                          uint8_t trh, uint8_t trl, const char *label)
{
    uint8_t irq = 0u, level = 0u, val = 0u, i;
    uint32_t t0;

    demo_printf("-- 0x0C: IEn=0x%02X TMode=0x%02X TPresc=0x%02X TRel=0x%02X%02X (%s)\n",
                (unsigned int)ien, (unsigned int)tmode, (unsigned int)tpresc,
                (unsigned int)trh, (unsigned int)trl, label);

    MFRC522_WriteRegister(&rfid, 0x01u, 0x00u);      /* Idle           */
    MFRC522_WriteRegister(&rfid, 0x02u, ien);        /* ComIEn         */
    MFRC522_WriteRegister(&rfid, 0x04u, 0x7Fu);      /* clear ComIrq   */
    MFRC522_WriteRegister(&rfid, 0x0Au, 0x80u);      /* flush FIFO     */
    MFRC522_WriteRegister(&rfid, 0x2Au, tmode);      /* TMode          */
    MFRC522_WriteRegister(&rfid, 0x2Bu, tpresc);     /* TPrescaler     */
    MFRC522_WriteRegister(&rfid, 0x2Cu, trh);        /* TReload high   */
    MFRC522_WriteRegister(&rfid, 0x2Du, trl);        /* TReload low    */
    MFRC522_WriteRegister(&rfid, 0x09u, 0x26u);      /* FIFO <- REQA   */
    MFRC522_WriteRegister(&rfid, 0x0Cu, 0x07u);      /* TxLastBits = 7 */
    MFRC522_WriteRegister(&rfid, 0x01u, 0x0Cu);      /* Transceive     */
    MFRC522_SetBits(&rfid, 0x0Cu, 0x80u);            /* StartSend      */

    t0 = HAL_GetTick();
    while ((uint32_t)(HAL_GetTick() - t0) < 60u) {
        MFRC522_ReadRegister(&rfid, 0x04u, &irq);
        if ((irq & 0x31u) != 0u) {
            break;                                   /* Rx|Idle|Timer   */
        }
    }

    MFRC522_ReadRegister(&rfid, 0x0Au, &level);
    level = (uint8_t)(level & 0x7Fu);
    demo_printf("   IRQ=0x%02X FIFO=%u %s", (unsigned int)(irq & 0x7Fu),
                (unsigned int)level, (level == 1u) ? "(NOT consumed)" : "");
    for (i = 0u; i < level && i < 8u; i++) {
        MFRC522_ReadRegister(&rfid, 0x09u, &val);
        demo_printf(" [0x%02X]", val);
    }
    demo_printf("\n");

    MFRC522_WriteRegister(&rfid, 0x01u, 0x00u);      /* Idle           */
    MFRC522_WriteRegister(&rfid, 0x04u, 0x7Fu);      /* clear ComIrq   */
}

/**
 * @brief Hunt the Transceive stall: ComIEn and timer variations.
 */
static void demo_clone_probe(void)
{
    demo_printf("\n=== clone transceive probe: hold the card on the antenna ===\n");

    /* Group 1: ComIEn variations (driver timer). The old working library
     * wrote 0x77|0x80 = 0xF7 before every transceive. */
    demo_probe_0c(0x00u, 0x80u, 0xA9u, 0x03u, 0xE8u, "driver cfg, IEn=0x00");
    demo_probe_0c(0x77u, 0x80u, 0xA9u, 0x03u, 0xE8u, "driver cfg, IEn=0x77");
    demo_probe_0c(0xF7u, 0x80u, 0xA9u, 0x03u, 0xE8u, "driver cfg, IEn=0xF7");

    /* Group 2: timer variations (with IEn=0x77). */
    demo_probe_0c(0x77u, 0x8Du, 0x3Eu, 0x00u, 0x1Eu, "old-lib timer");
    demo_probe_0c(0x77u, 0x80u, 0x00u, 0x00u, 0x01u, "fast timer");

    /* Group 3: RF drivers explicitly on (reset state may have them off). */
    demo_printf("-- with antenna on --\n");
    MFRC522_SetBits(&rfid, 0x14u, 0x03u);
    demo_probe_0c(0x77u, 0x80u, 0xA9u, 0x03u, 0xE8u, "IEn=0x77, antenna on");
    demo_probe_0c(0x77u, 0x8Du, 0x3Eu, 0x00u, 0x1Eu, "old-lib timer, antenna on");

    demo_printf("FIFO=1 (NOT consumed) = the command never started.\n");
    demo_printf("FIFO=0                = the command ran, no card answer.\n");
    demo_printf("FIFO=2 [0x04][0x00]   = a card ANSWERED (ATQA)!\n");
}

int main(void)
{
    uint8_t any_found = 0u;

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART3_UART_Init();

    if (MFRC522_STM32_SPI_Init(&rfid, &hspi1,
                               RFID_CS_GPIO_Port,  RFID_CS_Pin,
                               RFID_RST_GPIO_Port, RFID_RST_Pin,
                               RFID_IRQ_GPIO_Port, RFID_IRQ_Pin) != MFRC522_OK) {
        Error_Handler();
    }
    if (MFRC522_Init(&rfid) != MFRC522_OK) {
        Error_Handler();
    }

    demo_printf("Polling for cards (5 s)...\n");

    /* Option A: non-busy polling. */
    {
        uint32_t deadline = HAL_GetTick() + 5000u;
        uint8_t found = 0u;

        while (HAL_GetTick() < deadline) {
            if (MFRC522_IsCardPresent(&rfid) == MFRC522_OK) {
                demo_printf("Card detected (polling)!\n");
                found = 1u;
                break;
            }
        }
        if (!found) {
            demo_printf("No card seen while polling.\n");
        }
        any_found = found;
    }

    /* Option B: blocking wait with timeout. */
    demo_printf("Now blocking-waiting for a card (5 s)...\n");
    if (MFRC522_WaitForCard(&rfid, 5000u) == MFRC522_OK) {
        demo_printf("Card detected (wait)!\n");
        any_found = 1u;
    } else {
        demo_printf("Wait timed out.\n");
    }

    /* No card by the driver? Probe the Transceive stall directly. */
    if (any_found == 0u) {
        demo_clone_probe();
    }

    while (1) {
    }
}
