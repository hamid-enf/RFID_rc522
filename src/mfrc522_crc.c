/**
 * @file    mfrc522_crc.c
 * @brief   Software implementation of the ISO/IEC 14443-A CRC (CRC_A).
 *
 * The MFRC522 computes this CRC in hardware (see MFRC522_CalcCRC); this
 * bitwise implementation exists for host-side validation, for tests, and for
 * applications that need to verify a frame without touching the reader.
 *
 * CRC_A parameters (ISO/IEC 14443-3, 6.2.4):
 *   - polynomial: x^16 + x^12 + x^5 + 1  (0x1021)
 *   - initial value: 0x6363
 *   - the CRC is transmitted LSB-first on the air interface.
 *
 * Reference vector: CRC_A({0x50, 0x00}) == 0xCD57 (the MIFARE HALT command
 * is transmitted on air as 50 00 57 CD).
 */

#include "mfrc522_protocol.h"

void MFRC522_CRC_A(const uint8_t *data, uint32_t len, uint16_t *crc_out)
{
    uint16_t crc = 0x6363u;
    uint32_t i;

    if ((data == NULL) && (len != 0u)) {
        return;
    }

    for (i = 0u; i < len; i++) {
        uint8_t b = data[i];

        b = (uint8_t)(b ^ (uint8_t)(crc & 0xFFu));
        b = (uint8_t)(b ^ (uint8_t)(b << 4));

        crc = (uint16_t)((crc >> 8)
                         ^ ((uint16_t)b << 8)
                         ^ ((uint16_t)b << 3)
                         ^ ((uint16_t)b >> 4));
    }

    *crc_out = crc;
}
