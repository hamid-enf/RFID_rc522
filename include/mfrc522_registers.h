/**
 * @file    mfrc522_registers.h
 * @brief   Complete register map of the NXP MFRC522.
 *
 * Addresses, reset values, bit fields and command codes follow the official
 * NXP MFRC522 product data sheet (Rev. 3.9, "Standard performance MIFARE and
 * NTAG frontend"). Where the datasheet and a third-party library disagree,
 * the datasheet is authoritative.
 *
 * Registers are grouped into 8 pages of 8 registers (0x00..0x3F). Only
 * registers listed here are implemented by the silicon; access to reserved
 * registers is undefined and must not be performed.
 */

#ifndef MFRC522_REGISTERS_H
#define MFRC522_REGISTERS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================== */
/*  Register addresses                                                */
/* ================================================================== */

/* ---- Page 0: Command and status ---------------------------------- */
#define MFRC522_REG_COMMAND         (0x00u)  /**< Starts/stops command execution.   */
#define MFRC522_REG_COM_I_EN        (0x01u)  /**< Enable/disable IRQ request control bits. */
#define MFRC522_REG_DIV_I_EN        (0x02u)  /**< Enable/disable IRQ request control bits. */
#define MFRC522_REG_COM_IRQ         (0x03u)  /**< IRQ request bits.                 */
#define MFRC522_REG_DIV_IRQ         (0x04u)  /**< IRQ request bits.                 */
#define MFRC522_REG_ERROR           (0x05u)  /**< Error bits showing error status.  */
#define MFRC522_REG_STATUS1         (0x06u)  /**< Communication status bits.        */
#define MFRC522_REG_STATUS2         (0x07u)  /**< Receiver and transmitter status.  */
#define MFRC522_REG_FIFO_DATA       (0x08u)  /**< Input/output of 64-byte FIFO.     */
#define MFRC522_REG_FIFO_LEVEL      (0x09u)  /**< Number of bytes stored in FIFO.   */
#define MFRC522_REG_WATER_LEVEL     (0x0Au)  /**< Level for FIFO under/overflow.    */
#define MFRC522_REG_CONTROL         (0x0Bu)  /**< Miscellaneous control register.   */
#define MFRC522_REG_BIT_FRAMING     (0x0Cu)  /**< Adjustments for bit-oriented frames. */
#define MFRC522_REG_COLL            (0x0Du)  /**< Bit position of first bit-collision. */

/* ---- Page 1: Command --------------------------------------------- */
#define MFRC522_REG_MODE            (0x10u)  /**< Defines general modes.            */
#define MFRC522_REG_TX_MODE         (0x11u)  /**< Defines transmission data rate/framing. */
#define MFRC522_REG_RX_MODE         (0x12u)  /**< Defines reception data rate/framing. */
#define MFRC522_REG_TX_CONTROL      (0x13u)  /**< Controls antenna driver pins.     */
#define MFRC522_REG_TX_ASK          (0x14u)  /**< Controls TX modulation.           */
#define MFRC522_REG_TX_SEL          (0x15u)  /**< Selects internal sources for antenna driver. */
#define MFRC522_REG_RX_SEL          (0x16u)  /**< Selects internal receiver settings. */
#define MFRC522_REG_RX_THRESHOLD    (0x17u)  /**< Selects thresholds for bit decoder. */
#define MFRC522_REG_DEMOD           (0x18u)  /**< Defines demodulator settings.      */
#define MFRC522_REG_MF_TX           (0x19u)  /**< Controls some MIFARE communication TX parameters. */
#define MFRC522_REG_MF_RX           (0x1Au)  /**< Controls some MIFARE communication RX parameters. */
#define MFRC522_REG_SERIAL_SPEED    (0x1Bu)  /**< Selects speed of the serial UART interface. */
#define MFRC522_REG_CRC_RESULT_MSB  (0x1Cu)  /**< (reserved on MFRC522) CRC result MSB. */
#define MFRC522_REG_CRC_RESULT_LSB  (0x1Du)  /**< (reserved on MFRC522) CRC result LSB. */
#define MFRC522_REG_MOD_WIDTH       (0x1Eu)  /**< Controls the ModWidth setting.     */

