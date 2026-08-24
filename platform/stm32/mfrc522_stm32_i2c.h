/**
 * @file    mfrc522_stm32_i2c.h
 * @brief   STM32 HAL adapter for the I2C host interface.
 *
 * The MFRC522 must be strapped to I2C mode: pin I2C (pin 1) tied HIGH.
 * The 7-bit slave address defaults to 0x28 (all ADR pins low, EA low).
 * See docs/i2c.md for the address-pin table.
 */

#ifndef MFRC522_STM32_I2C_H
#define MFRC522_STM32_I2C_H

#include "mfrc522_stm32.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Private I2C-adapter context (stored inside the handle).
 */
typedef struct MFRC522_STM32_I2C_Context
{
    I2C_HandleTypeDef *hi2c;     /**< Configured I2C peripheral.              */
    GPIO_TypeDef      *rst_port; /**< Reset (NRSTPD) port.                    */
    uint16_t           rst_pin;  /**< Reset (NRSTPD) pin.                     */
    GPIO_TypeDef      *irq_port; /**< IRQ port (may be NULL).                 */
    uint16_t           irq_pin;  /**< IRQ pin (may be 0).                     */
} MFRC522_STM32_I2C_Context_t;

/**
 * @brief Configure a handle to talk to the MFRC522 over I2C.
 *
 * @param handle      Zero-initialized reader handle.
 * @param hi2c        Initialized I2C handle (Fast mode supported).
 * @param dev_addr    7-bit slave address (use MFRC522_I2C_DEFAULT_ADDR for
 *                    the default 0x28).
 * @param rst_port    Reset GPIO port.
 * @param rst_pin     Reset GPIO pin.
 * @param irq_port    Optional IRQ GPIO port (NULL to disable IRQ).
 * @param irq_pin     Optional IRQ GPIO pin.
 */
MFRC522_Status_t MFRC522_STM32_I2C_Init(MFRC522_Handle_t *handle,
                                        I2C_HandleTypeDef *hi2c,
                                        uint8_t dev_addr,
                                        GPIO_TypeDef *rst_port, uint16_t rst_pin,
                                        GPIO_TypeDef *irq_port, uint16_t irq_pin);

#ifdef __cplusplus
}
#endif

#endif /* MFRC522_STM32_I2C_H */
