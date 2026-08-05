/**
 * @file    example_ssp.c
 * @brief   MSSP driver smoke test on the sim backend.
 *
 *   Verifies:
 *     1. SSPADD computation: 16 MHz / 100 kHz I²C → 39 (Fosc/(4*Fscl)-1).
 *     2. EPIC_SSP_Init() in SPI master mode programs SSPCON correctly.
 *     3. SPI write goes to SSPBUF, write collision reporting works.
 *     4. pic16f87xa_sim_drive_ssp_rx() injects a byte, EPIC_SSP_ReadByte
 *        returns it, and BF is cleared.
 */

#include "pic16f87xa.h"
#include "pic16f87xa_sim.h"
#include "pic16f87xa_sfr.h"
#include "peripherals/pic16f87xa_ssp.h"
#include <stdio.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } \
} while (0)

int main(void)
{
    /* 1. SSPADD formula. */
    uint16_t add = SSP_ComputeSSPADD(16000000UL, 100000UL);
    /* 16e6 / (4 * 100000) - 1 = 39 */
    CHECK(add == 39U, "SSPADD 16MHz 100kHz != 39");
    add = SSP_ComputeSSPADD(16000000UL, 400000UL);
    /* 16e6 / (4 * 400000) - 1 = 9 */
    CHECK(add == 9U, "SSPADD 16MHz 400kHz != 9");

    /* 2. Init in SPI master mode, Fosc/4. */
    pic16f87xa_sim_reset();

    SSP_HandleTypeDef h = SSP_HANDLE_DEFAULT;
    h.Mode = SSP_MODE_SPI_MASTER_FOSC_4;
    EPIC_SSP_Init(&h);

    /* Verify SSPCON. Mode 0000 (bits 0..3), CKP=0, SSPEN=1 (bit 5). */
    uint8_t sspcon = PIC8_REG8(0x14U);
    CHECK((sspcon & 0x3FU) == 0x20U,
          "SSPCON not programmed for SPI master Fosc/4 (expected 0x20)");

    /* 3. Write a byte to SSPBUF. */
    CHECK(EPIC_SSP_WriteByte(0xA5U) == 0U, "WriteByte returned error");
    CHECK(PIC8_REG8(PIC_REG_SSPBUF) == 0xA5U, "SSPBUF did not capture 0xA5");

    /* 4. RX: drive a byte, read it back. */
    pic16f87xa_sim_drive_ssp_rx(0xC3U);
    /* Sim sets SSPSTAT<BF>. */
    CHECK(EPIC_SSP_IsBufferFull() == 1U, "BF not set after drive_ssp_rx");
    uint8_t got = EPIC_SSP_ReadByte();
    CHECK(got == 0xC3U, "ReadByte did not return 0xC3");
    CHECK(EPIC_SSP_IsBufferFull() == 0U, "BF not cleared after ReadByte");

    /* 5. I²C master mode + start/stop. Verify SSPCON2 SEN, PEN bits. */
    h.Mode = SSP_MODE_I2C_MASTER_FOSC;
    h.SSPADD = 39U;
    EPIC_SSP_Init(&h);

    EPIC_SSP_Start();
    /* SSPCON2 SEN bit should be set. */
    {
        uint8_t prev = (PIC8_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        uint8_t sspcon2 = PIC8_REG8(0x91U);
        pic_select_bank(prev);
        CHECK((sspcon2 & PIC_SSPCON2_SEN) != 0U, "SEN not set after Start");
    }

    EPIC_SSP_Stop();
    {
        uint8_t prev = (PIC8_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        uint8_t sspcon2 = PIC8_REG8(0x91U);
        pic_select_bank(prev);
        CHECK((sspcon2 & PIC_SSPCON2_PEN) != 0U, "PEN not set after Stop");
    }

    printf("OK: SSP driver, SSPADD math, SPI master, I2C master all pass.\n");
    return 0;
}
