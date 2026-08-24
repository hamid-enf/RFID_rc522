/**
 * @file    main.c
 * @brief   Example 15 — STM32H743 full demo (monitoring).
 *
 * A complete, observable demo: initializes the reader, prints its status,
 * waits for a card, and on detection reports ATQA/SAK/UID/type, authenticates
 * a MIFARE Classic sector and reads/writes a block. Output mirrors the
 * "MFRC522 RFID DEMO" console layout.
 *
 * Wiring: see docs/stm32h743.md.
 */

#include "mfrc522_demo.h"

MFRC522_Handle_t rfid = {0};

static void print_uid(const MFRC522_UID_t *uid)
{
    uint8_t i;
    for (i = 0u; i < uid->length; i++) {
        demo_printf("%02X ", uid->bytes[i]);
    }
}

int main(void)
{
    MFRC522_Version_t version;
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

    demo_printf("--------------------------------\n");
    demo_printf("MFRC522 RFID DEMO\n");
    demo_printf("--------------------------------\n");

    demo_printf("Reader       : MFRC522\n");
    MFRC522_GetVersion(&rfid, &version);
    demo_printf("Version      : %u.%u\n", version.major, version.minor);
    demo_printf("Interface    : SPI\n");
    demo_printf("Antenna      : %s\n", MFRC522_IsAntennaOn(&rfid) ? "ON" : "OFF");
    demo_printf("Self Test    : %s\n",
                (MFRC522_SelfTest(&rfid) == MFRC522_OK) ? "PASS" : "FAIL");

    while (1) {
        demo_printf("\nWaiting for card...\n");

        if (MFRC522_WaitForCard(&rfid, 0u) != MFRC522_OK) {
            continue;   /* 0 => default timeout, then retry */
        }

        demo_printf("\nCard detected!\n\n");

        s = MFRC522_ReadUID(&rfid, &uid);
        if (s != MFRC522_OK) {
            demo_printf("UID read failed: %s\n", MFRC522_StatusToString(s));
            HAL_Delay(500u);
            continue;
        }

        {
            MFRC522_CardInfo_t info;
            if (MFRC522_GetCardInfo(&rfid, &info) == MFRC522_OK) {
                demo_printf("ATQA         : %02X %02X\n", info.atqa[0], info.atqa[1]);
                demo_printf("SAK          : %02X\n", info.sak);
                demo_printf("UID          : ");
                print_uid(&uid);
                demo_printf("\nUID Length   : %u\n", uid.length);
                demo_printf("Card Type    : %s\n", MFRC522_CardTypeToString(info.type));
            }
        }

        /* MIFARE Classic 1K: authenticate sector 1 and read/write block 4. */
        s = MFRC522_AuthKeyA(&rfid, &DEMO_KEY, 4u, uid.bytes, uid.length);
        demo_printf("\nAuthentication : %s\n", MFRC522_StatusToString(s));

        if (s == MFRC522_OK) {
            s = MFRC522_ReadBlock(&rfid, 4u, data);
            if (s == MFRC522_OK) {
                demo_printf("\nBlock 4:\n");
                for (i = 0u; i < 16u; i++) {
                    demo_printf("%02X ", data[i]);
                }
                demo_printf("\n");
            }

            /* Write a demo pattern and read it back. */
            for (i = 0u; i < 16u; i++) {
                data[i] = (uint8_t)(0x11u * (i + 1u));
            }
            s = MFRC522_WriteBlock(&rfid, 4u, data);
            demo_printf("\nWrite         : %s\n", MFRC522_StatusToString(s));

            s = MFRC522_ReadBlock(&rfid, 4u, data);
            demo_printf("Read Back     : %s\n", MFRC522_StatusToString(s));

            MFRC522_StopCrypto1(&rfid);
        }

        MFRC522_HaltTag(&rfid);
        HAL_Delay(500u);
    }
}
