/**
 * @file    mfrc522_registers.h
 * @brief   Complete register map of the NXP MFRC522.
 *
 * Addresses, reset values, bit fields and command codes follow the official
 * NXP MFRC522 product data sheet (Rev. 3.9, "Standard performance MIFARE and
 * NTAG frontend"). Where the datasheet and a third-party library disagree,
 * the datasheet is authoritative.
 *
 * IMPORTANT: the register addresses here are the RAW 6-bit register addresses
 * (0x01..0x3B). The SPI "address byte" is derived as (addr << 1) | R/W inside
 * the SPI transport; the I2C device frame and the UART LSB-first address byte
 * are likewise derived inside their respective transports. Do NOT pre-shift.
 *
 * Registers are grouped into pages of 8. Addresses marked "reserved" in the
 * datasheet must not be accessed.
 */

#ifndef MFRC522_REGISTERS_H
#define MFRC522_REGISTERS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================== */
/*  Register addresses (raw, 6-bit)                                   */
/* ================================================================== */

/* ---- Page 0: Command and status (0x00 reserved) ------------------ */
#define MFRC522_REG_COMMAND         (0x01u)  /**< Starts/stops command execution.   */
#define MFRC522_REG_COM_I_EN        (0x02u)  /**< ComIrqReg enable bits.            */
#define MFRC522_REG_DIV_I_EN        (0x03u)  /**< DivIrqReg enable bits.            */
#define MFRC522_REG_COM_IRQ         (0x04u)  /**< IRQ request bits.                 */
#define MFRC522_REG_DIV_IRQ         (0x05u)  /**< IRQ request bits (CRC/MFIN).      */
#define MFRC522_REG_ERROR           (0x06u)  /**< Error bits of the last command.   */
#define MFRC522_REG_STATUS1         (0x07u)  /**< Communication status bits.        */
#define MFRC522_REG_STATUS2         (0x08u)  /**< Receiver/transmitter status.      */
#define MFRC522_REG_FIFO_DATA       (0x09u)  /**< In/out of the 64-byte FIFO.       */
#define MFRC522_REG_FIFO_LEVEL      (0x0Au)  /**< Number of bytes stored in FIFO.   */
#define MFRC522_REG_WATER_LEVEL     (0x0Bu)  /**< FIFO under/overflow watermark.    */
#define MFRC522_REG_CONTROL         (0x0Cu)  /**< Miscellaneous control.            */
#define MFRC522_REG_BIT_FRAMING     (0x0Du)  /**< Bit-oriented frame adjustments.   */
#define MFRC522_REG_COLL            (0x0Eu)  /**< Bit position of first collision.  */

/* ---- Page 1: Command (0x10, 0x0F reserved) ----------------------- */
#define MFRC522_REG_MODE            (0x11u)  /**< General TX/RX modes.              */
#define MFRC522_REG_TX_MODE         (0x12u)  /**< TX data rate and framing.         */
#define MFRC522_REG_RX_MODE         (0x13u)  /**< RX data rate and framing.         */
#define MFRC522_REG_TX_CONTROL      (0x14u)  /**< Antenna driver pin control.       */
#define MFRC522_REG_TX_ASK          (0x15u)  /**< TX modulation control.            */
#define MFRC522_REG_TX_SEL          (0x16u)  /**< Internal TX sources.              */
#define MFRC522_REG_RX_SEL          (0x17u)  /**< Internal receiver settings.       */
#define MFRC522_REG_RX_THRESHOLD    (0x18u)  /**< Bit decoder thresholds.           */
#define MFRC522_REG_DEMOD           (0x19u)  /**< Demodulator settings.             */

/* ---- Page 1 (cont.): MIFARE / serial (0x1A, 0x1B, 0x1E reserved) - */
#define MFRC522_REG_MF_TX           (0x1Cu)  /**< MIFARE TX parameters.             */
#define MFRC522_REG_MF_RX           (0x1Du)  /**< MIFARE RX parameters.             */
#define MFRC522_REG_SERIAL_SPEED    (0x1Fu)  /**< Serial UART baud-rate selector.   */

