/**
 * @file    main.c
 * @brief   Example 11 — low-power modes.
 *
 * Demonstrates MFRC522_Sleep() (soft power-down), MFRC522_WakeUp() and
 * MFRC522_PowerDown() (NRSTPD low). Power consumption figures are documented
 * in docs/architecture.md ("Low power").
 */

#include "mfrc522_demo.h"

MFRC522_Handle_t rfid = {0};

int main(void)
{
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

    demo_printf("Low-power demo\n");

    /* Soft power-down (keeps register contents, antenna off). */
    demo_printf("Sleep...\n");
    MFRC522_Sleep(&rfid);
    HAL_Delay(2000u);

    /* Wake up (clears the PowerDown bit, waits for the oscillator). */
    demo_printf("Wake up...\n");
    MFRC522_WakeUp(&rfid);

    /* Full power-down via NRSTPD (lowest consumption). */
    demo_printf("Power down (NRSTPD low)...\n");
    MFRC522_PowerDown(&rfid);
    HAL_Delay(2000u);

    /* After a full power-down the reader must be re-initialized. */
    demo_printf("Re-init...\n");
    if (MFRC522_HardReset(&rfid) != MFRC522_OK ||
        MFRC522_Init(&rfid) != MFRC522_OK) {
        demo_printf("Re-init failed\n");
        Error_Handler();
    }

    demo_printf("Back online.\n");

    while (1) {
    }
}
