/**
 * @file    main.c
 * @brief   Example 05 — MIFARE Classic authentication.
 *
 * Selects a card and authenticates sector 0 (block 0) with the factory
 * default key using both Key A and Key B, then drops the Crypto1 state.
 */

#include "mfrc522_demo.h"

MFRC522_Handle_t rfid = {0};

int main(void)
{
    MFRC522_UID_t uid;
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

            s = MFRC522_AuthKeyA(&rfid, &DEMO_KEY, 0u, uid.bytes, uid.length);
            demo_printf("Auth Key A (block 0): %s\n", MFRC522_StatusToString(s));
            MFRC522_StopCrypto1(&rfid);

            s = MFRC522_AuthKeyB(&rfid, &DEMO_KEY, 0u, uid.bytes, uid.length);
            demo_printf("Auth Key B (block 0): %s\n", MFRC522_StatusToString(s));
            MFRC522_StopCrypto1(&rfid);

            MFRC522_HaltTag(&rfid);
            HAL_Delay(500u);
        }
    }
}
