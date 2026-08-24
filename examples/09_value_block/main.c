/**
 * @file    main.c
 * @brief   Example 09 — MIFARE Classic value blocks.
 *
 * Builds a value block with MFRC522_FormatValueBlock(), writes it, then
 * exercises the value protocol: Increment -> Transfer -> Decrement ->
 * Transfer. The sector must already be authenticated.
 *
 * NOTE: value operations only work on blocks whose access bits mark them as
 * value blocks ([C1 C2 C3] = 110b or 001b). With the factory default key the
 * access bits are 000b (transport configuration), so this example is
 * illustrative; format the sector trailer first on a real card.
 */

#include "mfrc522_demo.h"

MFRC522_Handle_t rfid = {0};

int main(void)
{
    MFRC522_UID_t uid;
    uint8_t block[16];
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

            if (MFRC522_AuthKeyA(&rfid, &DEMO_KEY, 4u, uid.bytes, uid.length) == MFRC522_OK) {
                /* Format a value block worth 1000, address 0x05. */
                MFRC522_FormatValueBlock(block, 1000, 0x05u);
                s = MFRC522_WriteBlock(&rfid, 4u, block);
                demo_printf("Write value block: %s\n", MFRC522_StatusToString(s));

                s = MFRC522_Increment(&rfid, 4u, 100);
                demo_printf("Increment 100:    %s\n", MFRC522_StatusToString(s));
                s = MFRC522_Transfer(&rfid, 4u);
                demo_printf("Transfer:         %s\n", MFRC522_StatusToString(s));

                s = MFRC522_Decrement(&rfid, 4u, 50);
                demo_printf("Decrement 50:     %s\n", MFRC522_StatusToString(s));
                s = MFRC522_Transfer(&rfid, 4u);
                demo_printf("Transfer:         %s\n", MFRC522_StatusToString(s));

                MFRC522_StopCrypto1(&rfid);
            }

            MFRC522_HaltTag(&rfid);
            HAL_Delay(500u);
        }
    }
}
