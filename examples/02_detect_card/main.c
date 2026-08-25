/**
 * @file    main.c
 * @brief   Example 02 — card detection.
 *
 * Polls MFRC522_IsCardPresent() (a short, bounded check) and also shows
 * MFRC522_WaitForCard() (blocking until a card appears or the timeout
 * expires). When no card is seen, a command sweep probes the chip at the
 * register level: it soft-resets the reader (bypassing the driver's init
 * writes), tries every plausible CommandReg code with a REQA byte in the
 * FIFO, and reports which code consumes the byte / starts a transceive.
 *
 * Field-verified triage (0xB2 clone silicon): the driver's transceive
 * sequence left the REQA byte in the FIFO and raised only HiAlert — the
 * command never executed. The sweep separates three worlds:
 *   A. a different command code consumes the FIFO  -> clone command table
 *   B. 0x0C consumes it after a plain reset        -> the driver's init
 *      writes (NXP values) put this clone into a bad state
 *   C. even 0x04 (Transmit, positive control) does not consume the FIFO
 *      -> the chip cannot execute commands at all (power/oscillator)
 */

#include "mfrc522_demo.h"

MFRC522_Handle_t rfid = {0};

/**
 * @brief One raw probe: put `req_byte` in the FIFO, start `cmd_code` with
 *        StartSend, wait up to 50 ms for Rx/Idle/Timer, then report the
 *        FIFO (was the byte consumed?), ComIrq, Error, Status1/2 and the
 *        CommandReg value.
 */
static void demo_probe_cmd(uint8_t cmd_code, uint8_t req_byte)
{
    uint8_t irq = 0u, level = 0u, val = 0u, err = 0u, s1 = 0u, s2 = 0u,
           creg = 0u, i;
    uint32_t t0;

    MFRC522_WriteRegister(&rfid, 0x01u, 0x00u);      /* Idle            */
    MFRC522_WriteRegister(&rfid, 0x04u, 0x7Fu);      /* clear ComIrq    */
    MFRC522_WriteRegister(&rfid, 0x0Au, 0x80u);      /* flush FIFO      */
    MFRC522_WriteRegister(&rfid, 0x09u, req_byte);   /* FIFO <- request */
    MFRC522_WriteRegister(&rfid, 0x0Cu, 0x07u);      /* TxLastBits = 7  */
    MFRC522_WriteRegister(&rfid, 0x01u, cmd_code);   /* start command   */
    MFRC522_SetBits(&rfid, 0x0Cu, 0x80u);            /* StartSend       */

    t0 = HAL_GetTick();
    while ((uint32_t)(HAL_GetTick() - t0) < 50u) {
        MFRC522_ReadRegister(&rfid, 0x04u, &irq);
        if ((irq & 0x31u) != 0u) {
            break;                                   /* Rx|Idle|Timer   */
        }
    }

    MFRC522_ReadRegister(&rfid, 0x0Au, &level);
    level = (uint8_t)(level & 0x7Fu);
    MFRC522_ReadRegister(&rfid, 0x06u, &err);
    MFRC522_ReadRegister(&rfid, 0x07u, &s1);
    MFRC522_ReadRegister(&rfid, 0x08u, &s2);
    MFRC522_ReadRegister(&rfid, 0x01u, &creg);
    MFRC522_WriteRegister(&rfid, 0x01u, 0x00u);      /* Idle            */
    MFRC522_WriteRegister(&rfid, 0x04u, 0x7Fu);      /* clear ComIrq    */

    demo_printf("cmd=0x%02X: FIFO=%u %s IRQ=0x%02X Err=0x%02X S1=0x%02X "
                "S2=0x%02X Cmd=0x%02X",
                cmd_code, level, (level == 1u) ? "(NOT consumed)" : "",
                (unsigned int)(irq & 0x7Fu), (unsigned int)err,
                (unsigned int)s1, (unsigned int)s2, (unsigned int)creg);
    for (i = 0u; i < level && i < 8u; i++) {
        MFRC522_ReadRegister(&rfid, 0x09u, &val);
        demo_printf(" [0x%02X]", val);
    }
    demo_printf("\n");
}

