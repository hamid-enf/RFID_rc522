/**
 * @file    mfrc522.h
 * @brief   Top-level public API of the MFRC522 driver.
 *
 * Include this single header to use the whole library:
 *
 *     #include "mfrc522.h"
 *
 * The header pulls in the type, register, transport, protocol and MIFARE
 * layers. Typical usage:
 *
 *     MFRC522_Handle_t rfid;
 *     MFRC522_STM32_SPI_Init(&rfid, &hspi1, CS_GPIO_Port, CS_Pin,
 *                            RST_GPIO_Port, RST_Pin);
 *     if (MFRC522_Init(&rfid) != MFRC522_OK) { handle error }
 *     while (1) { if (MFRC522_IsCardPresent(&rfid) == MFRC522_OK) { ... } }
 */

#ifndef MFRC522_H
#define MFRC522_H

#include <stdint.h>
#include "mfrc522_config.h"
#include "mfrc522_types.h"
#include "mfrc522_registers.h"
#include "mfrc522_transport.h"
#include "mfrc522_protocol.h"
#include "mfrc522_mifare.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================== */
/*  IRQ callback type (forward: used by the runtime state)           */
/* ================================================================== */

#if MFRC522_ENABLE_IRQ
/** IRQ callback signature (called from MFRC522_ProcessIRQ). */
typedef void (*MFRC522_IrqCallback_t)(MFRC522_Handle_t *handle,
                                      uint8_t irq_source, void *user);
#endif

/* ================================================================== */
/*  Runtime state                                                    */
/* ================================================================== */

/** Handle state flags. */
#define MFRC522_FLAG_INITIALIZED   (0x01u)
#define MFRC522_FLAG_ANTENNA_ON    (0x02u)

/** Async operation identifiers (non-blocking mode). */
typedef enum MFRC522_AsyncOp
{
    MFRC522_ASYNC_NONE = 0,
    MFRC522_ASYNC_READ_UID,
    MFRC522_ASYNC_READ_BLOCK,
    MFRC522_ASYNC_WRITE_BLOCK,
    MFRC522_ASYNC_COUNT
} MFRC522_AsyncOp_t;

/** Internal async state machine context (non-blocking mode only). */
typedef struct MFRC522_AsyncState
{
    MFRC522_AsyncOp_t op;        /**< Operation being executed.         */
    uint8_t  step;               /**< Current step of the state machine.*/
    uint32_t deadline_tick;      /**< Absolute ms tick for the timeout. */
    MFRC522_Status_t result;     /**< Final/current result.             */
    uint8_t  buf[MFRC522_BLOCK_SIZE + 8]; /**< Scratch (UID / block).   */
    uint8_t  buf_len;            /**< Valid bytes in buf.               */
    uint8_t  block;              /**< Block index (read/write).         */
    uint8_t  key_type;           /**< MFRC522_KeyType_t (auth).         */
} MFRC522_AsyncState_t;

/**
 * @brief Per-reader runtime state (all inside the handle: no globals).
 */
typedef struct MFRC522_State
{
    uint8_t  version_raw;        /**< Raw VersionReg value.             */
    uint8_t  flags;              /**< MFRC522_FLAG_* bits.             */
    uint8_t  reserved[2];
    MFRC522_Status_t last_error; /**< Last error code (for diagnostics). */
#if MFRC522_ENABLE_IRQ
    MFRC522_IrqCallback_t irq_callback; /**< Registered IRQ callback.   */
    void                 *irq_user;     /**< Callback user context.     */
#endif
#if MFRC522_ENABLE_NONBLOCKING
    MFRC522_AsyncState_t async;  /**< Non-blocking state machine.      */
#endif
} MFRC522_State_t;

/* ================================================================== */
/*  Reader handle                                                    */
/* ================================================================== */

/**
 * @brief Complete per-reader object. Allocate one per physical MFRC522.
 *
 * The handle owns no heap memory: every byte it needs lives inside this
 * struct (including the opaque platform storage used by the HAL adapter).
 * Initialize to zero ( `= {0}` ) before first use.
 */
