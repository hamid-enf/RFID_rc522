/**
 * @file    mfrc522_types.h
 * @brief   Public types, enumerations and small data structures for the
 *          MFRC522 driver.
 *
 * This header has **no** hardware dependency. It only uses standard integer
 * types (<stdint.h>) and is safe to include from application, platform and
 * core code alike.
 *
 * Naming conventions:
 *   - Types       : MFRC522_Xxx_t
 *   - Enums       : MFRC522_Xxx_t (typedef'd enum)
 *   - Enum values : MFRC522_XXX_YYY
 *   - Macros      : MFRC522_XXX
 */

#ifndef MFRC522_TYPES_H
#define MFRC522_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include "mfrc522_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Forward declaration of the main reader handle.                     */
/* ------------------------------------------------------------------ */

/**
 * @brief The main reader handle is *defined* in mfrc522.h (it needs the
 *        transport/platform types). It is forward-declared here so the
 *        protocol / MIFARE headers can reference MFRC522_Handle_t* in their
 *        prototypes without a circular include.
 */
struct MFRC522_Handle;
typedef struct MFRC522_Handle MFRC522_Handle_t;

/* ================================================================== */
/*  Status / error codes                                              */
/* ================================================================== */

/**
 * @brief Return status for every public API function.
 *
 * MFRC522_OK is always zero so that a plain `if (MFRC522_xxx(...))` test is
 * a valid "did it fail?" check.
 */
typedef enum MFRC522_Status
{
    MFRC522_OK = 0,              /**< Operation completed successfully.            */
    MFRC522_ERR_INVALID_PARAM,   /**< NULL pointer or out-of-range argument.      */
    MFRC522_ERR_BUSY,            /**< Another (non-blocking) operation in flight. */
    MFRC522_ERR_TIMEOUT,         /**< A timed wait expired.                       */
    MFRC522_ERR_COMM,            /**< Host interface (SPI/I2C/UART) failure.      */
    MFRC522_ERR_CRC,             /**< CRC mismatch (card reply or block read).    */
    MFRC522_ERR_COLLISION,       /**< Bit-collision during anti-collision.        */
    MFRC522_ERR_AUTH,            /**< Crypto1 authentication failed.              */
    MFRC522_ERR_NO_CARD,         /**< No card in the field / no reply to REQA.    */
    MFRC522_ERR_PROTOCOL,        /**< ISO/IEC 14443-A protocol violation.         */
    MFRC522_ERR_DEVICE,          /**< Unexpected VersionReg / wrong device.       */
    MFRC522_ERR_FIFO,            /**< FIFO underflow/overflow while draining.     */
    MFRC522_ERR_OVERFLOW,        /**< Destination buffer too small.               */
    MFRC522_ERR_NOT_SUPPORTED,   /**< Feature disabled at compile time or by HW.  */
    MFRC522_ERR_INTERNAL,        /**< Internal invariant violated (shouldn't happen). */
    MFRC522_STATUS_COUNT         /**< Number of status codes (not a real status). */
} MFRC522_Status_t;

/* ================================================================== */
/*  Host interface / transport                                        */
/* ================================================================== */

/**
 * @brief Host interface type used to talk to the MFRC522.
 */
typedef enum MFRC522_TransportType
{
    MFRC522_TRANSPORT_SPI = 0,   /**< 4-wire SPI (NSS, SCK, MOSI, MISO). */
    MFRC522_TRANSPORT_I2C,       /**< I2C-bus (SDA, SCL).               */
    MFRC522_TRANSPORT_UART,      /**< Serial UART (RX, TX).             */
    MFRC522_TRANSPORT_COUNT
} MFRC522_TransportType_t;

/**
 * @brief UART host-interface baud-rate selectors (indices, not bit rates).
 *
 * Maps 1:1 to the baud-rate table of the MFRC522 serial UART interface
 * (see the NXP datasheet, "UART interface"). The concrete divisor is
 * resolved in interface/mfrc522_uart.c.
 */
typedef enum MFRC522_UartBaud
{
    MFRC522_UART_BAUD_9600 = 0,
    MFRC522_UART_BAUD_14400,
    MFRC522_UART_BAUD_19200,
    MFRC522_UART_BAUD_38400,
    MFRC522_UART_BAUD_57600,
    MFRC522_UART_BAUD_115200,
    MFRC522_UART_BAUD_128000,
    MFRC522_UART_BAUD_230400,
    MFRC522_UART_BAUD_460800,
    MFRC522_UART_BAUD_921600,
    MFRC522_UART_BAUD_1228800,
    MFRC522_UART_BAUD_COUNT
} MFRC522_UartBaud_t;

/**
 * @brief SPI host-interface speed selector (index into the timing table).
 */
typedef enum MFRC522_SpiSpeed
{
    MFRC522_SPI_SPEED_LOW = 0,   /**< ~1 MHz, maximum robustness.   */
    MFRC522_SPI_SPEED_MED,       /**< ~4 MHz.                       */
    MFRC522_SPI_SPEED_HIGH,      /**< up to 10 MHz (hardware max).  */
    MFRC522_SPI_SPEED_COUNT
} MFRC522_SpiSpeed_t;

