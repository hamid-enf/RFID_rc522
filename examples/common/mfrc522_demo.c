/**
 * @file    mfrc522_demo.c
 * @brief   demo_printf() implementation (examples only).
 *
 * Formats into a static buffer and transmits it over the demo UART using
 * HAL_UART_Transmit. Used by every example for console output; the core
 * library itself never calls printf.
 */

#include <stdarg.h>
#include "mfrc522_demo.h"

#define DEMO_PRINTF_BUF_SIZE (128u)

void demo_printf(const char *fmt, ...)
{
    static char buf[DEMO_PRINTF_BUF_SIZE];
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (n > 0) {
        uint32_t len = (uint32_t)n;
        if (len >= sizeof(buf)) {
            len = sizeof(buf) - 1u;   /* truncated */
        }
        (void)HAL_UART_Transmit(MFRC522_DEMO_UART, (uint8_t *)buf,
                                (uint16_t)len, 1000u);
    }
}
