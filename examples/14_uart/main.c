/**
 * @file    main.c
 * @brief   Example 14 — UART interface.
 *
 * The MFRC522 serial UART is logic-level, 8N1, LSB-first (the transport
 * handles the bit reversal). Configure the MCU UART to 115200 8N1 and pass
 * MFRC522_UART_BAUD_115200 to the adapter.
 *
 * IMPORTANT: read docs/uart.md before relying on this interface — it is the
 * least common of the three and has framing/flow-control caveats.
 *
 * Wiring: see docs/stm32h743.md (UART table).
 */

#include "mfrc522_demo.h"

MFRC522_Handle_t rfid = {0};

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART3_UART_Init();
    /* MX_USART1_UART_Init();  <- the UART connected to the MFRC522 */

    /* Attach the UART adapter (115200). */
    if (MFRC522_STM32_UART_Init(&rfid, &huart1, MFRC522_UART_BAUD_115200,
                                RFID_RST_GPIO_Port, RFID_RST_Pin,
                                RFID_IRQ_GPIO_Port, RFID_IRQ_Pin) != MFRC522_OK) {
        Error_Handler();
    }

    if (MFRC522_Init(&rfid) != MFRC522_OK) {
        demo_printf("UART init failed\n");
        Error_Handler();
    }

    demo_printf("UART reader ready.\n");

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
