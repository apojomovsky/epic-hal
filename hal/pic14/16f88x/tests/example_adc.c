/* A/D driver smoke test on the sim backend: Init programs ADCON0+ADCON1
 * (AN2, Fosc/8, right-justified, VDD/VSS), Start sets GO/DONE, and a
 * simulated conversion (pic16f88x_sim_drive_adc_done) returns 0x1A3
 * and sets ADIF; DeInit zeroes the ADC registers. */

#include "pic16f88x.h"
#include "pic16f88x_sim.h"
#include "pic16f88x_sfr.h"
#include "peripherals/pic16f88x_adc.h"
#include "core/pic16_irq.h"
#include <stdio.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } \
} while (0)

/**
 * @brief Exercise the A/D driver on the sim backend: init, start,
 *        simulated completion, read and deinit.
 */
int main(void)
{
    pic16f88x_sim_reset();

    ADC_HandleTypeDef h = ADC_HANDLE_DEFAULT;
    h.Channel         = ADC_CHANNEL_AN2;
    h.ClockSource     = ADC_CLOCK_FOSC_8;
    h.ResultFormat    = ADC_FORMAT_RIGHT;
    h.Reference       = ADC_REFERENCE_VDD_VSS;
    EPIC_ADC_Init(&h);

    /* ADCON0 = ADON(0x01) | CHS=0010<<2(0x08) | ADCS=01<<6(0x40) = 0x49 */
    uint8_t adcon0 = EPIC_REG8(PIC_REG_ADCON0);
    CHECK((adcon0 & 0x49U) == 0x49U, "ADCON0 not programmed for AN2 Fosc/8");

    /* ADCON1 = ADFM=1(0x80) | VCFG1:VCFG0=00(0x00) = 0x80 */
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        uint8_t adcon1 = EPIC_REG8(PIC_REG_ADCON1);
        pic_select_bank(prev);
        CHECK((adcon1 & 0x80U) == 0x80U, "ADCON1 not programmed correctly");
    }

    /* Start a conversion. */
    CHECK(EPIC_ADC_Start() == 0U, "EPIC_ADC_Start returned error");
    CHECK(EPIC_ADC_IsConversionInProgress() == 1U, "GO/DONE not set after Start");

    /* Sim the conversion. */
    pic16f88x_sim_drive_adc_done(0x1A3U);
    CHECK(EPIC_ADC_IsConversionInProgress() == 0U, "GO/DONE not cleared after done");
    CHECK(EPIC_ADC_IsConversionDone() == 1U, "ADIF not set after done");

    uint16_t got = EPIC_ADC_Read();
    CHECK(got == 0x1A3U, "Read did not return 0x1A3");

    EPIC_ADC_ClearITFlag();
    CHECK(EPIC_ADC_IsConversionDone() == 0U, "ADIF not cleared by ClearITFlag");

    /* DeInit. */
    EPIC_ADC_DeInit();
    CHECK(EPIC_REG8(PIC_REG_ADCON0) == 0x00U, "ADCON0 not zero after DeInit");

    printf("OK: ADC driver, channel/clock config, start, complete, read all pass.\n");
    return 0;
}
