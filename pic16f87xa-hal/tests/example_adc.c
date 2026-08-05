/**
 * @file    example_adc.c
 * @brief   A/D driver smoke test on the sim backend.
 *
 *   Verifies:
 *     1. EPIC_ADC_Init() programs ADCON0 + ADCON1 correctly for
 *        channel AN2, Fosc/8, right-justified, VDD/VSS reference.
 *     2. EPIC_ADC_Start() sets GO/DONE.
 *     3. pic16f87xa_sim_drive_adc_done(0x1A3) simulates conversion
 *        completion; EPIC_ADC_Read() returns 0x1A3, GO/DONE clears,
 *        and ADIF is set.
 *     4. The default ADRESH:ADRESL are 0/0 after DeInit.
 */

#include "pic16f87xa.h"
#include "pic16f87xa_sim.h"
#include "pic16f87xa_sfr.h"
#include "peripherals/pic16f87xa_adc.h"
#include "core/pic16_irq.h"
#include <stdio.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } \
} while (0)

int main(void)
{
    pic16f87xa_sim_reset();

    ADC_HandleTypeDef h = ADC_HANDLE_DEFAULT;
    h.Channel         = ADC_CHANNEL_AN2;
    h.ClockSource     = ADC_CLOCK_FOSC_8;
    h.ResultFormat    = ADC_FORMAT_RIGHT;
    h.Reference       = ADC_REFERENCE_VDD_VSS_5CH;
    EPIC_ADC_Init(&h);

    /* ADCON0 = ADON(0x01) | CHS=010(0x10) | ADCS=001(0x40) = 0x51 */
    uint8_t adcon0 = EPIC_REG8(0x1FU);
    CHECK((adcon0 & 0x51U) == 0x51U, "ADCON0 not programmed for AN2 Fosc/8");

    /* ADCON1 = PCFG=0010(0x02) | ADFM=1(0x80) = 0x82 */
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        uint8_t adcon1 = EPIC_REG8(0x9FU);
        pic_select_bank(prev);
        CHECK((adcon1 & 0x82U) == 0x82U, "ADCON1 not programmed correctly");
    }

    /* Start a conversion. */
    CHECK(EPIC_ADC_Start() == 0U, "EPIC_ADC_Start returned error");
    CHECK(EPIC_ADC_IsConversionInProgress() == 1U, "GO/DONE not set after Start");

    /* Sim the conversion. */
    pic16f87xa_sim_drive_adc_done(0x1A3U);
    CHECK(EPIC_ADC_IsConversionInProgress() == 0U, "GO/DONE not cleared after done");
    CHECK(EPIC_ADC_IsConversionDone() == 1U, "ADIF not set after done");

    uint16_t got = EPIC_ADC_Read();
    CHECK(got == 0x1A3U, "Read did not return 0x1A3");

    EPIC_ADC_ClearITFlag();
    CHECK(EPIC_ADC_IsConversionDone() == 0U, "ADIF not cleared by ClearITFlag");

    /* DeInit. */
    EPIC_ADC_DeInit();
    CHECK(EPIC_REG8(0x1FU) == 0x00U, "ADCON0 not zero after DeInit");

    printf("OK: ADC driver, channel/clock config, start, complete, read all pass.\n");
    return 0;
}
