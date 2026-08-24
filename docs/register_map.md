# MFRC522 register map

The authoritative definition lives in
[`include/mfrc522_registers.h`](../include/mfrc522_registers.h). Addresses,
reset values, bit fields and command codes follow the NXP MFRC522 product data
sheet (Rev. 3.9). This page is a quick cross-reference.

> The register addresses below are the **raw 6-bit addresses** as listed in the
> datasheet. Host-interface framing (SPI `<<1` + mode bit, I2C device frame,
> UART LSB-first address byte) is applied inside the transports — never in the
> register definitions.

## Register overview

| Address | Register        | Function                          |
|--------:|-----------------|-----------------------------------|
| 0x01    | CommandReg      | Command execution / power-down    |
| 0x02    | ComIEnReg       | ComIrqReg enable mask             |
| 0x03    | DivIEnReg       | DivIrqReg enable mask             |
| 0x04    | ComIrqReg       | IRQ request flags                 |
| 0x05    | DivIrqReg       | IRQ flags (CRC, MFIN)             |
| 0x06    | ErrorReg        | Error status of the last command  |
| 0x07    | Status1Reg      | Communication status              |
| 0x08    | Status2Reg      | Receiver/transmitter status       |
| 0x09    | FIFODataReg     | 64-byte FIFO window               |
| 0x0A    | FIFOLevelReg    | FIFO byte count / flush           |
| 0x0B    | WaterLevelReg   | FIFO watermark                    |
| 0x0C    | ControlReg      | RxLastBits / flush / timer start  |
| 0x0D    | BitFramingReg   | StartSend / TxLastBits / RxAlign  |
| 0x0E    | CollReg         | Collision position                |
| 0x11    | ModeReg         | General modes / CRC preset        |
| 0x12    | TxModeReg       | TX data rate / CRC / framing      |
| 0x13    | RxModeReg       | RX data rate / CRC / framing      |
| 0x14    | TxControlReg    | Antenna driver (TX1/TX2) enables  |
| 0x15    | TxASKReg        | Modulation (100% ASK)             |
| 0x16    | TxSelReg        | Internal TX sources               |
| 0x17    | RxSelReg        | Internal receiver settings        |
| 0x18    | RxThresholdReg  | Bit decoder thresholds            |
| 0x19    | DemodReg        | Demodulator settings              |
| 0x1C    | MfTxReg         | MIFARE TX parameters              |
| 0x1D    | MfRxReg         | MIFARE RX parameters              |
| 0x1F    | SerialSpeedReg  | UART baud-rate divisor            |
| 0x21    | CRCResultRegH   | CRC result, high byte             |
| 0x22    | CRCResultRegL   | CRC result, low byte              |
| 0x24    | ModWidthReg     | Modulation pulse width            |
| 0x26    | RFCfgReg        | Receiver gain                     |
| 0x27    | GsNReg          | TX conductance (modulation)       |
| 0x28    | CWGsPReg        | P-driver conductance (no mod)     |
| 0x29    | ModGsPReg       | P-driver conductance (modulation) |
| 0x2A    | TModeReg        | Timer mode / TAuto / prescaler hi |
| 0x2B    | TPrescalerReg   | Timer prescaler (low 8 bits)      |
| 0x2C    | TReloadRegH     | Timer reload, high byte           |
| 0x2D    | TReloadRegL     | Timer reload, low byte            |
| 0x2E    | TCounterValRegH | Timer counter, high byte          |
| 0x2F    | TCounterValRegL | Timer counter, low byte           |
| 0x31    | TestSel1Reg     | Test signal config                |
| 0x32    | TestSel2Reg     | Test signal config                |
| 0x33    | TestPinEnReg    | Test pin enables                  |
| 0x34    | TestPinValueReg | Test pin values                   |
| 0x35    | TestBusReg      | Internal test bus                 |
| 0x36    | AutoTestReg     | Digital self-test                 |
| 0x37    | VersionReg      | Firmware version (0x92 = v2.0)    |
| 0x38    | AnalogTestReg   | AUX1/AUX2 control                 |
| 0x39    | TestDAC1Reg     | TestDAC1 value                    |
| 0x3A    | TestDAC2Reg     | TestDAC2 value                    |
| 0x3B    | TestADCReg      | ADC I/Q values                    |

## Command codes (CommandReg)

| Value | Name              | Purpose                          |
|------:|-------------------|----------------------------------|
| 0x00  | Idle              | No action / cancel command       |
| 0x01  | Mem               | Store 25 bytes to internal buffer|
| 0x02  | GenerateRandomID  | 10-byte random ID                |
| 0x03  | CalcCRC           | CRC coprocessor                  |
| 0x04  | Transmit          | Transmit FIFO content            |
| 0x05  | NoCmdChange       | Command unchanged                |
| 0x06  | Receive           | Activate receiver                |
| 0x07  | Transceive        | Transmit then receive            |
| 0x0C  | MFAuthent         | MIFARE Crypto1 authentication    |
| 0x0F  | SoftReset         | Soft reset                       |

## Notes

- **FIFODataReg** (0x09) is a window register: sequential reads pop and
  sequential writes push; it does *not* auto-increment.
- **ComIrqReg** (0x04) bits: `TimerIRq=0x01, ErrIRq=0x02, HiAlert=0x04,
  LoAlert=0x08, IdleIRq=0x10, RxIRq=0x20, TxIRq=0x40, Set1=0x80`.
- **DivIrqReg** (0x05) bits: `CRCIRq=0x04, MfinActIRq=0x10, Set2=0x80`.
- **ErrorReg** (0x06) bits: `Prot=0x01, Parity=0x02, CRC=0x04, Coll=0x08,
  BufferOvfl=0x10, Temp=0x40, Wr=0x80`.
- **TxControlReg** (0x14): `Tx1RFEn=0x01, Tx2RFEn=0x02` (both set = antenna on).