/**
 * @brief Card not seen by the driver: sweep the CommandReg codes.
 *
 * Runs a plain soft reset first (NOT the driver's full init — its NXP
 * register writes may be exactly what puts this clone into a bad state).
 * 0x04 (Transmit) is the positive control: it must consume the FIFO byte
 * on any command-capable silicon.
 */
static void demo_cmd_sweep(void)
{
    static const uint8_t cmds[] =
        { 0x0Cu, 0x0Du, 0x0Eu, 0x0Bu, 0x0Au, 0x09u, 0x08u, 0x04u };
    uint8_t i, w1c;

    demo_printf("\n=== command sweep: hold the card on the antenna ===\n");

    /* 1) ComIrq write-1-to-clear sanity check. */
    MFRC522_WriteRegister(&rfid, 0x04u, 0x7Fu);
    MFRC522_ReadRegister(&rfid, 0x04u, &w1c);
    demo_printf("W1C check: ComIrq after clear = 0x%02X (expect 0x00)\n",
                (unsigned int)w1c);

    /* 2) Plain soft reset — clean state, no driver init writes. */
    MFRC522_WriteRegister(&rfid, 0x01u, 0x0Fu);
    HAL_Delay(50u);

    /* 3) Try every plausible code; the real transceive consumes the FIFO
     *    byte (FIFO != 1) and, with a card present, fills it with ATQA. */
    for (i = 0u; i < (uint8_t)(sizeof(cmds) / sizeof(cmds[0])); i++) {
        demo_probe_cmd(cmds[i], 0x26u);
    }

    /* 4) Re-probe 0x0C with the RF drivers switched on (the reset state
     *    may have the antenna off). */
    demo_printf("-- re-probe with antenna on --\n");
    MFRC522_SetBits(&rfid, 0x14u, 0x03u);
    demo_probe_cmd(0x0Cu, 0x26u);

    demo_printf("FIFO=1 (NOT consumed) = the command never ran.\n");
    demo_printf("FIFO=0                = command ran, no card answer.\n");
    demo_printf("FIFO=2 [0x04][0x00]   = a card ANSWERED (ATQA)!\n");
}

int main(void)
{
    uint8_t any_found = 0u;

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART3_UART_Init();

    if (MFRC522_STM32_SPI_Init(&rfid, &hspi1,
                               RFID_CS_GPIO_Port,  RFID_CS_Pin,
                               RFID_RST_GPIO_Port, RFID_RST_Pin,
                               RFID_IRQ_GPIO_Port, RFID_IRQ_Pin) != MFRC522_OK) {
        Error_Handler();
    }
    if (MFRC522_Init(&rfid) != MFRC522_OK) {
        Error_Handler();
    }

    demo_printf("Polling for cards (5 s)...\n");

    /* Option A: non-busy polling. */
    {
        uint32_t deadline = HAL_GetTick() + 5000u;
        uint8_t found = 0u;

        while (HAL_GetTick() < deadline) {
            if (MFRC522_IsCardPresent(&rfid) == MFRC522_OK) {
                demo_printf("Card detected (polling)!\n");
                found = 1u;
                break;
            }
        }
        if (!found) {
            demo_printf("No card seen while polling.\n");
        }
        any_found = found;
    }

    /* Option B: blocking wait with timeout. */
    demo_printf("Now blocking-waiting for a card (5 s)...\n");
    if (MFRC522_WaitForCard(&rfid, 5000u) == MFRC522_OK) {
        demo_printf("Card detected (wait)!\n");
        any_found = 1u;
    } else {
        demo_printf("Wait timed out.\n");
    }

    /* No card by the driver? Sweep the command codes directly. */
    if (any_found == 0u) {
        demo_cmd_sweep();
    }

    while (1) {
    }
}
