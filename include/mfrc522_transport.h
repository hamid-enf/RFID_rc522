/**
 * @file    mfrc522_transport.h
 * @brief   Host-interface abstraction: transport layer + platform (MCU)
 *          abstraction.
 *
 * The core library never talks to SPI/I2C/UART or GPIO directly. It sees two
 * small, well-defined interfaces:
 *
 *   1. MFRC522_PlatformOps_t — the "MCU Hardware Abstraction". Implemented
 *      once per platform (platform/stm32/...). Provides timing, GPIO (CS,
 *      reset, IRQ), raw byte I/O and optional locking. This is the ONLY place
 *      that may call HAL_*() functions.
 *
 *   2. MFRC522_TransportOps_t — the "Transport Layer". Implemented once per
 *      host interface (interface/mfrc522_spi.c, _i2c.c, _uart.c). Converts
 *      abstract register read/write requests into the byte sequences the
 *      MFRC522 expects for the selected host interface (address byte for SPI,
 *      device-address frame for I2C, LSB-first address byte for UART).
 *
 * Both layers use plain function-pointer tables ("vtables") and opaque
 * context pointers: no dynamic allocation, no global state, thread-safe when
 * a lock is provided.
 */

#ifndef MFRC522_TRANSPORT_H
#define MFRC522_TRANSPORT_H

#include <stdint.h>
#include "mfrc522_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Forward declaration of the reader handle (defined in mfrc522.h). */
typedef struct MFRC522_Handle MFRC522_Handle_t;

/* ================================================================== */
/*  Platform (MCU hardware abstraction)                              */
/* ================================================================== */

/**
 * @brief Function table that abstracts the host MCU away from the driver.
 *
 * All functions receive the opaque `ctx` pointer supplied by the platform
 * adapter. Implementations may be blocking or interrupt/DMA-driven; the
 * driver only requires that a call returns only when the operation finished
 * (or failed). A lock/unlock pair may be provided for RTOS thread safety.
 */
typedef struct MFRC522_PlatformOps
{
    /* ---- timing -------------------------------------------------- */
    void      (*delay_us)(void *ctx, uint32_t us);   /**< Busy/blocking µs delay. */
    void      (*delay_ms)(void *ctx, uint32_t ms);   /**< Blocking ms delay.      */
    uint32_t  (*get_tick_ms)(void *ctx);             /**< Free-running ms tick.   */

    /* ---- GPIO ---------------------------------------------------- */
    void      (*cs_assert)(void *ctx);               /**< Drive chip-select low.  */
    void      (*cs_deassert)(void *ctx);             /**< Drive chip-select high. */
    void      (*reset_assert)(void *ctx);            /**< Drive NRSTPD low.       */
    void      (*reset_deassert)(void *ctx);          /**< Release NRSTPD.         */
    uint8_t   (*irq_read)(void *ctx);                /**< Read IRQ pin level (1/0). */

    /* ---- raw byte I/O (the specific peripheral is chosen by the   */
    /*      transport that consumes these) --------------------------- */
    MFRC522_Status_t (*transmit)(void *ctx, const uint8_t *tx, uint32_t len);
    MFRC522_Status_t (*receive)(void *ctx, uint8_t *rx, uint32_t len);
    MFRC522_Status_t (*transmit_receive)(void *ctx, const uint8_t *tx,
                                         uint8_t *rx, uint32_t len);

    /**
     * @brief Combined "write register address, then read data" access.
     *
     * This is the natural memory-mapped read of the I2C host interface
     * (device address + register address, repeated start, read). The I2C
     * transport uses it for register reads; SPI/UART adapters set it to NULL
     * because they express reads through transmit/receive.
     *
     * @param tx      Register address byte(s) — for I2C, a single byte.
     * @param tx_len  Number of address bytes.
     * @param rx      Receive buffer.
     * @param rx_len  Number of bytes to read.
     */
    MFRC522_Status_t (*write_read)(void *ctx, const uint8_t *tx,
                                   uint32_t tx_len, uint8_t *rx,
                                   uint32_t rx_len);

    /* ---- optional locking (NULL = single-threaded) --------------- */
    void      (*lock)(void *ctx);                    /**< Take the bus lock.      */
    void      (*unlock)(void *ctx);                  /**< Release the bus lock.   */
} MFRC522_PlatformOps_t;

/**
 * @brief A platform instance: an ops table plus its opaque context.
 */
typedef struct MFRC522_Platform
{
    const MFRC522_PlatformOps_t *ops;   /**< Function table (often const/ROM).   */
    void                        *ctx;   /**< Opaque platform context.            */
} MFRC522_Platform_t;

/* ================================================================== */
/*  Transport (host-interface framing)                               */
/* ================================================================== */

/**
 * @brief Transport instance: the selected host interface and its parameters.
 */
typedef struct MFRC522_Transport
{
    MFRC522_TransportType_t type;   /**< SPI / I2C / UART.                       */
    uint8_t i2c_addr;               /**< I2C 7-bit slave address (default 0x28). */
    uint8_t uart_baud;              /**< MFRC522_UartBaud_t index.               */
    uint8_t spi_speed;              /**< MFRC522_SpiSpeed_t index.               */
    uint8_t reserved;               /**< Reserved for alignment.                 */
} MFRC522_Transport_t;

/**
 * @brief Function table that converts register-level requests into the byte
 *        framing of a specific host interface.
 *
 * `t`   - transport parameters (address/baud/speed).
 * `p`   - the platform (ops + ctx) used for the actual byte I/O.
 */
typedef struct MFRC522_TransportOps
{
    MFRC522_Status_t (*read_register)(const MFRC522_Transport_t *t,
                                      const MFRC522_Platform_t *p,
                                      uint8_t addr, uint8_t *value);

    MFRC522_Status_t (*write_register)(const MFRC522_Transport_t *t,
                                       const MFRC522_Platform_t *p,
                                       uint8_t addr, uint8_t value);

    MFRC522_Status_t (*read_burst)(const MFRC522_Transport_t *t,
                                   const MFRC522_Platform_t *p,
                                   uint8_t addr, uint8_t *data, uint32_t len);

    MFRC522_Status_t (*write_burst)(const MFRC522_Transport_t *t,
                                    const MFRC522_Platform_t *p,
                                    uint8_t addr, const uint8_t *data,
                                    uint32_t len);
} MFRC522_TransportOps_t;

/* ================================================================== */
/*  Transport instances (defined in the interface/ sources)           */
/* ================================================================== */

#if MFRC522_ENABLE_SPI
/** SPI transport ops table (implemented in interface/mfrc522_spi.c). */
extern const MFRC522_TransportOps_t MFRC522_SPI_TransportOps;
#endif

#if MFRC522_ENABLE_I2C
/** I2C transport ops table (implemented in interface/mfrc522_i2c.c). */
extern const MFRC522_TransportOps_t MFRC522_I2C_TransportOps;
#endif

#if MFRC522_ENABLE_UART
/** UART transport ops table (implemented in interface/mfrc522_uart.c). */
extern const MFRC522_TransportOps_t MFRC522_UART_TransportOps;

/**
 * @brief Write the SerialSpeedReg (0x1F) with the baud divisor that matches
 *        the handle's configured UART baud rate.
 *
 * Called by MFRC522_Init() when the UART host interface is selected. Also
 * useful to change the baud rate at runtime (after which the MCU UART must be
 * reconfigured to match).
 */
MFRC522_Status_t MFRC522_UART_ApplyBaud(MFRC522_Handle_t *handle);
#endif

#ifdef __cplusplus
}
#endif

#endif /* MFRC522_TRANSPORT_H */
