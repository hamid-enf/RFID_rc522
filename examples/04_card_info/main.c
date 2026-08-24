/**
 * @file    main.c
 * @brief   Example 04 — full card information.
 *
 * Uses MFRC522_GetCardInfo() to gather ATQA, SAK, UID and the derived card
 * type in a single call.
 */

#include "mfrc522_demo.h"

MFRC522_Handle_t rfid = {0};

int main(void)
{
    MFRC522_CardInfo_t info;
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
            if (MFRC522_GetCardInfo(&rfid, &info) == MFRC522_OK) {
                demo_printf("ATQA     : %02X %02X\n", info.atqa[0], info.atqa[1]);
                demo_printf("SAK      : %02X\n", info.sak);
                demo_printf("UID      : ");
                for (i = 0u; i < info.uid_length; i++) {
                    demo_printf("%02X ", info.uid[i]);
                }
                demo_printf("\nUID Len  : %u\n", info.uid_length);
                demo_printf("Type     : %s\n", MFRC522_CardTypeToString(info.type));
                MFRC522_HaltTag(&rfid);
            }
            HAL_Delay(500u);
        }
    }
}