/* ---- Page 2: CRC / config / timer (0x20, 0x23, 0x25 reserved) --- */
#define MFRC522_REG_CRC_RESULT_MSB  (0x21u)  /**< CRC result, high byte.            */
#define MFRC522_REG_CRC_RESULT_LSB  (0x22u)  /**< CRC result, low byte.             */
#define MFRC522_REG_MOD_WIDTH       (0x24u)  /**< ModWidth (modulation pulse width).*/
#define MFRC522_REG_RF_CFG          (0x26u)  /**< Receiver gain configuration.      */
#define MFRC522_REG_GS_N            (0x27u)  /**< TX conductance during modulation. */
#define MFRC522_REG_CW_GS_P         (0x28u)  /**< P-driver conductance (no mod).    */
#define MFRC522_REG_MOD_GS_P        (0x29u)  /**< P-driver conductance (modulation).*/
#define MFRC522_REG_T_MODE          (0x2Au)  /**< Timer mode (TAuto / prescaler hi).*/
#define MFRC522_REG_T_PRESCALER     (0x2Bu)  /**< Timer prescaler, low 8 bits.      */
#define MFRC522_REG_T_RELOAD_MSB    (0x2Cu)  /**< Timer reload value, high byte.    */
#define MFRC522_REG_T_RELOAD_LSB    (0x2Du)  /**< Timer reload value, low byte.     */
#define MFRC522_REG_T_COUNTER_MSB   (0x2Eu)  /**< Timer counter, high byte.         */
#define MFRC522_REG_T_COUNTER_LSB   (0x2Fu)  /**< Timer counter, low byte.          */

/* ---- Page 3: Test / version (0x30, 0x3C..0x3F reserved) ---------- */
#define MFRC522_REG_TEST_SEL1       (0x31u)  /**< Test signal configuration.        */
#define MFRC522_REG_TEST_SEL2       (0x32u)  /**< Test signal configuration.        */
#define MFRC522_REG_TEST_PIN_EN     (0x33u)  /**< Test pin output enables.          */
#define MFRC522_REG_TEST_PIN_VALUE  (0x34u)  /**< Test pin values.                  */
#define MFRC522_REG_TEST_BUS        (0x35u)  /**< Internal test bus status.         */
#define MFRC522_REG_AUTO_TEST       (0x36u)  /**< Digital self-test control.        */
#define MFRC522_REG_VERSION         (0x37u)  /**< Software version.                 */
#define MFRC522_REG_ANALOG_TEST     (0x38u)  /**< AUX1/AUX2 control.                */
#define MFRC522_REG_TEST_DAC1       (0x39u)  /**< TestDAC1 value.                   */
#define MFRC522_REG_TEST_DAC2       (0x3Au)  /**< TestDAC2 value.                   */
#define MFRC522_REG_TEST_ADC        (0x3Bu)  /**< ADC I/Q channel values.           */

/* ================================================================== */
/*  CommandReg (0x01) command codes                                   */
/* ================================================================== */
#define MFRC522_CMD_IDLE            (0x00u)  /**< No action / cancel current command. */
#define MFRC522_CMD_MEM             (0x01u)  /**< Store 25 bytes into internal buffer. */
#define MFRC522_CMD_GENERATE_RANDOM_ID (0x02u) /**< Generate a 10-byte random ID.  */
#define MFRC522_CMD_CALC_CRC        (0x03u)  /**< Activate the CRC coprocessor.    */
#define MFRC522_CMD_TRANSMIT        (0x04u)  /**< Transmit FIFO data.               */
#define MFRC522_CMD_NO_CMD_CHANGE   (0x05u)  /**< Command not changed.              */
#define MFRC522_CMD_RECEIVE         (0x06u)  /**< Activate the receiver.            */
#define MFRC522_CMD_TRANSCEIVE      (0x07u)  /**< Transmit, then switch to receive. */
#define MFRC522_CMD_MF_AUTHENT      (0x0Cu)  /**< MIFARE (Crypto1) authentication.  */
#define MFRC522_CMD_SOFT_RESET      (0x0Fu)  /**< Soft reset.                       */

