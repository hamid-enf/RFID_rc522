/**
 * @file    main.c
 * @brief   Example 10 — interrupt (IRQ) usage.
 *
 * Registers an IRQ callback and services reader interrupts from the main
 * loop (polling style). In a real EXTI setup, call MFRC522_ProcessIRQ() from
 * the GPIO EXTI interrupt handler instead of from the loop.
 */

#include "mfrc522_demo.h"

MFRC522_Handle_t rfid = {0};

/* IRQ callback: reports which interrupt sources fired. */
static void irq_handler(MFRC522_Handle_t *handle, uint8_t irq_source, void *user)
{
    (void)handle;
    (void)user;

    demo_printf("IRQ 0x%02X:", irq_source);
    if (irq_source & MFRC522_IRQ_TX)       demo_printf(" TX");
    if (irq_source & MFRC522_IRQ_RX)       demo_printf(" RX");
    if (irq_source & MFRC522_IRQ_IDLE)     demo_printf(" IDLE");
    if (irq_source & MFRC522_IRQ_ERR)      demo_printf(" ERR");
    if (irq_source & MFRC522_IRQ_TIMER)    demo_printf(" TIMER");
    if (irq_source & MFRC522_IRQ_HI_ALERT) demo_printf(" HI");
    if (irq_source & MFRC522_IRQ_LO_ALERT) demo_printf(" LO");
    demo_printf("\n");
}

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

    /* Register the callback. */
    if (MFRC522_AttachIRQCallback(&rfid, irq_handler, NULL) != MFRC522_OK) {
        Error_Handler();
    }

    demo_printf("IRQ demo running (card I/O will trigger interrupts)...\n");

    while (1) {
        /* Poll pending reader interrupts. In an EXTI setup, replace this
         * with a call to MFRC522_ProcessIRQ() inside the EXTI handler. */
        MFRC522_ProcessIRQ(&rfid);

        /* Generate some traffic so IRQs fire. */
        MFRC522_IsCardPresent(&rfid);
        HAL_Delay(100u);
    }
}
