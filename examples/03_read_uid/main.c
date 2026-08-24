/**
 * @file    main.c
 * @brief   Example 03 — read a card UID.
 *
 * Waits for a card and prints its UID (4, 7 or 10 bytes) and SAK, then halts
 * the card.
 */

#include "mfrc522_demo.h"

MFRC522_Handle_t rfid = {0};

int main(void)
{
    MFRC522_UID_t uid;
    MFRC522_Status_t s;
    uint8_t i;

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
            s = MFRC522_ReadUID(&rfid, &uid);
            if (s == MFRC522_OK) {
                demo_printf("UID (%u bytes): ", uid.length);
                for (i = 0u; i < uid.length; i++) {
                    demo_printf("%02X ", uid.bytes[i]);
                }
                demo_printf(" SAK: 0x%02X\n", uid.sak);
                MFRC522_HaltTag(&rfid);
            } else {
                demo_printf("UID read failed: %s\n", MFRC522_StatusToString(s));
            }
            HAL_Delay(500u);   /* debounce: let the card leave the field */
        }
    }
}