/* ---- CommandReg control bits ------------------------------------- */
#define MFRC522_COMMAND_POWER_DOWN  (0x10u)  /**< bit 4: soft power-down.           */

/* ================================================================== */
/*  ComIrqReg (0x04) / ComIEnReg (0x02) bits                          */
/* ================================================================== */
#define MFRC522_IRQ_TIMER           (0x01u)  /**< Timer reached zero.               */
#define MFRC522_IRQ_ERR             (0x02u)  /**< Error (see ErrorReg).             */
#define MFRC522_IRQ_HI_ALERT        (0x04u)  /**< FIFO nearly full.                 */
#define MFRC522_IRQ_LO_ALERT        (0x08u)  /**< FIFO nearly empty.                */
#define MFRC522_IRQ_IDLE            (0x10u)  /**< Command finished (idle).          */
#define MFRC522_IRQ_RX              (0x20u)  /**< Data received.                    */
#define MFRC522_IRQ_TX              (0x40u)  /**< Data transmitted.                 */
#define MFRC522_IRQ_SET1            (0x80u)  /**< Set1 (also Status1Reg.Irq).       */

#define MFRC522_IRQ_ALL             (0x7Fu)  /**< All seven ComIrqReg IRQ bits.     */

/* ================================================================== */
/*  DivIrqReg (0x05) / DivIEnReg (0x03) bits                          */
/* ================================================================== */
#define MFRC522_DIV_IRQ_CRC          (0x04u) /**< CRC command completed.            */
#define MFRC522_DIV_IRQ_MFIN_ACT     (0x10u) /**< MFIN active.                      */
#define MFRC522_DIV_IRQ_SET2         (0x80u) /**< Set2.                             */

/* ================================================================== */
/*  ErrorReg (0x06) bits                                              */
/* ================================================================== */
#define MFRC522_ERR_PROT             (0x01u) /**< Protocol error.                   */
#define MFRC522_ERR_PARITY           (0x02u) /**< Parity error.                     */
#define MFRC522_ERR_CRC              (0x04u) /**< CRC error.                        */
#define MFRC522_ERR_COLL             (0x08u) /**< Collision error.                  */
#define MFRC522_ERR_BUFFER_OVFL      (0x10u) /**< FIFO buffer overflow.             */
#define MFRC522_ERR_TEMP             (0x40u) /**< Temperature error.                */
#define MFRC522_ERR_WR               (0x80u) /**< Write error.                      */

/* ---- Errors treated as fatal during a transceive ----------------- */
#define MFRC522_ERR_FATAL_MASK       (MFRC522_ERR_PROT | MFRC522_ERR_PARITY | \
                                      MFRC522_ERR_BUFFER_OVFL)

/* ================================================================== */
/*  Status1Reg (0x07) bits                                            */
/* ================================================================== */
#define MFRC522_STATUS1_LO_ALERT     (0x01u) /**< FIFO below LoAlert threshold.     */
#define MFRC522_STATUS1_HI_ALERT     (0x02u) /**< FIFO above HiAlert threshold.     */
#define MFRC522_STATUS1_IRQ          (0x10u) /**< Mirror of ComIrqReg.Set1.         */
#define MFRC522_STATUS1_T_RUNNING    (0x20u) /**< Timer running.                    */

/* ================================================================== */
/*  Status2Reg (0x08) bits                                            */
/* ================================================================== */
#define MFRC522_STATUS2_CRYPTO1_ON   (0x08u) /**< Crypto1 authentication active.    */
#define MFRC522_STATUS2_TEMP_SENS_CLR (0x80u) /**< Clear temperature error.         */

/* ================================================================== */
/*  FIFOLevelReg (0x0A) bits                                          */
/* ================================================================== */
#define MFRC522_FIFO_LEVEL_MASK      (0x7Fu) /**< FlushBuffer + FIFO level[6:0].    */
#define MFRC522_FIFO_LEVEL_FLUSH     (0x80u) /**< FlushBuffer: reset the FIFO.      */