/* ---- Page 2: CRC / RX / TX / Timer ------------------------------- */
#define MFRC522_REG_RF_CFG          (0x20u)  /**< (reserved on MFRC522) RF configuration. */
#define MFRC522_REG_T_MODE          (0x21u)  /**< Defines settings for the internal timer. */
#define MFRC522_REG_T_PRESCALER     (0x22u)  /**< Defines prescaler of the internal timer. */
#define MFRC522_REG_T_RELOAD_MSB    (0x23u)  /**< Defines 16-bit timer reload value (MSB). */
#define MFRC522_REG_T_RELOAD_LSB    (0x24u)  /**< Defines 16-bit timer reload value (LSB). */
#define MFRC522_REG_T_COUNTER_MSB   (0x25u)  /**< 16-bit timer value (MSB).         */
#define MFRC522_REG_T_COUNTER_LSB   (0x26u)  /**< 16-bit timer value (LSB).         */

/* ---- Page 3: Test / version -------------------------------------- */
#define MFRC522_REG_TEST_SEL1       (0x2Au)  /**< General test signal configuration. */
#define MFRC522_REG_TEST_SEL2       (0x2Bu)  /**< General test signal configuration. */
#define MFRC522_REG_TEST_PIN_EN     (0x2Cu)  /**< Enables pin output driver.         */
#define MFRC522_REG_TEST_PIN_VALUE  (0x2Du)  /**< Defines the value of the D1..D7 pins. */
#define MFRC522_REG_TEST_BUS        (0x2Eu)  /**< Shows the status of the internal test bus. */
#define MFRC522_REG_AUTO_TEST       (0x2Fu)  /**< Controls the digital self-test.    */
#define MFRC522_REG_VERSION         (0x30u)  /**< Shows the software version.        */
#define MFRC522_REG_ANALOG_TEST     (0x31u)  /**< Controls the pins AUX1 and AUX2.   */
#define MFRC522_REG_TEST_DAC1       (0x32u)  /**< Defines the test value for TestDAC1. */
#define MFRC522_REG_TEST_DAC2       (0x33u)  /**< Defines the test value for TestDAC2. */
#define MFRC522_REG_TEST_ADC        (0x34u)  /**< Shows the value of ADC I and Q channels. */

/* ================================================================== */
/*  CommandReg (0x00) command codes                                  */
/* ================================================================== */
#define MFRC522_CMD_IDLE            (0x00u)  /**< Cancels current command.          */
#define MFRC522_CMD_MEM             (0x01u)  /**< Stores 25 bytes into the internal buffer. */
#define MFRC522_CMD_GENERATE_RANDOM_ID (0x02u) /**< Generates a 10-byte random ID. */
#define MFRC522_CMD_CALC_CRC        (0x03u)  /**< Activates the CRC coprocessor.    */
#define MFRC522_CMD_TRANSMIT        (0x04u)  /**< Transmits data from the FIFO.     */
#define MFRC522_CMD_NO_CMD_CHANGE   (0x05u)  /**< No command change (command not altered). */
#define MFRC522_CMD_RECEIVE         (0x06u)  /**< Activates the receiver.           */
#define MFRC522_CMD_TRANSCEIVE      (0x07u)  /**< Transmits then switches to receive. */
#define MFRC522_CMD_MF_AUTHENT      (0x0Cu)  /**< Performs MIFARE authentication.   */
#define MFRC522_CMD_SOFT_RESET      (0x0Fu)  /**< Performs a soft reset.            */

/* ================================================================== */
/*  ComIrqReg (0x03) / ComIEnReg (0x01) bits                         */
/* ================================================================== */
#define MFRC522_IRQ_TIMER           (0x01u)  /**< Timer has reached zero.           */
#define MFRC522_IRQ_ERR             (0x02u)  /**< Error bit (see ErrorReg).         */
#define MFRC522_IRQ_HI_ALERT        (0x04u)  /**< FIFO nearly full (HiAlert).       */
#define MFRC522_IRQ_LO_ALERT        (0x08u)  /**< FIFO nearly empty (LoAlert).      */
#define MFRC522_IRQ_IDLE            (0x10u)  /**< Command finished (idle).          */
#define MFRC522_IRQ_RX              (0x20u)  /**< Data received.                    */
#define MFRC522_IRQ_TX              (0x40u)  /**< Data transmitted.                 */
#define MFRC522_IRQ_SET1            (0x80u)  /**< Set1 (also sets Status1Reg.Irq).  */

/* ================================================================== */
/*  DivIrqReg (0x04) / DivIEnReg (0x02) bits                         */
/* ================================================================== */
#define MFRC522_DIV_IRQ_CRC          (0x01u) /**< CRC command completed.            */
#define MFRC522_DIV_IRQ_MFIN_ACT     (0x04u) /**< MFIN active.                      */
#define MFRC522_DIV_IRQ_SET2         (0x80u) /**< Set2.                             */