struct MFRC522_Handle
{
    MFRC522_Transport_t        transport;      /**< Selected interface + params. */
    const MFRC522_TransportOps_t *transport_ops; /**< Framing function table.     */
    MFRC522_Platform_t         platform;       /**< MCU abstraction (ops + ctx). */
    MFRC522_Config_t           config;         /**< Runtime configuration copy.  */
    MFRC522_State_t            state;          /**< Runtime state.               */
    const MFRC522_Debug_t     *debug;          /**< Optional debug sink.         */

    /** Opaque, aligned storage for the platform adapter's private context. */
    union
    {
        uint8_t  bytes[MFRC522_PLATFORM_CTX_SIZE];
        void    *ptr_align;
        uint64_t u64_align;
    } platform_storage;
};

/* ================================================================== */
/*  Initialization / version / self-test                             */
/* ================================================================== */

/**
 * @brief Initialize the reader: hard+soft reset, version check, register and
 *        timer/FIFO/antenna configuration, antenna on.
 *
 * @param handle  Handle whose `transport`/`platform` were already configured
 *                by a platform adapter (e.g. MFRC522_STM32_SPI_Init).
 * @return        MFRC522_OK on success.
 */
MFRC522_Status_t MFRC522_Init(MFRC522_Handle_t *handle);

/**
 * @brief Put the reader into a clean, low-power state and release resources.
 */
MFRC522_Status_t MFRC522_Deinit(MFRC522_Handle_t *handle);

/**
 * @brief Perform a software reset (CommandReg = SoftReset) and wait for it.
 */
MFRC522_Status_t MFRC522_SoftReset(MFRC522_Handle_t *handle);

/**
 * @brief Perform a hardware reset using the platform NRSTPD GPIO.
 */
MFRC522_Status_t MFRC522_HardReset(MFRC522_Handle_t *handle);

/**
 * @brief Read and decode the VersionReg (0x30).
 * @param version  Receives raw + decoded version.
 */
MFRC522_Status_t MFRC522_GetVersion(MFRC522_Handle_t *handle,
                                    MFRC522_Version_t *version);

/**
 * @brief Run the MFRC522 digital self-test and return a pass/fail verdict.
 */
MFRC522_Status_t MFRC522_SelfTest(MFRC522_Handle_t *handle);

/* ================================================================== */
/*  Register / FIFO API                                              */
/* ================================================================== */

MFRC522_Status_t MFRC522_ReadRegister(MFRC522_Handle_t *handle,
                                      uint8_t addr, uint8_t *value);
MFRC522_Status_t MFRC522_WriteRegister(MFRC522_Handle_t *handle,
                                       uint8_t addr, uint8_t value);
MFRC522_Status_t MFRC522_SetBits(MFRC522_Handle_t *handle,
                                 uint8_t addr, uint8_t mask);
MFRC522_Status_t MFRC522_ClearBits(MFRC522_Handle_t *handle,
                                   uint8_t addr, uint8_t mask);
MFRC522_Status_t MFRC522_ReadFIFO(MFRC522_Handle_t *handle,
                                  uint8_t *data, uint32_t len);
MFRC522_Status_t MFRC522_WriteFIFO(MFRC522_Handle_t *handle,
                                   const uint8_t *data, uint32_t len);

/**
 * @brief Use the MFRC522 CRC coprocessor to compute CRC_A over a buffer.
 * @param crc  Receives the 16-bit result (host byte order).
 */
MFRC522_Status_t MFRC522_CalcCRC(MFRC522_Handle_t *handle,
                                 const uint8_t *data, uint32_t len,
                                 uint16_t *crc);

/* ================================================================== */
/*  RF / antenna control                                             */
/* ================================================================== */

MFRC522_Status_t MFRC522_AntennaOn(MFRC522_Handle_t *handle);
MFRC522_Status_t MFRC522_AntennaOff(MFRC522_Handle_t *handle);
uint8_t         MFRC522_IsAntennaOn(const MFRC522_Handle_t *handle);

/* ================================================================== */
/*  Card detection                                                   */
/* ================================================================== */

