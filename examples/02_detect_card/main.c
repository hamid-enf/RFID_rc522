/**
 * @file    main.c
 * @brief   Example 02 — card detection.
 *
 * Polls MFRC522_IsCardPresent() (a short, bounded check) and also shows
 * MFRC522_WaitForCard() (blocking until a card appears or the timeout
 * expires). When no card is seen, a raw REQA monitor reports the
 * chip-level outcome (IRQ bits + FIFO content) to separate "the card
 * answered but the driver mishandled it" from "no card answered at all".
 */

#include "mfrc522_demo.h"

MFRC522_Handle_t rfid = {0};

/**
 * @brief One raw request through the chip: write the request byte (REQA or
 *        WUPA) into the FIFO, start the command, then report which ComIrq
 *        bits fire, how soon, and what lands in the FIFO.
 *
 * @param cmd_code  CommandReg value: 0x0C (datasheet Transceive) or 0x07
 *                  (the value this repo used before the fix — a no-op).
 * @param req_byte  0x26 (REQA: answers IDLE/READY cards) or
 *                  0x52 (WUPA: additionally wakes HALT'ed cards).
 */
static void demo_reqa_monitor_byte(uint8_t cmd_code, uint8_t req_byte)
{
    uint8_t irq = 0u, level = 0u, val = 0u, i;
    uint32_t t0, t_irq = 0u;

    demo_printf("\n[monitor] CommandReg=0x%02X, byte=0x%02X\n",
                cmd_code, req_byte);

    MFRC522_WriteRegister(&rfid, 0x01u, 0x00u);      /* Idle            */
    MFRC522_WriteRegister(&rfid, 0x04u, 0x7Fu);      /* clear ComIrq    */
    MFRC522_WriteRegister(&rfid, 0x0Au, 0x80u);      /* flush FIFO      */
    MFRC522_WriteRegister(&rfid, 0x09u, req_byte);   /* FIFO <- request */
    MFRC522_WriteRegister(&rfid, 0x0Cu, 0x07u);      /* TxLastBits = 7  */
    MFRC522_WriteRegister(&rfid, 0x01u, cmd_code);   /* start command   */
    MFRC522_SetBits(&rfid, 0x0Cu, 0x80u);            /* StartSend       */

    t0 = HAL_GetTick();
    while ((uint32_t)(HAL_GetTick() - t0) < 100u) {
        MFRC522_ReadRegister(&rfid, 0x04u, &irq);
        irq = (uint8_t)(irq & 0x7Fu);
        if (irq != 0u) {
            t_irq = (uint32_t)(HAL_GetTick() - t0);
            break;
        }
    }
    if (irq == 0u) {
        t_irq = 100u;   /* no IRQ within 100 ms */
    }

    MFRC522_WriteRegister(&rfid, 0x01u, 0x00u);      /* Idle            */
    MFRC522_WriteRegister(&rfid, 0x04u, 0x7Fu);      /* clear ComIrq    */

    MFRC522_ReadRegister(&rfid, 0x0Au, &level);
    level = (uint8_t)(level & 0x7Fu);
    demo_printf("[monitor] IRQ=0x%02X after %lu ms, FIFO=%u:",
                irq, (unsigned long)t_irq, level);
    for (i = 0u; i < level && i < 8u; i++) {
        MFRC522_ReadRegister(&rfid, 0x09u, &val);
        demo_printf(" 0x%02X", val);
    }
    demo_printf("\n");
}

/**
 * @brief Card not seen by the driver: probe the RF path directly.
 *
 * Interpretation:
 *   - IRQ 0x30 (Rx+Idle) within a few ms + FIFO "0x04 0x00": a card
 *     ANSWERED the request (ATQA of a MIFARE card). If the driver still
 *     reports "no card", the linked library is stale — rebuild.
 *   - IRQ 0x01 (Timer) only, no RX: the command ran but no card answered
 *     -> RF side: card not in the field, wrong frequency, antenna
 *     matching, module power.
 *   - IRQ 0x00 (nothing for 100 ms): the command never ran -> the value
 *     written to CommandReg is not Transceive on this silicon.
 */
static void demo_reqa_monitor(void)
{
    demo_printf("\n=== REQA monitor: hold the card on the antenna ===\n");
    demo_reqa_monitor_byte(0x0Cu, 0x26u);   /* Transceive + REQA   */
    demo_reqa_monitor_byte(0x0Cu, 0x52u);   /* Transceive + WUPA   */
    demo_reqa_monitor_byte(0x07u, 0x26u);   /* legacy 0x07 (no-op check) */
    demo_printf("IRQ 0x30 + FIFO 0x04 0x00 = card answered (ATQA).\n");
    demo_printf("IRQ 0x01 only              = command ran, no answer (RF).\n");
    demo_printf("IRQ 0x00                   = command never ran.\n");
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

    /* No card by the driver? Probe the RF path directly. */
    if (any_found == 0u) {
        demo_reqa_monitor();
    }

    while (1) {
    }
}
