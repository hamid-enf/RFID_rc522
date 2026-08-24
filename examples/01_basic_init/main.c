/**
 * @file    main.c
 * @brief   Example 01 — basic initialization.
 *
 * Attaches the SPI adapter, initializes the reader, reads the firmware
 * version, runs the self-test and confirms the antenna is on.
 *
 * Wiring: see docs/stm32h743.md (SPI table).
 */

#include "mfrc522_demo.h"

MFRC522_Handle_t rfid = {0};

int main(void)
{
    MFRC522_Version_t version;

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART3_UART_Init();

    /* 1. Attach the STM32 SPI adapter to the handle. */
    if (MFRC522_STM32_SPI_Init(&rfid, &hspi1,
                               RFID_CS_GPIO_Port,  RFID_CS_Pin,
                               RFID_RST_GPIO_Port, RFID_RST_Pin,
                               RFID_IRQ_GPIO_Port, RFID_IRQ_Pin) != MFRC522_OK) {
        Error_Handler();
    }

    /* 2. Initialize the reader (reset + version check + antenna on). */
    if (MFRC522_Init(&rfid) != MFRC522_OK) {
        demo_printf("Reader init failed: %s\n",
                    MFRC522_StatusToString(MFRC522_GetLastError(&rfid)));
        Error_Handler();
    }

    /* 3. Report the firmware version. */
    if (MFRC522_GetVersion(&rfid, &version) == MFRC522_OK) {
        demo_printf("MFRC522 firmware: %u.%u (raw 0x%02X)\n",
                    version.major, version.minor, version.raw);
    }

    /* 4. Run the digital self-test. */
    if (MFRC522_SelfTest(&rfid) == MFRC522_OK) {
        demo_printf("Self-test: PASS\n");
    } else {
        demo_printf("Self-test: FAIL\n");
    }

    demo_printf("Antenna: %s\n", MFRC522_IsAntennaOn(&rfid) ? "ON" : "OFF");

    while (1) {
        /* Idle. */
    }
}
