/**
 * @file    main.c
 * @brief   Example 12 — SPI interface (recommended).
 *
 * Complete SPI example: configure SPI1 (mode 0, MSB first, 8-bit, software
 * NSS) and run the reader. Shows SPI speed selection via
 * MFRC522_STM32_SPI_SetSpeed().
 *
 * Wiring: see docs/stm32h743.md (SPI table).
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

    /* Attach the SPI adapter. */
    if (MFRC522_STM32_SPI_Init(&rfid, &hspi1,
                               RFID_CS_GPIO_Port,  RFID_CS_Pin,
                               RFID_RST_GPIO_Port, RFID_RST_Pin,
                               RFID_IRQ_GPIO_Port, RFID_IRQ_Pin) != MFRC522_OK) {
        Error_Handler();
    }

    /* Optional: pick the SPI clock preset (LOW / MED / HIGH). */
    MFRC522_STM32_SPI_SetSpeed(&rfid, MFRC522_SPI_SPEED_MED);

    if (MFRC522_Init(&rfid) != MFRC522_OK) {
        demo_printf("SPI init failed\n");
        Error_Handler();
    }

    demo_printf("SPI reader ready.\n");

    while (1) {
        if (MFRC522_IsCardPresent(&rfid) == MFRC522_OK) {
            MFRC522_UID_t uid;
            if (MFRC522_ReadUID(&rfid, &uid) == MFRC522_OK) {
                demo_printf("UID: %02X %02X %02X %02X\n",
                            uid.bytes[0], uid.bytes[1], uid.bytes[2], uid.bytes[3]);
                MFRC522_HaltTag(&rfid);
            }
            HAL_Delay(500u);
        }
    }
}
