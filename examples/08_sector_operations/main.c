/**
 * @file    main.c
 * @brief   Example 08 — sector-level read/write.
 *
 * Demonstrates MFRC522_AuthenticateSector(), MFRC522_ReadSector() and
 * MFRC522_WriteSector(). Sector 1 (blocks 4..7) is 64 bytes: 3 data blocks
 * (48 bytes) + the trailer (block 7, untouched by WriteSector).
 */

#include "mfrc522_demo.h"

MFRC522_Handle_t rfid = {0};

int main(void)
{
    MFRC522_UID_t uid;
    uint8_t buffer[48];
    uint32_t len;
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

            /* Authenticate the whole sector via its first block. */
            s = MFRC522_AuthenticateSector(&rfid, 1u, MFRC522_KEY_A, &DEMO_KEY,
                                           uid.bytes, uid.length);
            demo_printf("Auth sector 1: %s\n", MFRC522_StatusToString(s));

            /* Read the three data blocks (48 bytes). */
            len = sizeof(buffer);
            s = MFRC522_ReadSector(&rfid, 1u, MFRC522_KEY_A, &DEMO_KEY,
                                   uid.bytes, uid.length, buffer, &len);
            if (s == MFRC522_OK) {
                demo_printf("Read sector 1 (%lu bytes):\n", (unsigned long)len);
                for (i = 0u; i < 16u; i++) {
                    demo_printf("%02X ", buffer[i]);
                }
                demo_printf("\n");
            } else {
                demo_printf("ReadSector failed: %s\n", MFRC522_StatusToString(s));
            }

            MFRC522_HaltTag(&rfid);
            HAL_Delay(500u);
        }
    }
}
