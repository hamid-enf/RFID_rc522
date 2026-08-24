/**
 * @file    mfrc522_config.h
 * @brief   Compile-time configuration for the MFRC522 driver.
 *
 * All configuration in this library is resolved at compile time. There is
 * **no dynamic memory allocation** and **no printf** in the core. Every
 * feature can be disabled at build time so that unused code is stripped by
 * the linker, reducing Flash/RAM footprint.
 *
 * The user may override any of these values BEFORE including this header by
 * defining the corresponding macro with a compiler flag (-D) or in a project
 * "app_config.h" file.
 *
 * @note  This header must be included first (it is included by all other
 *        public headers) and must not pull in any hardware dependency.
 */

#ifndef MFRC522_CONFIG_H
#define MFRC522_CONFIG_H

/* ------------------------------------------------------------------ */
/* Feature toggles                                                     */
/* ------------------------------------------------------------------ */

/**
 * @brief Enable the SPI host-interface transport (default: on).
 */
#ifndef MFRC522_ENABLE_SPI
#define MFRC522_ENABLE_SPI        (1)
#endif

/**
 * @brief Enable the I2C host-interface transport (default: on).
 */
#ifndef MFRC522_ENABLE_I2C
#define MFRC522_ENABLE_I2C        (1)
#endif

/**
 * @brief Enable the UART host-interface transport (default: on).
 *
 * @note  See docs/uart.md for the hardware limitations of the MFRC522 UART
 *        host interface before relying on it.
 */
#ifndef MFRC522_ENABLE_UART
#define MFRC522_ENABLE_UART       (1)
#endif

/**
 * @brief Enable the interrupt (IRQ) API and the IRQ state machine.
 */
#ifndef MFRC522_ENABLE_IRQ
#define MFRC522_ENABLE_IRQ        (1)
#endif

/**
 * @brief Enable MIFARE Classic / Ultralight high-level API (default: on).
 */
#ifndef MFRC522_ENABLE_MIFARE
#define MFRC522_ENABLE_MIFARE     (1)
#endif

/**
 * @brief Enable the non-blocking (asynchronous) operation state machine.
 *
 * The blocking API is always available. Enabling this adds a small amount of
 * RAM (one operation context in the handle state) but does not require any
 * dynamic allocation.
 */
#ifndef MFRC522_ENABLE_NONBLOCKING
#define MFRC522_ENABLE_NONBLOCKING (0)
#endif

/**
 * @brief Enable debug logging through the MFRC522_Debug_t callback interface.
 *
 * When disabled, all MFRC522_LOG*() calls compile to nothing.
 */
#ifndef MFRC522_ENABLE_DEBUG
#define MFRC522_ENABLE_DEBUG      (0)
#endif

/* ------------------------------------------------------------------ */
/* Sizing / defaults                                                   */
/* ------------------------------------------------------------------ */

/**
 * @brief Size in bytes of the per-handle opaque platform storage.
 *
 * The platform adapter (e.g. platform/stm32) stores its private context
 * (peripheral handle, GPIO ports/pins, ...) inside the MFRC522_Handle_t.
 * This buffer must be large enough to hold the adapter's context struct.
 *
 * On a 32-bit MCU the STM32 SPI context needs 24 bytes; on a 64-bit host
 * (used for compile/run tests) it needs 56 bytes. 64 covers both, stays
 * naturally aligned, and costs only 64 bytes of RAM per handle on target.
 * A compile-time check in the adapter asserts that the chosen size suffices.
 */
#ifndef MFRC522_PLATFORM_CTX_SIZE
#define MFRC522_PLATFORM_CTX_SIZE (64u)
#endif

/**
 * @brief Default operation timeout in milliseconds (used by every blocking
 *        loop that waits on a hardware condition).
 */
#ifndef MFRC522_DEFAULT_TIMEOUT_MS
#define MFRC522_DEFAULT_TIMEOUT_MS (2000u)
#endif

/**
 * @brief Delay after a hard/soft reset before the first register access.
 */
#ifndef MFRC522_RESET_WAIT_MS
#define MFRC522_RESET_WAIT_MS      (50u)
#endif

/**
 * @brief Timeout for a single card-presence poll (REQA/WUPA) inside
 *        MFRC522_IsCardPresent() / MFRC522_WaitForCard(). Kept short so the
 *        presence check stays responsive.
 */
#ifndef MFRC522_CARD_POLL_TIMEOUT_MS
#define MFRC522_CARD_POLL_TIMEOUT_MS (50u)
#endif

/**
 * @brief Antenna settling time in microseconds after switching Tx on.
 */
#ifndef MFRC522_ANTENNA_SETTLE_US
#define MFRC522_ANTENNA_SETTLE_US  (50u)
#endif

/**
 * @brief FIFO depth of the MFRC522 (fixed by hardware).
 */
#define MFRC522_FIFO_SIZE          (64u)

/**
 * @brief Maximum UID length (10 bytes: 4-byte + 2 cascade tags of 3 bytes).
 */
#define MFRC522_UID_MAX_LEN        (10u)

/**
 * @brief Size of one MIFARE block (16 bytes).
 */
#define MFRC522_BLOCK_SIZE         (16u)

/**
 * @brief Default I2C 7-bit slave address (all ADR pins low, EA low).
 *        See docs/i2c.md for the full address calculation.
 */
#ifndef MFRC522_I2C_DEFAULT_ADDR
#define MFRC522_I2C_DEFAULT_ADDR   (0x28u)
#endif

/**
 * @brief SPI clock speed selector (index into the timing table in
 *        interface/mfrc522_spi.c). 0 = most conservative (~1 MHz), 1 = 4 MHz,
 *        2 = up to 10 MHz (hardware maximum). Slower modes tolerate longer
 *        wires; use them when debugging.
 */
#ifndef MFRC522_SPI_SPEED
#define MFRC522_SPI_SPEED          (0u)
#endif

/**
 * @brief UART baud-rate selector (index into the MFRC522_UART_BAUD_xxx list
 *        declared in mfrc522_types.h). Default index 5 == 115200 baud.
 */
#ifndef MFRC522_UART_BAUD
#define MFRC522_UART_BAUD          (5u)
#endif

#endif /* MFRC522_CONFIG_H */
