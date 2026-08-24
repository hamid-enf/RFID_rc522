/**
 * @file    mfrc522_stm32_gpio.c
 * @brief   Shared reset/IRQ GPIO helpers for the STM32 platform adapter.
 *
 * The reset (NRSTPD) and IRQ pins are common to every host interface, so they
 * are factored here. Each interface context embeds a MFRC522_STM32_Gpio_t as
 * its FIRST member, so the opaque context pointer can be safely reinterpreted
 * as MFRC522_STM32_Gpio_t*.
 *
 * Both pins are optional: a NULL port (or 0 pin) makes the corresponding
 * helper a no-op / return 0.
 */

#include "mfrc522_stm32_internal.h"

static MFRC522_STM32_Gpio_t *mfrc522_stm32_gpio(void *ctx)
{
    return (MFRC522_STM32_Gpio_t *)ctx;
}

void mfrc522_stm32_reset_assert(void *ctx)
{
    MFRC522_STM32_Gpio_t *g = mfrc522_stm32_gpio(ctx);
    if (g->rst_port != NULL) {
        HAL_GPIO_WritePin(g->rst_port, g->rst_pin, GPIO_PIN_RESET);
    }
}

void mfrc522_stm32_reset_deassert(void *ctx)
{
    MFRC522_STM32_Gpio_t *g = mfrc522_stm32_gpio(ctx);
    if (g->rst_port != NULL) {
        HAL_GPIO_WritePin(g->rst_port, g->rst_pin, GPIO_PIN_SET);
    }
}

uint8_t mfrc522_stm32_irq_read(void *ctx)
{
    MFRC522_STM32_Gpio_t *g = mfrc522_stm32_gpio(ctx);
    if (g->irq_port == NULL) {
        return 0u;
    }
    return (HAL_GPIO_ReadPin(g->irq_port, g->irq_pin) == GPIO_PIN_SET) ? 1u : 0u;
}
