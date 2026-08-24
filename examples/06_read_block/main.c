/**
 * @file    main.c
 * @brief   Example 06 — read a 16-byte MIFARE Classic block.
 *
 * Authenticates sector 1 (block 4..7) with Key A, reads block 4 and prints
 * the 16 data bytes, then halts the card.
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

    demo_printf("Waiting for card...\n");

    while (1) {
        if (MFRC522_IsCardPresent(&rfid) == MFRC522_OK) {
            if (MFRC522_ReadUID(&rfid, &uid) != MFRC522_OK) {
                HAL_Delay(500u);
                continue;
            }

            s = MFRC522_AuthKeyA(&rfid, &DEMO_KEY, 4u, uid.bytes, uid.length);
            if (s != MFRC522_OK) {
                demo_printf("Auth failed: %s\n", MFRC522_StatusToString(s));
            } else {
                s = MFRC522_ReadBlock(&rfid, 4u, data);
                if (s == MFRC522_OK) {
                    demo_printf("Block 4:\n");
                    for (i = 0u; i < 16u; i++) {
                        demo_printf("%02X ", data[i]);
                    }
                    demo_printf("\n");
                } else {
                    demo_printf("Read failed: %s\n", MFRC522_StatusToString(s));
                }
                MFRC522_StopCrypto1(&rfid);
            }

            MFRC522_HaltTag(&rfid);
            HAL_Delay(500u);
        }
    }
}
