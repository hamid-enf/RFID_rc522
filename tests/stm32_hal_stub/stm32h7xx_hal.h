/**
 * @file    stm32h7xx_hal.h
 * @brief   Minimal STM32 HAL stub — TEST-ONLY.
 *
 * Provides just enough of the STM32 HAL / CMSIS surface for the platform
 * adapter (platform/stm32 sources) to be syntax-checked and compile-checked
 * on a host without the real STM32Cube HAL. It is NOT used on target
 * hardware and is NOT part of the library.
 *
 * Compile-check the adapter with:
 *   cc -std=c99 -Wall -Wextra -Wpedantic -DSTM32H743xx \
 *      -Iinclude -Iplatform/stm32 -Itests/stm32_hal_stub \
 *      -c platform/stm32/mfrc522_stm32_spi.c
 */

#ifndef STM32H7XX_HAL_STUB_H
#define STM32H7XX_HAL_STUB_H

#include <stdint.h>

/* ---- HAL status / pin states ------------------------------------- */
typedef enum {
    HAL_OK = 0x00u,
    HAL_ERROR = 0x01u,
    HAL_BUSY = 0x02u,
    HAL_TIMEOUT = 0x03u
} HAL_StatusTypeDef;

typedef enum {
    GPIO_PIN_RESET = 0u,
    GPIO_PIN_SET
} GPIO_PinState;

/* ---- Opaque peripheral handle types ------------------------------ */
typedef struct { uint8_t _; } SPI_HandleTypeDef;
typedef struct { uint8_t _; } I2C_HandleTypeDef;
typedef struct { uint8_t _; } UART_HandleTypeDef;
typedef struct { uint8_t _; } GPIO_TypeDef;

/* ---- I2C memory-address size selector ---------------------------- */
#define I2C_MEMADD_SIZE_8BIT (0x01u)

/* ---- Minimal DWT / CoreDebug (CMSIS core_cm7.h surface) ---------- */
typedef struct { volatile uint32_t CTRL; volatile uint32_t CYCCNT; } DWT_Type;
typedef struct { volatile uint32_t DEMCR; } CoreDebug_Type;

/* On real CMSIS these are pointer casts to core peripheral addresses; the
 * stub uses writable instances so host tests can execute the adapter. */
extern DWT_Type mfrc522_stub_dwt;
extern CoreDebug_Type mfrc522_stub_coredebug;

#define DWT        (&mfrc522_stub_dwt)
#define CoreDebug  (&mfrc522_stub_coredebug)

#define DWT_CTRL_CYCCNTENA_Msk     (0x00000001UL)
#define CoreDebug_DEMCR_TRCENA_Msk (0x01000000UL)

/* ---- HAL functions used by the adapter --------------------------- */
HAL_StatusTypeDef HAL_Init(void);
HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *h, uint8_t *pData,
                                   uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *h, uint8_t *pData,
                                  uint16_t Size, uint32_t Timeout);

HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *h, uint16_t DevAddress,
                                          uint8_t *pData, uint16_t Size,
                                          uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *h, uint16_t DevAddress,
                                   uint16_t MemAddress, uint16_t MemAddSize,
                                   uint8_t *pData, uint16_t Size, uint32_t Timeout);

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *h, uint8_t *pData,
                                    uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *h, uint8_t *pData,
                                   uint16_t Size, uint32_t Timeout);

void      HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState s);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);

void     HAL_Delay(uint32_t ms);
uint32_t HAL_GetTick(void);
uint32_t HAL_RCC_GetHCLKFreq(void);

#endif /* STM32H7XX_HAL_STUB_H */
