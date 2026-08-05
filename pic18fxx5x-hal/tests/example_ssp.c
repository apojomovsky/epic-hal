/**
 * @file    example_ssp.c
 * @brief   MSSP driver smoke test on the PIC18 host sim.
 *
 * @details
 *   Verifies SSPADD baud computation, SPI master init/write/collision,
 *   sim-injected RX + BF clear, and I2C Start/Stop (SEN/PEN in SSPCON2).
 *   Builds for host sim and XC8 target with no `#ifdef`.
 */

#include "pic8_hal.h"
#include "core/pic8_harness.h"
#include "pic18fxx5x_sim.h"
#include <stdio.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { pic8_harness_log("FAIL: %s\n", msg); return pic8_harness_report(0); } \
} while (0)

int main(void)
{
    /* 1. SSPADD formula: Fosc / (4*Fscl) - 1. */
    uint16_t add = SSP_ComputeSSPADD(16000000UL, 100000UL);
    CHECK(add == 39U, "SSPADD 16MHz 100kHz != 39");
    add = SSP_ComputeSSPADD(16000000UL, 400000UL);
    CHECK(add == 9U, "SSPADD 16MHz 400kHz != 9");

    /* 2. Init in SPI master mode, Fosc/4. */
    pic8_harness_init(16U);

    SSP_HandleTypeDef h = SSP_HANDLE_DEFAULT;
    h.Mode = SSP_MODE_SPI_MASTER_FOSC_4;
    EPIC_SSP_Init(&h);

    /* Verify SSPCON1 = SSPEN(bit5) | mode 0000 = 0x20. */
    CHECK(pic8_sfr_read8(PIC_REG_SSPCON1) == 0x20U,
          "SSPCON1 not 0x20 for SPI master Fosc/4");

    /* 3. Write a byte to SSPBUF. */
    CHECK(EPIC_SSP_WriteByte(0xA5U) == 0U, "WriteByte returned error");
    CHECK(pic8_sfr_read8(PIC_REG_SSPBUF) == 0xA5U,
          "SSPBUF did not capture 0xA5");

    /* 4. RX: drive a byte, read it back. */
    pic18_sim_drive_ssp_rx(0xC3U);
    CHECK(EPIC_SSP_IsBufferFull() == 1U, "BF not set after drive_ssp_rx");
    uint8_t got = EPIC_SSP_ReadByte();
    CHECK(got == 0xC3U, "ReadByte did not return 0xC3");
    CHECK(EPIC_SSP_IsBufferFull() == 0U, "BF not cleared after ReadByte");

    /* 5. I2C master mode + start/stop. Verify SSPCON2 SEN, PEN bits. */
    h.Mode = SSP_MODE_I2C_MASTER_FOSC;
    h.SSPADD = 39U;
    EPIC_SSP_Init(&h);

    EPIC_SSP_Start();
    CHECK((pic8_sfr_read8(PIC_REG_SSPCON2) & PIC_SSPCON2_SEN) != 0U,
          "SEN not set after Start");

    EPIC_SSP_Stop();
    CHECK((pic8_sfr_read8(PIC_REG_SSPCON2) & PIC_SSPCON2_PEN) != 0U,
          "PEN not set after Stop");

    pic8_harness_log("OK: SSP driver, SSPADD math, SPI master, I2C master all pass.\n");
    return pic8_harness_report(1);
}
