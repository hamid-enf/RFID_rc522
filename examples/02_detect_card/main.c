/**
 * @file    main.c
 * @brief   Example 02 — card detection.
 *
 * Polls MFRC522_IsCardPresent() (a short, bounded check) and also shows
 * MFRC522_WaitForCard() (blocking until a card appears or the timeout
 * expires). When no card is seen, a manual Transmit+Receive probe runs.
 *
 * Field-verified facts on the 0xB2 clone (see docs/troubleshooting.md):
 *   - The command state machine works: 0x04 (Transmit) and 0x0E
 *     (MFAuthent) consume the FIFO; 0x08 (Receive) engages.
 *   - 0x0C (Transceive) is accepted but NEVER starts: the FIFO byte is
 *     not consumed and no Timer/Rx/Idle IRQ fires. ComIEn (0x00/0x77/
 *     0xF7), timer configurations (driver / old-library / fast) and the
 *     antenna state all make no difference.
 *
 * So this probe emulates Transceive with the two commands that DO work:
 * Transmit the REQA (0x04), then — after a short switch delay that
 * covers the card's ATQA response window (~100..500 us) — switch to
 * Receive (0x08) and watch the FIFO. Several switch delays are tried so
 * the ATQA window is covered regardless of exact timing:
 *   - FIFO=2 [0x04][0x00] at some delay: the RF path is alive and the
 *     card is a 14443-A card -> the driver gets a Transmit+Receive
 *     transceive path for this silicon.
 *   - no FIFO data at any delay: nothing radiates back -> RF front end
 *     (antenna/oscillator/power) or the card is not a 14443-A card.
 */

#include "mfrc522_demo.h"

MFRC522_Handle_t rfid = {0};

/**
 * @brief Microsecond busy-wait using the DWT cycle counter (calibrated
 *        with the current HCLK, so it works at any core clock).
 */
static void demo_delay_us(uint32_t us)
{
    static uint8_t dwt_on = 0u;
    uint32_t target;

    if (!dwt_on) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0u;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
        dwt_on = 1u;
    }
    target = us * (HAL_RCC_GetHCLKFreq() / 1000000u);
    while (DWT->CYCCNT < target) {
        /* wait */
    }
}

/**
 * @brief One manual transceive: Transmit REQA, switch to Receive after
 *        `switch_us`, wait for RxIRq, report the FIFO content.
 */
static uint8_t demo_txrx_round(uint32_t switch_us)
{
    uint8_t irq = 0u, level = 0u, val = 0u, i, answered = 0u;
    uint32_t t0;

    MFRC522_WriteRegister(&rfid, 0x01u, 0x00u);      /* Idle           */
    MFRC522_WriteRegister(&rfid, 0x04u, 0x7Fu);      /* clear ComIrq   */
    MFRC522_WriteRegister(&rfid, 0x0Au, 0x80u);      /* flush FIFO     */
    MFRC522_WriteRegister(&rfid, 0x09u, 0x26u);      /* FIFO <- REQA   */
    MFRC522_WriteRegister(&rfid, 0x0Cu, 0x07u);      /* TxLastBits = 7 */

    /* Transmit the REQA to the antenna. */
    MFRC522_WriteRegister(&rfid, 0x01u, 0x04u);      /* Transmit      */
    MFRC522_SetBits(&rfid, 0x0Cu, 0x80u);            /* StartSend      */

    /* Let the REQA frame finish on air (7 Miller bits @106 kBd ~ 70 us),
     * then open the receiver. */
    if (switch_us != 0u) {
        demo_delay_us(switch_us);
    }
    MFRC522_WriteRegister(&rfid, 0x01u, 0x08u);      /* Receive        */

    /* Wait up to 30 ms for a receive IRQ. */
    t0 = HAL_GetTick();
    while ((uint32_t)(HAL_GetTick() - t0) < 30u) {
        MFRC522_ReadRegister(&rfid, 0x04u, &irq);
        if ((irq & 0x20u) != 0u) {
            break;                                   /* RxIRq           */
        }
    }

    MFRC522_ReadRegister(&rfid, 0x0Au, &level);
    level = (uint8_t)(level & 0x7Fu);
    demo_printf("switch@%5u us: IRQ=0x%02X FIFO=%u", (unsigned int)switch_us,
                (unsigned int)(irq & 0x7Fu), (unsigned int)level);
    for (i = 0u; i < level && i < 8u; i++) {
        MFRC522_ReadRegister(&rfid, 0x09u, &val);
        demo_printf(" [0x%02X]", val);
    }
    demo_printf("\n");

    /* ATQA of a type-A card is 04 00 (or 02/44 for UL/DESFire). */
    if (level >= 2u) {
        answered = 1u;
    }

    MFRC522_WriteRegister(&rfid, 0x01u, 0x00u);      /* Idle           */
    MFRC522_WriteRegister(&rfid, 0x04u, 0x7Fu);      /* clear ComIrq   */
    MFRC522_WriteRegister(&rfid, 0x0Au, 0x80u);      /* flush FIFO     */
    return answered;
}

/**
 * @brief Run the manual Transmit+Receive probe across the ATQA window.
 */
static void demo_txrx_probe(void)
{
    static const uint32_t switch_delays[] =
        { 0u, 100u, 250u, 500u, 1000u, 3000u };
    uint32_t r, d;
    uint8_t answered = 0u;

    demo_printf("\n=== tx+rx probe (manual transceive): card on antenna ===\n");

    for (r = 0u; (r < 3u) && (!answered); r++) {
        for (d = 0u;
             d < (uint32_t)(sizeof(switch_delays) / sizeof(switch_delays[0]));
             d++) {
            if (demo_txrx_round(switch_delays[d])) {
                answered = 1u;
                break;
            }
        }
    }

    if (answered) {
        demo_printf(">> A card ANSWERED through the RF path! <<\n");
    } else {
        demo_printf(">> No RF response at any switch delay. <<\n");
        demo_printf(">> RF front end or card frequency: check next. <<\n");
    }
    demo_printf("FIFO=2 [0x04][0x00] = a 14443-A card answered (ATQA).\n");
    demo_printf("No data at all = nothing radiates back (antenna/power/card).\n");
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

    /* No card by the driver? Probe the RF path manually. */
    if (any_found == 0u) {
        demo_txrx_probe();
    }

    while (1) {
    }
}
