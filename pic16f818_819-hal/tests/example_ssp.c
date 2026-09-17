/* MSSP driver smoke test on the sim backend: SSPADD math (16 MHz /
 * 100 kHz gives 39), the SPI master Init register image, a byte written
 * to SSPBUF and a simulated RX byte that sets BF and comes back from
 * ReadByte. Expected image (DS39598F Registers 10-1/10-2): SSPCON =
 * 0x20 (SSPM = 0000 SPI master Fosc/4, CKP = 0, SSPEN = 1), SSPSTAT =
 * 0x40 (CKE = 1, SMP = 0) and SSPADD = 39 at address 0x93. This die has
 * no SSPCON2 and no SSPMSK, so there are no I2C master start/stop/ack
 * helpers and only the SPI path is covered. */

#include "pic16f818_819_hal.h"
#include "pic16f818_819_sim.h"
#include "pic16f818_819_sfr.h"
#include "peripherals/hal_ssp.h"
#include <stdio.h>

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m) do { if (c) { g_pass++; } else { printf("FAIL: %s\n", m); g_fail++; } } while (0)

/**
 * @brief Smoke-test the MSSP driver: SSPADD math, the SPI master
 *        register image, a byte transfer and the receive path.
 */
int main(void)
{
    /* 1. SSPADD formula: 16e6 / (4 x 100000) - 1 = 39. */
    uint16_t add = SSP_ComputeSSPADD(16000000UL, 100000UL);
    CHECK(add == 39U, "SSPADD 16MHz 100kHz != 39");
    add = SSP_ComputeSSPADD(16000000UL, 400000UL);
    CHECK(add == 9U, "SSPADD 16MHz 400kHz != 9");

    /* 2. Init in SPI master mode, Fosc/4. SSPADD is programmed in every
     *    mode, which also pins the Bank-1 literal write path. */
    pic16f818_819_sim_reset();

    SSP_HandleTypeDef h = SSP_HANDLE_DEFAULT;
    h.Mode   = SSP_MODE_SPI_MASTER_FOSC_4;
    h.SSPADD = 39U;
    EPIC_StatusTypeDef st = EPIC_SSP_Init(&h);
    CHECK(st == EPIC_OK, "Init returned error");

    CHECK(EPIC_REG8(PIC_REG_SSPCON) == 0x20U, "SSPCON (0x14) != 0x20");
    CHECK(EPIC_REG8(PIC_REG_SSPSTAT) == 0x40U, "SSPSTAT (0x94) != 0x40");
    CHECK(EPIC_REG8(PIC_REG_SSPADD) == 39U, "SSPADD (0x93) != 39");

    /* 3. Write a byte to SSPBUF. */
    CHECK(EPIC_SSP_WriteByte(0xA5U) == 0U, "WriteByte returned error");
    CHECK(EPIC_REG8(PIC_REG_SSPBUF) == 0xA5U, "SSPBUF did not capture 0xA5");

    /* 4. RX: drive a byte, read it back. The sim sets SSPSTAT<BF>. */
    pic16f818_819_sim_drive_ssp_rx(0xC3U);
    CHECK(EPIC_SSP_IsBufferFull() == 1U, "BF not set after drive_ssp_rx");
    uint8_t got = EPIC_SSP_ReadByte();
    CHECK(got == 0xC3U, "ReadByte did not return 0xC3");
    CHECK(EPIC_SSP_IsBufferFull() == 0U, "BF not cleared after ReadByte");

    printf("example_ssp: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
