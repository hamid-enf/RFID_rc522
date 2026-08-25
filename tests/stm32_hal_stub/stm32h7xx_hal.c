/**
 * @file    stm32h7xx_hal.c
 * @brief   STM32 HAL stub implementations — TEST-ONLY.
 *
 * Defines the DWT/CoreDebug instances and stub HAL functions so the adapter
 * can be linked into a host test. All functions are no-ops returning success.
 */

#include "stm32h7xx_hal.h"

DWT_Type mfrc522_stub_dwt;
CoreDebug_Type mfrc522_stub_coredebug;

HAL_StatusTypeDef HAL_Init(void) { return HAL_OK; }

HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *h, uint8_t *pData,
                                   uint16_t Size, uint32_t Timeout)
{ (void)h; (void)pData; (void)Size; (void)Timeout; return HAL_OK; }

HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *h, uint8_t *pData,
                                  uint16_t Size, uint32_t Timeout)
{ (void)h; (void)pData; (void)Size; (void)Timeout; return HAL_OK; }

uint32_t HAL_SPI_GetError(SPI_HandleTypeDef *h)
{ (void)h; return 0u; }

HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *h, uint16_t a,
                                          uint8_t *pData, uint16_t Size, uint32_t t)
{ (void)h; (void)a; (void)pData; (void)Size; (void)t; return HAL_OK; }

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *h, uint16_t a, uint16_t m,
                                   uint16_t s, uint8_t *pData, uint16_t Size, uint32_t t)
{ (void)h; (void)a; (void)m; (void)s; (void)pData; (void)Size; (void)t; return HAL_OK; }

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *h, uint8_t *pData,
                                    uint16_t Size, uint32_t t)
{ (void)h; (void)pData; (void)Size; (void)t; return HAL_OK; }

HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *h, uint8_t *pData,
                                   uint16_t Size, uint32_t t)
{ (void)h; (void)pData; (void)Size; (void)t; return HAL_OK; }

void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState s)
{ (void)port; (void)pin; (void)s; }

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{ (void)port; (void)pin; return GPIO_PIN_RESET; }

void HAL_Delay(uint32_t ms) { (void)ms; }

uint32_t HAL_GetTick(void) { return 0u; }

uint32_t HAL_RCC_GetHCLKFreq(void) { return 480000000u; }