/* ================================================================== */
/*  ControlReg (0x0C) bits                                            */
/* ================================================================== */
#define MFRC522_CONTROL_RX_LAST_BITS_MASK (0x07u) /**< RxLastBits[2:0].              */
#define MFRC522_CONTROL_FLUSH_FIFO   (0x08u) /**< Flush the FIFO (write-only).      */
#define MFRC522_CONTROL_T_START_NOW  (0x10u) /**< Start the timer immediately.      */
#define MFRC522_CONTROL_T_STOP_NOW   (0x20u) /**< Stop the timer immediately.       */

/* ================================================================== */
/*  BitFramingReg (0x0D) bit fields                                   */
/* ================================================================== */
#define MFRC522_BITFRAMING_START_SEND (0x80u) /**< StartSend: begin transmission.   */
#define MFRC522_BITFRAMING_TX_LAST_BITS_MASK (0x07u) /**< TxLastBits[2:0].           */
#define MFRC522_BITFRAMING_TX_LAST_BITS_POS  (0u)
#define MFRC522_BITFRAMING_RX_ALIGN_MASK     (0x70u) /**< RxAlign[2:0].              */
#define MFRC522_BITFRAMING_RX_ALIGN_POS      (4u)

/* ================================================================== */
/*  CollReg (0x0E) bit fields                                         */
/* ================================================================== */
#define MFRC522_COLL_POS_MASK        (0x1Fu) /**< CollPos[4:0] (0..31).             */
#define MFRC522_COLL_POS_NOT_VALID   (0x20u) /**< No collision occurred.            */
#define MFRC522_COLL_VALUES_AFTER_COLL (0x80u) /**< All bits after collision are 0. */

/* ================================================================== */
/*  TxControlReg (0x14) bits                                          */
/* ================================================================== */
#define MFRC522_TX_CONTROL_TX1_RF_EN (0x01u) /**< TX1 RF driver enabled.            */
#define MFRC522_TX_CONTROL_TX2_RF_EN (0x02u) /**< TX2 RF driver enabled.            */
#define MFRC522_TX_CONTROL_RF_EN_MASK (0x03u) /**< TX1+TX2 RF driver enables.       */

/* ================================================================== */
/*  TxASKReg (0x15) bits                                              */
/* ================================================================== */
#define MFRC522_TX_ASK_FORCE_100_ASK (0x40u) /**< Force 100% ASK modulation.        */

/* ================================================================== */
/*  ModeReg (0x11) / TxModeReg (0x12) / RxModeReg (0x13) fields       */
/* ================================================================== */
#define MFRC522_MODE_CRC_PRESET_6363 (0x02u) /**< ModeReg bit 1: 0 = CRC preset 0x6363. */
#define MFRC522_TX_MODE_CRC_EN       (0x80u) /**< TxModeReg bit 7: append CRC on TX. */
#define MFRC522_RX_MODE_CRC_EN       (0x80u) /**< RxModeReg bit 7: check CRC on RX.  */

/* ================================================================== */
/*  TModeReg (0x2A) bits                                              */
/* ================================================================== */
#define MFRC522_T_MODE_TAUTO         (0x80u) /**< Timer auto-start after TX.        */
#define MFRC522_T_MODE_TGATED_MASK   (0x30u) /**< TGated[1:0].                      */
#define MFRC522_T_MODE_TPRESCALER_HI_MASK (0x0Fu) /**< TPrescaler high nibble.       */

/* ================================================================== */
/*  VersionReg (0x37) known values                                    */
/* ================================================================== */
#define MFRC522_VERSION_REG_RESET    (0x92u) /**< Reset value: silicon v2.0.        */
#define MFRC522_VERSION_V2_0         (0x92u)
#define MFRC522_VERSION_V1_0         (0x91u)
#define MFRC522_VERSION_V0_0         (0x90u)
#define MFRC522_VERSION_FM17522      (0x88u) /**< Fudan FM17522 clone.              */
#define MFRC522_VERSION_B2           (0xB2u) /**< MFRC522-compatible clone silicon
                                                 reporting 0xB2 (field-verified
                                                 functional).                    */

#ifdef __cplusplus
}
#endif

#endif /* MFRC522_REGISTERS_H */
