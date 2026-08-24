/**
 * @file    mfrc522_stm32_uart.h
 * @brief   STM32 HAL adapter for the UART host interface.
 *
 * The MFRC522 serial UART interface is a **logic-level** UART (not RS-232):
 * 8 data bits LSB-first, no parity, 1 stop bit. The MFRC522 is selected into
 * UART mode by tying pin I2C (pin 1) LOW and pin EA (pin 32) LOW.
 *
 * @warning The UART host interface is the least commonly used of the three.
 *          Read docs/uart.md before relying on it — notably the DTRQ flow
 *          control line and the LSB-first address byte.
 */

#ifndef MFRC522_STM32_UART_H
#define MFRC522_STM32_UART_H

#include "mfrc522_stm32.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Private UART-adapter context (stored inside the handle).
 */
typedef struct MFRC522_STM32_UART_Context
{
    UART_HandleTypeDef *huart;   /**< Configured UART peripheral.             */
    GPIO_TypeDef       *rst_port;/**< Reset (NRSTPD) port.                    */
    uint16_t            rst_pin; /**< Reset (NRSTPD) pin.                     */
    GPIO_TypeDef       *irq_port;/**< IRQ port (may be NULL).                 */
    uint16_t            irq_pin; /**< IRQ pin (may be 0).                     */
} MFRC522_STM32_UART_Context_t;

/**
 * @brief Configure a handle to talk to the MFRC522 over UART.
 *
 * @param handle      Zero-initialized reader handle.
 * @param huart       Initialized UART handle (8N1). Configure the baud rate
 *                    to one of the MFRC522-supported rates (e.g. 115200).
 * @param baud        MFRC522_UartBaud_t index matching the UART baud rate.
 * @param rst_port    Reset GPIO port.
 * @param rst_pin     Reset GPIO pin.
 * @param irq_port    Optional IRQ GPIO port (NULL to disable IRQ).
 * @param irq_pin     Optional IRQ GPIO pin.
 */
MFRC522_Status_t MFRC522_STM32_UART_Init(MFRC522_Handle_t *handle,
                                         UART_HandleTypeDef *huart,
                                         uint8_t baud,
                                         GPIO_TypeDef *rst_port, uint16_t rst_pin,
                                         GPIO_TypeDef *irq_port, uint16_t irq_pin);

#ifdef __cplusplus
}
#endif

#endif /* MFRC522_STM32_UART_H */
