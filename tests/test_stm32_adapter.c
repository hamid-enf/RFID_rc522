/**
 * @file    test_stm32_adapter.c
 * @brief   Host-side test of the STM32 platform adapter wiring.
 *
 * Uses the minimal HAL stub (tests/stm32_hal_stub) to verify that each
 * MFRC522_STM32_*_Init() correctly populates the handle: transport type,
 * transport ops table, platform ops table and the opaque context pointer
 * (which must point inside handle->platform_storage — no heap).
 *
 * Build:
 *   cc -std=c99 -Wall -Wextra -DSTM32H743xx \
 *      -Iinclude -Iplatform/stm32 -Itests/stm32_hal_stub \
 *      test_stm32_adapter.c \
 *      src/mfrc522.c src/mfrc522_registers.c src/mfrc522_crc.c \
 *      src/mfrc522_irq.c src/mfrc522_protocol.c src/mfrc522_mifare.c \
 *      interface/mfrc522_spi.c interface/mfrc522_i2c.c interface/mfrc522_uart.c \
 *      platform/stm32/mfrc522_stm32_spi.c platform/stm32/mfrc522_stm32_i2c.c \
 *      platform/stm32/mfrc522_stm32_uart.c platform/stm32/mfrc522_stm32_gpio.c \
 *      platform/stm32/mfrc522_stm32_time.c \
 *      tests/stm32_hal_stub/stm32h7xx_hal.c -o test_stm32_adapter
 */

#include <stdio.h>
#include <string.h>
#include "mfrc522_stm32_spi.h"
#include "mfrc522_stm32_i2c.h"
#include "mfrc522_stm32_uart.h"

static int g_failures = 0;

#define CHECK(cond) do {                                                   \
    if (!(cond)) {                                                         \
        printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);           \
        g_failures++;                                                      \
    }                                                                      \
} while (0)

static GPIO_TypeDef    gpio;
static SPI_HandleTypeDef  hspi;
static I2C_HandleTypeDef  hi2c;
static UART_HandleTypeDef huart;

int main(void)
{
    MFRC522_Handle_t rfid;

    printf("MFRC522 STM32 adapter wiring tests\n");
    printf("----------------------------------\n");

    /* ---- SPI ---- */
    memset(&rfid, 0, sizeof(rfid));
    CHECK(MFRC522_STM32_SPI_Init(&rfid, &hspi, &gpio, 4u, &gpio, 0u, NULL, 0u)
          == MFRC522_OK);
    CHECK(rfid.transport.type == MFRC522_TRANSPORT_SPI);
    CHECK(rfid.transport_ops == &MFRC522_SPI_TransportOps);
    CHECK(rfid.platform.ops != NULL);
    CHECK(rfid.platform.ctx != NULL);
    /* ctx must live inside the handle's reserved storage (no heap). */
    CHECK((const uint8_t *)rfid.platform.ctx >= rfid.platform_storage.bytes);
    CHECK((const uint8_t *)rfid.platform.ctx <
          (rfid.platform_storage.bytes + MFRC522_PLATFORM_CTX_SIZE));
    CHECK(rfid.platform.ops->cs_assert != NULL);
    CHECK(rfid.platform.ops->cs_deassert != NULL);
    CHECK(rfid.platform.ops->transmit != NULL);
    CHECK(rfid.platform.ops->receive != NULL);

    /* ---- I2C ---- */
    memset(&rfid, 0, sizeof(rfid));
    CHECK(MFRC522_STM32_I2C_Init(&rfid, &hi2c, MFRC522_I2C_DEFAULT_ADDR,
                                 &gpio, 0u, NULL, 0u) == MFRC522_OK);
    CHECK(rfid.transport.type == MFRC522_TRANSPORT_I2C);
    CHECK(rfid.transport.i2c_addr == MFRC522_I2C_DEFAULT_ADDR);
    CHECK(rfid.transport_ops == &MFRC522_I2C_TransportOps);
    CHECK(rfid.platform.ops->write_read != NULL);
    CHECK(rfid.platform.ctx != NULL);

    /* ---- UART ---- */
    memset(&rfid, 0, sizeof(rfid));
    CHECK(MFRC522_STM32_UART_Init(&rfid, &huart, MFRC522_UART_BAUD_115200,
                                  &gpio, 0u, NULL, 0u) == MFRC522_OK);
    CHECK(rfid.transport.type == MFRC522_TRANSPORT_UART);
    CHECK(rfid.transport.uart_baud == MFRC522_UART_BAUD_115200);
    CHECK(rfid.transport_ops == &MFRC522_UART_TransportOps);
    CHECK(rfid.platform.ctx != NULL);

    /* ---- invalid args ---- */
    memset(&rfid, 0, sizeof(rfid));
    CHECK(MFRC522_STM32_SPI_Init(NULL, &hspi, &gpio, 0u, NULL, 0u, NULL, 0u)
          == MFRC522_ERR_INVALID_PARAM);
    CHECK(MFRC522_STM32_UART_Init(&rfid, &huart, 255u, NULL, 0u, NULL, 0u)
          == MFRC522_ERR_INVALID_PARAM);   /* bad baud index */

    if (g_failures == 0) {
        printf("\nALL TESTS PASSED\n");
        return 0;
    }
    printf("\n%d CHECK(S) FAILED\n", g_failures);
    return 1;
}
