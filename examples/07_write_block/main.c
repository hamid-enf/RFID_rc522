/**
 * @file    main.c
 * @brief   Example 07 — write a 16-byte MIFARE Classic block.
 *
 * Authenticates sector 1 (block 4..7) with Key A, writes a pattern to block 4
 * and reads it back to confirm. NOTE: writing to block 4 (sector 1) is safe
 * with the factory key; avoid the sector trailer and the manufacturer block.
 */

#include "mfrc522_demo.h"

MFRC522_Handle_t rfid = {0};

int main(void)
{
    MFRC522_UID_t uid;
    uint8_t data[16];
    uint8_t i;
    MFRC522_Status_t s;

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

    for (i = 0u; i < 16u; i++) {
        data[i] = (uint8_t)(0x11u * (i + 1u));   /* 0x11 0x22 0x33 ... */
    }

    demo_printf("Waiting for card...\n");

    while (1) {
        if (MFRC522_IsCardPresent(&rfid) == MFRC522_OK) {
            if (MFRC522_ReadUID(&rfid, &uid) != MFRC522_OK) {
                HAL_Delay(500u);
                continue;
            }

            s = MFRC522_AuthKeyA(&rfid, &DEMO_KEY, 4u, uid.bytes, uid.length);
            if (s == MFRC522_OK) {
                s = MFRC522_WriteBlock(&rfid, 4u, data);
                demo_printf("Write block 4: %s\n", MFRC522_StatusToString(s));
                MFRC522_StopCrypto1(&rfid);
            } else {
                demo_printf("Auth failed: %s\n", MFRC522_StatusToString(s));
            }

            MFRC522_HaltTag(&rfid);
            HAL_Delay(500u);
        }
    }
}
