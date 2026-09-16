/*
 * ADC smoke: program AN0 conversion, verify ADCON registers and start
 * conversion. Host sim verifies programming; mdb gate proves GO/DONE +
 * ADIF on hardware.
 */

#include "pic18f6520_hal.h"
#include "pic18f6520_sfr.h"
#include "peripherals/pic18f6520_adc.h"
#include "core/pic18_irq.h"
#include "core/epic_harness.h"

/** Simulated run length (host only). */
#define SIM_CYCLES  1000UL

/**
 * @brief  Program ADC + verify registers.
 *
 *          PASS if ADCON0 programs AN0+ADON.
 */
int main(void)
{
    epic_harness_init(SIM_CYCLES);
    uint8_t ok = 1U;

    /* ADC: AN0, Fosc/8, 2 Tad, right-justified, VDD/VSS. */
    ADC_HandleTypeDef h = ADC_HANDLE_DEFAULT;
    if (EPIC_ADC_Init(&h) != EPIC_OK) ok = 0U;

    /* Verify ADCON0: ADON + CHS=AN0. */
    uint8_t adcon0 = epic_sfr_read8(PIC_REG_ADCON0);
    if (!(adcon0 & PIC_ADCON0_ADON)) ok = 0U;
    if ((adcon0 & PIC_ADCON0_CHS_MASK) != (ADC_CHANNEL_AN0 << PIC_ADCON0_CHS_POS)) ok = 0U;

    /* Start a conversion. Host sim does not model completion; hardware
     * mdb gate proves GO clears + ADIF sets. */
    EPIC_ADC_Start();

    for (uint32_t i = 0; epic_harness_running(i); i++)
    {
        epic_harness_tick();
    }

    epic_harness_log("ADC AN0 programmed (ADCON0=0x%02X).\n", (unsigned)adcon0);
    return epic_harness_report(ok);
}
