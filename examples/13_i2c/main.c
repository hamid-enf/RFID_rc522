/**
 * @file    main.c
 * @brief   Example 13 — I2C interface.
 *
 * The MFRC522 must be strapped to I2C mode (pin I2C = HIGH). The default
 * 7-bit address is 0x28 (EA low, all ADR pins low). Register reads use
 * HAL_I2C_Mem_Read (repeated start) inside the adapter.
 *
 * Wiring: see docs/stm32h743.md (I2C table) and docs/i2c.md.
 */

#include "mfrc522_demo.h"

MFRC522_Handle_t rfid = {0};

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART3_UART_Init();

    /* Attach the I2C adapter (default address 0x28). */
    if (MFRC522_STM32_I2C_Init(&rfid, &hi2c1, MFRC522_I2C_DEFAULT_ADDR,
                               RFID_RST_GPIO_Port, RFID_RST_Pin,
                               RFID_IRQ_GPIO_Port, RFID_IRQ_Pin) != MFRC522_OK) {
        Error_Handler();
    }

    if (MFRC522_Init(&rfid) != MFRC522_OK) {
        demo_printf("I2C init failed (check address/pull-ups)\n");
        Error_Handler();
    }

    demo_printf("I2C reader ready (addr 0x%02X).\n", MFRC522_I2C_DEFAULT_ADDR);

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
