/**
 * @file    mfrc522_stm32_internal.h
 * @brief   Internal declarations shared by the STM32 platform adapter files.
 *
 * NOT part of the public API. The adapter consists of:
 *   - mfrc522_stm32_time.c   : µs/ms delays + ms tick (DWT + HAL)
 *   - mfrc522_stm32_gpio.c   : shared reset/IRQ GPIO helpers
 *   - mfrc522_stm32_spi.c    : SPI interface adapter
 *   - mfrc522_stm32_i2c.c    : I2C interface adapter
 *   - mfrc522_stm32_uart.c   : UART interface adapter
 *
 * This is the ONLY place in the repository that may call HAL_*() functions.
 */

#ifndef MFRC522_STM32_INTERNAL_H
#define MFRC522_STM32_INTERNAL_H

#include "mfrc522_stm32.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Default HAL transfer timeout (ms) for SPI/I2C/UART byte I/O.
 *        Override with -DMFRC522_STM32_IO_TIMEOUT=... if needed.
 */
#ifndef MFRC522_STM32_IO_TIMEOUT
#define MFRC522_STM32_IO_TIMEOUT (1000u)
#endif

/* ---- timing (mfrc522_stm32_time.c) ------------------------------- */

/** Enable the DWT cycle counter (call once from any adapter init). */
void mfrc522_stm32_time_init(void);

/** Busy-wait µs delay (DWT cycle counter). */
void mfrc522_stm32_delay_us(void *ctx, uint32_t us);

/** Blocking ms delay (HAL_Delay). */
void mfrc522_stm32_delay_ms(void *ctx, uint32_t ms);

/** Free-running millisecond tick (HAL_GetTick). */
uint32_t mfrc522_stm32_get_tick_ms(void *ctx);

/* ---- GPIO (mfrc522_stm32_gpio.c) --------------------------------- */

/** Drive NRSTPD low (hard reset / power-down). */
void mfrc522_stm32_reset_assert(void *ctx);

/** Release NRSTPD. */
void mfrc522_stm32_reset_deassert(void *ctx);

/** Read the IRQ pin level (1/0). Returns 0 when the pin is not wired. */
uint8_t mfrc522_stm32_irq_read(void *ctx);

#ifdef __cplusplus
}
#endif

#endif /* MFRC522_STM32_INTERNAL_H */
