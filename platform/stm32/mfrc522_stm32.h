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
 * @brief Compile-time assertion helper (MISRA-friendly: no _Static_assert
 *        macro tricks, uses a typedef of a negative-size array on failure).
 */
#define MFRC522_STM32_CT_ASSERT(cond) \
    typedef char MFRC522_ct_assert_##__LINE__[(cond) ? 1 : -1]

#ifdef __cplusplus
}
#endif

#endif /* MFRC522_STM32_H */