/**
 * @brief Non-blocking presence check (REQA + short timeout).
 * @return MFRC522_OK if a card answered, MFRC522_ERR_NO_CARD otherwise.
 */
MFRC522_Status_t MFRC522_IsCardPresent(MFRC522_Handle_t *handle);

/**
 * @brief Block until a card appears or the timeout expires.
 */
MFRC522_Status_t MFRC522_WaitForCard(MFRC522_Handle_t *handle,
                                     uint32_t timeout_ms);

/* ================================================================== */
/*  UID / card info                                                  */
/* ================================================================== */

/**
 * @brief Fully resolve the card UID (all cascade levels) and its SAK.
 */
MFRC522_Status_t MFRC522_ReadUID(MFRC522_Handle_t *handle,
                                 MFRC522_UID_t *uid);

/**
 * @brief Gather ATQA, SAK, UID and the derived card type.
 */
MFRC522_Status_t MFRC522_GetCardInfo(MFRC522_Handle_t *handle,
                                     MFRC522_CardInfo_t *info);

/* ================================================================== */
/*  Non-blocking API                                                 */
/* ================================================================== */

#if MFRC522_ENABLE_NONBLOCKING
/**
 * @brief Start a non-blocking UID read. Poll with MFRC522_Process().
 */
MFRC522_Status_t MFRC522_StartReadUID(MFRC522_Handle_t *handle);

/**
 * @brief Advance any in-flight non-blocking operation (call from main loop
 *        or an RTOS task). Returns MFRC522_ERR_BUSY while still running.
 */
MFRC522_Status_t MFRC522_Process(MFRC522_Handle_t *handle);

/**
 * @brief True when the last started operation finished.
 * @param result  Receives the final status (may be NULL).
 */
uint8_t MFRC522_IsOperationComplete(const MFRC522_Handle_t *handle,
                                    MFRC522_Status_t *result);
#endif

/* ================================================================== */
/*  IRQ                                                              */
/* ================================================================== */

#if MFRC522_ENABLE_IRQ
/**
 * @brief Register an application callback for reader interrupts.
 */
MFRC522_Status_t MFRC522_AttachIRQCallback(MFRC522_Handle_t *handle,
                                           MFRC522_IrqCallback_t callback,
                                           void *user);

/**
 * @brief Service pending reader interrupts. Reads ComIrqReg, clears the
 *        latched bits and dispatches to the registered callback with a
 *        bit-mask of the pending sources.
 *
 * Call from the GPIO EXTI handler (via the platform adapter) or poll it from
 * the main loop.
 */
MFRC522_Status_t MFRC522_ProcessIRQ(MFRC522_Handle_t *handle);
#endif

/* ================================================================== */
/*  Low power                                                        */
/* ================================================================== */

/**
 * @brief Enter the MFRC522 soft power-down mode (CommandReg PowerDown bit).
 *        The antenna is switched off; wake-up clears the bit.
 */
MFRC522_Status_t MFRC522_Sleep(MFRC522_Handle_t *handle);

/**
 * @brief Wake the MFRC522 from soft power-down.
 */
MFRC522_Status_t MFRC522_WakeUp(MFRC522_Handle_t *handle);

/**
 * @brief Full power-down via the NRSTPD pin (lowest consumption).
 *        The reader must be re-initialized after this.
 */
MFRC522_Status_t MFRC522_PowerDown(MFRC522_Handle_t *handle);

/* ================================================================== */
/*  Debug / utility                                                  */
/* ================================================================== */

/**
 * @brief Attach a debug sink (may be detached by passing NULL).
 */
void MFRC522_AttachDebug(MFRC522_Handle_t *handle,
                         const MFRC522_Debug_t *debug);

/**
 * @brief Return the last error recorded in the handle.
 */
MFRC522_Status_t MFRC522_GetLastError(const MFRC522_Handle_t *handle);

/**
 * @brief Human-readable name of a status code (static string).
 */
const char *MFRC522_StatusToString(MFRC522_Status_t status);

#ifdef __cplusplus
}
#endif

#endif /* MFRC522_H */
