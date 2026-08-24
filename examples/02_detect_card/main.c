/**
 * @file    main.c
 * @brief   Example 02 — card detection.
 *
 * Polls MFRC522_IsCardPresent() (a short, bounded check) and also shows
 * MFRC522_WaitForCard() (blocking until a card appears or the timeout
 * expires).
 */

#include "mfrc522_demo.h"

MFRC522_Handle_t rfid = {0};

int main(void)
{
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
    }

    /* Option B: blocking wait with timeout. */
    demo_printf("Now blocking-waiting for a card (5 s)...\n");
    if (MFRC522_WaitForCard(&rfid, 5000u) == MFRC522_OK) {
        demo_printf("Card detected (wait)!\n");
    } else {
        demo_printf("Wait timed out.\n");
    }

    while (1) {
    }
}
