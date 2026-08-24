/**
 * @file    mfrc522_stm32_spi.h
 * @brief   STM32 HAL adapter for the SPI host interface.
 *
 * Wires a STM32 SPI peripheral + GPIO pins into the library's transport and
 * platform abstraction. After calling MFRC522_STM32_SPI_Init(), the handle
 * is ready for MFRC522_Init().
 *
 * Wiring (MFRC522 -> STM32H743):
 *     SCK  -> SPI1_SCK        (PB3)
 *     MOSI -> SPI1_MOSI       (PB5)
 *     MISO -> SPI1_MISO       (PB4)
 *     SDA/NSS -> GPIO (chip select, any GPIO)
 *     RST  -> GPIO (NRSTPD)
 *     IRQ  -> GPIO EXTI (optional)
 *
 * The adapter keeps all private state inside handle->platform_storage, so a
 * single MFRC522_Handle_t is sufficient — no globals, no heap.
 */

#ifndef MFRC522_STM32_SPI_H
#define MFRC522_STM32_SPI_H

#include "mfrc522_stm32.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Private SPI-adapter context (stored inside the handle).
 *
 * @warning Must fit in MFRC522_PLATFORM_CTX_SIZE; a compile-time check in the
 *          implementation enforces this.
 */
typedef struct MFRC522_STM32_SPI_Context
{
    SPI_HandleTypeDef *hspi;     /**< Configured SPI peripheral.              */
    GPIO_TypeDef      *cs_port;  /**< Chip-select port.                       */
    uint16_t           cs_pin;   /**< Chip-select pin.                        */
    GPIO_TypeDef      *rst_port; /**< Reset (NRSTPD) port.                    */
    uint16_t           rst_pin;  /**< Reset (NRSTPD) pin.                     */
    GPIO_TypeDef      *irq_port; /**< IRQ port (may be NULL if unused).       */
    uint16_t           irq_pin;  /**< IRQ pin (may be 0 if unused).           */
} MFRC522_STM32_SPI_Context_t;

/**
 * @brief Configure a handle to talk to the MFRC522 over SPI.
 *
 * @param handle    Zero-initialized reader handle.
 * @param hspi      Initialized SPI handle (Mode 0, MSB first, 8-bit).
 * @param cs_port   Chip-select GPIO port.
 * @param cs_pin    Chip-select GPIO pin.
 * @param rst_port  Reset GPIO port.
 * @param rst_pin   Reset GPIO pin.
 * @param irq_port  Optional IRQ GPIO port (NULL to disable IRQ).
 * @param irq_pin   Optional IRQ GPIO pin.
 *
 * @return MFRC522_OK on success.
 */
MFRC522_Status_t MFRC522_STM32_SPI_Init(MFRC522_Handle_t *handle,
                                        SPI_HandleTypeDef *hspi,
                                        GPIO_TypeDef *cs_port, uint16_t cs_pin,
                                        GPIO_TypeDef *rst_port, uint16_t rst_pin,
                                        GPIO_TypeDef *irq_port, uint16_t irq_pin);

/**
 * @brief Override the SPI clock-speed selector for this handle.
 *
 * @param speed  MFRC522_SPI_SPEED_LOW / _MED / _HIGH.
 */
void MFRC522_STM32_SPI_SetSpeed(MFRC522_Handle_t *handle, uint8_t speed);

#ifdef __cplusplus
}
#endif

#endif /* MFRC522_STM32_SPI_H */
