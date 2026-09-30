/*
 * MSSP SPI smoke: program SPI master mode, verify SSPCON1/SSPSTAT program
 * correctly. No external slave on host sim (which does not model the SPI
 * shift register); the mdb gate proves hardware shifting + BF/SSPIF.
 */

#include "pic18f6520_hal.h"
#include "pic18f6520_sfr.h"
#include "peripherals/pic18f6520_ssp.h"
#include "core/pic18_irq.h"
#include "core/epic_harness.h"

/** Simulated run length (host only). */
#define SIM_CYCLES  1000UL

/**
 * @brief  Program MSSP SPI master + verify registers.
 *
 *          PASS if SSPCON1 programs to SPI-master-Fosc/4 + SSPEN, and
 *          SSPSTAT programs CKE/SMP as configured.
 */
int main(void)
{
    epic_harness_init(SIM_CYCLES);
    uint8_t ok = 1U;

    /* MSSP: SPI master Fosc/4, CKE=1, CKP=0, SMP=0. */
    SSP_HandleTypeDef h = SSP_HANDLE_DEFAULT;
    h.Mode             = SSP_MODE_SPI_MASTER_FOSC_4;
    h.ClockEdge        = SSP_SPI_CKE_IDLE_ACTIVE;
    h.ClockPolarity    = SSP_SPI_CKP_IDLE_LOW;
    h.SamplePhase      = SSP_SPI_SMP_MIDDLE;
    h.TransferCallback = NULL;
    if (EPIC_SSP_Init(&h) != EPIC_OK) ok = 0U;

    /* Verify SSPCON1: SSPM=0000 (SPI master Fosc/4) + SSPEN, CKP=0. */
    uint8_t con = epic_sfr_read8(PIC_REG_SSPCON1);
    if ((con & PIC_SSPCON1_SSPM_MASK) != SSP_MODE_SPI_MASTER_FOSC_4) ok = 0U;
    if (!(con & PIC_SSPCON1_SSPEN)) ok = 0U;
    if (con & PIC_SSPCON1_CKP) ok = 0U;

    /* Verify SSPSTAT: CKE=1, SMP=0. */
    uint8_t stat = epic_sfr_read8(PIC_REG_SSPSTAT);
    if (!(stat & PIC_SSPSTAT_CKE)) ok = 0U;
    if (stat & PIC_SSPSTAT_SMP) ok = 0U;

    for (uint32_t i = 0; epic_harness_running(i); i++)
    {
        epic_harness_tick();
    }

    epic_harness_log("MSSP SPI master programmed (CON=0x%02X STAT=0x%02X).\n",
                     (unsigned)con, (unsigned)stat);
    return epic_harness_report(ok);
}