/* ================================================================== */
/*  Chip version                                                      */
/* ================================================================== */

/**
 * @brief Decoded VersionReg (0x30) content.
 */
typedef struct MFRC522_Version
{
    uint8_t raw;                 /**< Raw register value as read back.        */
    uint8_t major;               /**< Major version digit (e.g. 2).           */
    uint8_t minor;               /**< Minor version digit (e.g. 0).           */
} MFRC522_Version_t;

/** @name Known VersionReg values (raw). */
#define MFRC522_VERSION_1_0       (0x91u)   /**< MFRC522 silicon v1.0. */
#define MFRC522_VERSION_2_0       (0x92u)   /**< MFRC522 silicon v2.0. */

/* ================================================================== */
/*  Card types (ISO/IEC 14443-A / MIFARE)                             */
/* ================================================================== */

/**
 * @brief High-level card type. Derived from SAK / ATQA during select.
 */
typedef enum MFRC522_CardType
{
    MFRC522_CARD_UNKNOWN = 0,
    MFRC522_CARD_MIFARE_MINI,        /**< MIFARE Mini (320 B, SAK 0x09).      */
    MFRC522_CARD_MIFARE_1K,          /**< MIFARE Classic 1K (SAK 0x08).       */
    MFRC522_CARD_MIFARE_4K,          /**< MIFARE Classic 4K (SAK 0x18).       */
    MFRC522_CARD_MIFARE_ULTRALIGHT,  /**< MIFARE Ultralight / NTAG (SAK 0x00).*/
    MFRC522_CARD_MIFARE_ULTRALIGHT_C,/**< MIFARE Ultralight C.                */
    MFRC522_CARD_ISO14443_4,         /**< PICC compliant to ISO 14443-4 (SAK 0x20). */
    MFRC522_CARD_COUNT
} MFRC522_CardType_t;

/**
 * @brief Authentication key selector for MIFARE Classic.
 */
typedef enum MFRC522_KeyType
{
    MFRC522_KEY_A = 0,
    MFRC522_KEY_B = 1
} MFRC522_KeyType_t;

/**
 * @brief 6-byte Crypto1 authentication key (MIFARE Classic).
 */
typedef struct MFRC522_Key
{
    uint8_t key[6];              /**< Key bytes, MSB first (per MIFARE).       */
} MFRC522_Key_t;

/**
 * @brief A card UID (4, 7 or 10 bytes after full cascade resolution).
 */
typedef struct MFRC522_UID
{
    uint8_t bytes[MFRC522_UID_MAX_LEN];  /**< Concatenated UID bytes.          */
    uint8_t length;                      /**< Number of valid bytes (4/7/10).  */
    uint8_t sak;                         /**< SAK of the final select.         */
} MFRC522_UID_t;

/**
 * @brief Complete card information gathered during anti-collision + select.
 */
typedef struct MFRC522_CardInfo
{
    uint8_t uid[MFRC522_UID_MAX_LEN];    /**< UID bytes (as MFRC522_UID_t).    */
    uint8_t uid_length;                  /**< UID length: 4, 7 or 10.          */
    uint8_t atqa[2];                     /**< Answer To Request, type A.       */
    uint8_t sak;                         /**< Select Acknowledge.              */
    MFRC522_CardType_t type;             /**< Derived high-level card type.    */
} MFRC522_CardInfo_t;

/* ================================================================== */
/*  Runtime configuration                                             */
/* ================================================================== */

/**
 * @brief Runtime configuration copied into the handle by MFRC522_Init().
 *
 * All fields have compile-time defaults in mfrc522_config.h. Set any field
 * before calling MFRC522_Init() to override the default.
 */
typedef struct MFRC522_Config
{
    uint32_t timeout_ms;         /**< Default blocking-operation timeout.      */
    uint32_t reset_wait_ms;      /**< Delay after hard/soft reset.             */
    uint32_t antenna_settle_us;  /**< Antenna settling time.                   */
    uint8_t  irq_enabled;        /**< 1 = enable IRQ-based flow where supported.*/
    uint8_t  reserved[3];        /**< Reserved for alignment.                  */
} MFRC522_Config_t;

/* ================================================================== */
/*  Debug / logging                                                   */
/* ================================================================== */

/**
 * @brief Log severity levels (mirrors a typical embedded log subsystem).
 */
typedef enum MFRC522_LogLevel
{
    MFRC522_LOG_ERROR = 0,
    MFRC522_LOG_WARNING,
    MFRC522_LOG_INFO,
    MFRC522_LOG_DEBUG,
    MFRC522_LOG_TRACE
} MFRC522_LogLevel_t;

/**
 * @brief Debug sink. The core never calls printf(); it routes all diagnostic
 *        output through this callback so the application controls the sink
 *        (UART, ITM/SWO, SWD semihosting, a ring buffer, ...).
 */
typedef struct MFRC522_Debug
{
    void (*log)(void *context, MFRC522_LogLevel_t level, const char *message);
    void *context;               /**< Opaque user context passed back.         */
} MFRC522_Debug_t;

#ifdef __cplusplus
}
#endif

#endif /* MFRC522_TYPES_H */
