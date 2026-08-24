/**
 * @file    mfrc522_stm32_time.c
 * @brief   Timing primitives for the STM32 platform adapter.
 *
 *   - mfrc522_stm32_delay_us    : DWT cycle-counter busy wait (µs-accurate on
 *                                 every Cortex-M3/M4/M7/M33 core).
 *   - mfrc522_stm32_delay_ms    : HAL_Delay (SysTick).
 *   - mfrc522_stm32_get_tick_ms : HAL_GetTick (free-running ms counter).
 *
 * The DWT counter is enabled once (idempotent). The CPU frequency is read via
 * HAL_RCC_GetHCLKFreq(), so no hard-coded clock value is needed.
 */

#include "mfrc522_stm32_internal.h"

void mfrc522_stm32_time_init(void)
{
    /* Enable the debug monitor (TRCENA) and the cycle counter. */
    if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0u) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0u;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
}

static uint32_t mfrc522_stm32_cpu_mhz(void)
{
    uint32_t hz = HAL_RCC_GetHCLKFreq();
    if (hz == 0u) {
        return 1u;   /* avoid divide-by-zero on a misconfigured clock */
    }
    return hz / 1000000u;
}

void mfrc522_stm32_delay_us(void *ctx, uint32_t us)
{
    uint32_t start;
    uint32_t ticks;

    (void)ctx;

    if (us == 0u) {
        return;
    }

    start = DWT->CYCCNT;
    ticks = us * mfrc522_stm32_cpu_mhz();

    /* The subtraction is unsigned and wraps correctly on overflow. */
    while ((DWT->CYCCNT - start) < ticks) {
        /* busy wait */
    }
}

void mfrc522_stm32_delay_ms(void *ctx, uint32_t ms)
{
    (void)ctx;
    HAL_Delay(ms);
}

uint32_t mfrc522_stm32_get_tick_ms(void *ctx)
{
    (void)ctx;
    return HAL_GetTick();
}
