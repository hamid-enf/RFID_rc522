/**
 * @file    mfrc522_stm32.h
 * @brief   Common declarations for the STM32 HAL platform adapter.
 *
 * This is the only place in the whole project where the STM32 HAL header is
 * included. The core library (<mfrc522.h>) has no knowledge of STM32.
 *
 * The correct HAL header is selected by the standard STM32 device macro
 * (usually supplied by the toolchain via the CMSIS device header). Add more
 * families here as needed — the adapter code itself is family-independent.
 */

#ifndef MFRC522_STM32_H
#define MFRC522_STM32_H

/* ---- Resolve the HAL header from the device define ------------------ */
#if defined(STM32H743xx) || defined(STM32H750xx) || defined(STM32H753xx) || \
    defined(STM32H7A3xx) || defined(STM32H7B3xx) || defined(STM32H5xx)
#include "stm32h7xx_hal.h"
#elif defined(STM32F0xx)
#include "stm32f0xx_hal.h"
#elif defined(STM32F1xx)
#include "stm32f1xx_hal.h"
#elif defined(STM32F3xx)
#include "stm32f3xx_hal.h"
#elif defined(STM32F4xx)
#include "stm32f4xx_hal.h"
#elif defined(STM32F7xx)
#include "stm32f7xx_hal.h"
#elif defined(STM32G0xx)
#include "stm32g0xx_hal.h"
#elif defined(STM32G4xx)
#include "stm32g4xx_hal.h"
#elif defined(STM32L0xx)
#include "stm32l0xx_hal.h"
#elif defined(STM32L4xx)
#include "stm32l4xx_hal.h"
#else
/* Default: STM32H7 (the primary target of this repository). */
#include "stm32h7xx_hal.h"
#endif

#include "mfrc522.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Shared reset/IRQ GPIO pair, common to every host-interface context.
 *
 * It is embedded as the FIRST member of each interface context so that the
 * shared reset/IRQ helpers in mfrc522_stm32_gpio.c can treat the context
 * pointer as a MFRC522_STM32_Gpio_t*. Both fields are optional (NULL/0 =
 * not wired).
 */
typedef struct MFRC522_STM32_Gpio
{
    GPIO_TypeDef *rst_port;     /**< NRSTPD port (may be NULL).   */
    uint16_t      rst_pin;      /**< NRSTPD pin  (may be 0).      */
    GPIO_TypeDef *irq_port;     /**< IRQ port    (may be NULL).   */
    uint16_t      irq_pin;      /**< IRQ pin     (may be 0).      */
} MFRC522_STM32_Gpio_t;

/**
 * @brief Compile-time assertion helper (portable C99: a typedef of a
 *        negative-size array fails to compile if the condition is false).
 *
 * Two-level expansion so __LINE__ is unique per use site.
 */
#define MFRC522_STM32_CT_ASSERT_(cond, line) \
    typedef char MFRC522_ct_assert_##line[(cond) ? 1 : -1]
#define MFRC522_STM32_CT_ASSERT(cond) MFRC522_STM32_CT_ASSERT_(cond, __LINE__)

#ifdef __cplusplus
}
#endif

#endif /* MFRC522_STM32_H */