/* ================================================================== */
/*  ErrorReg (0x05) bits                                             */
/* ================================================================== */
#define MFRC522_ERR_PROT             (0x01u) /**< Protocol error.                   */
#define MFRC522_ERR_PARITY           (0x02u) /**< Parity error.                     */
#define MFRC522_ERR_CRC              (0x04u) /**< CRC error.                        */
#define MFRC522_ERR_COLL             (0x08u) /**< Collision error.                  */
#define MFRC522_ERR_BUFFER_OVFL      (0x10u) /**< FIFO buffer overflow.             */
#define MFRC522_ERR_TEMP             (0x40u) /**< Temperature error (if supported). */
#define MFRC522_ERR_WR               (0x80u) /**< Write error.                      */

/* ================================================================== */
/*  Status1Reg (0x06) bits                                           */
/* ================================================================== */
#define MFRC522_STATUS1_LO_ALERT     (0x01u) /**< FIFO below LoAlert threshold.     */
#define MFRC522_STATUS1_HI_ALERT     (0x02u) /**< FIFO above HiAlert threshold.     */
#define MFRC522_STATUS1_IRQ          (0x10u) /**< Mirror of ComIrqReg.Set1.         */
#define MFRC522_STATUS1_T_RUNNING    (0x20u) /**< Timer running.                    */

/* ================================================================== */
/*  Status2Reg (0x07) bits                                           */
/* ================================================================== */
#define MFRC522_STATUS2_MODEM_STATE_MASK (0x07u) /**< ModemState[2:0].              */
#define MFRC522_STATUS2_CRYPTO1_ON   (0x08u) /**< Crypto1 authentication active.    */
#define MFRC522_STATUS2_TEMP_SENS_CLR (0x80u) /**< Clear temperature error.         */

/* ================================================================== */
/*  ControlReg (0x0B) bits                                           */
/* ================================================================== */
#define MFRC522_CONTROL_FLUSH_FIFO   (0x08u) /**< Flush the FIFO (write-only).      */
#define MFRC522_CONTROL_T_START_NOW  (0x10u) /**< Start the timer immediately.      */
#define MFRC522_CONTROL_T_STOP_NOW   (0x20u) /**< Stop the timer immediately.       */

/* ================================================================== */
/*  BitFramingReg (0x0C) bit fields                                  */
/* ================================================================== */
#define MFRC522_BITFRAMING_TX_LAST_BITS_MASK (0x07u) /**< TxLastBits[2:0].           */
#define MFRC522_BITFRAMING_TX_LAST_BITS_POS  (0u)
#define MFRC522_BITFRAMING_RX_ALIGN_MASK     (0x70u) /**< RxAlign[2:0].              */
#define MFRC522_BITFRAMING_RX_ALIGN_POS      (4u)

/* ================================================================== */
/*  CollReg (0x0D) bit fields                                        */
/* ================================================================== */
#define MFRC522_COLL_POS_MASK        (0x1Fu) /**< CollPos[4:0] (0..31).             */
#define MFRC522_COLL_POS_NOT_VALID   (0x20u) /**< No collision occurred.            */
#define MFRC522_COLL_VALUES_AFTER_COLL (0x80u) /**< All bits after collision are 0. */

/* ================================================================== */
/*  TxControlReg (0x13) bits                                         */
/* ================================================================== */
#define MFRC522_TX_CONTROL_TX1_RF_EN (0x10u) /**< TX1 RF driver enabled.            */
#define MFRC522_TX_CONTROL_TX2_RF_EN (0x20u) /**< TX2 RF driver enabled.            */

/* ================================================================== */
/*  TxASKReg (0x14) bits                                             */
/* ================================================================== */
#define MFRC522_TX_ASK_FORCE_100_ASK (0x40u) /**< Force 100% ASK modulation.        */

/* ================================================================== */
/*  TxModeReg (0x11) / RxModeReg (0x12) fields                       */
/* ================================================================== */
#define MFRC522_MODE_CRC_EN          (0x80u) /**< CRC enabled on TX/RX.             */

/* ================================================================== */
/*  VersionReg (0x30) values                                         */
/* ================================================================== */
#define MFRC522_VERSION_REG_RESET    (0x92u) /**< Reset value: silicon v2.0.        */

#ifdef __cplusplus
}
#endif

#endif /* MFRC522_REGISTERS_H */
