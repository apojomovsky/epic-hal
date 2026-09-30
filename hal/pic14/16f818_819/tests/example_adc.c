/* A/D driver smoke test on the sim backend: Init programs ADCON0 and
 * ADCON1 (AN2, Fosc/8, right justified, PCFG = 0010), Start sets
 * GO/DONE, and a simulated conversion returns 0x1A3 and sets ADIF.
 * Expected image: ADCON0 = 0x51 (ADON, CHS = 010, ADCS = 01), ADCON1 =
 * 0x82 (ADFM, ADCS2 = 0, PCFG = 0010: AN0 to AN4 analog against
 * AVDD/AVSS, Register 11-2), ADRESH = 0x01 and ADRESL = 0xA3 for a
 * right-justified 0x1A3; DeInit zeroes both. The five channels are
 * AN0 to AN4 on RA0 to RA4 (DS39598F Table 1-2). */

#include "pic16f818_819_hal.h"
#include "pic16f818_819_sim.h"
#include "pic16f818_819_sfr.h"
#include "peripherals/hal_adc.h"
#include "core/pic16_irq.h"
#include <stdio.h>

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m) do { if (c) { g_pass++; } else { printf("FAIL: %s\n", m); g_fail++; } } while (0)

/**
 * @brief Exercise the A/D driver on the sim backend: init, register
 *        image, start, simulated completion, read and deinit.
 */
int main(void)
{
    pic16f818_819_sim_reset();

    ADC_HandleTypeDef h = ADC_HANDLE_DEFAULT;
    h.Channel         = ADC_CHANNEL_AN2;
    h.ClockSource     = ADC_CLOCK_FOSC_8;
    h.ResultFormat    = ADC_FORMAT_RIGHT;
    h.Reference       = ADC_REFERENCE_VDD_VSS_5CH;
    EPIC_StatusTypeDef st = EPIC_ADC_Init(&h);
    CHECK(st == EPIC_OK, "Init returned error");

    /* ADCON0 = ADON(0x01) | CHS=010(0x10) | ADCS=01(0x40) = 0x51. */
    CHECK(EPIC_REG8(PIC_REG_ADCON0) == 0x51U, "ADCON0 (0x1F) != 0x51");
    /* ADCON1 = ADFM(0x80) | PCFG=0010(0x02) = 0x82. */
    CHECK(EPIC_REG8(PIC_REG_ADCON1) == 0x82U, "ADCON1 (0x9F) != 0x82");

    /* Start a conversion. */
    CHECK(EPIC_ADC_Start() == 0U, "EPIC_ADC_Start returned error");
    CHECK(EPIC_ADC_IsConversionInProgress() == 1U, "GO/DONE not set after Start");

    /* Sim the conversion. */
    pic16f818_819_sim_drive_adc_done(0x1A3U);
    CHECK(EPIC_ADC_IsConversionInProgress() == 0U, "GO/DONE not cleared after done");
    CHECK(EPIC_ADC_IsConversionDone() == 1U, "ADIF not set after done");
    CHECK(EPIC_REG8(PIC_REG_ADRESH) == 0x01U, "ADRESH (0x1E) != 0x01");
    CHECK(EPIC_REG8(PIC_REG_ADRESL) == 0xA3U, "ADRESL (0x9E) != 0xA3");

    uint16_t got = EPIC_ADC_Read();
    CHECK(got == 0x1A3U, "Read did not return 0x1A3");

    EPIC_ADC_ClearITFlag();
    CHECK(EPIC_ADC_IsConversionDone() == 0U, "ADIF not cleared by ClearITFlag");

    /* DeInit. */
    EPIC_ADC_DeInit();
    CHECK(EPIC_REG8(PIC_REG_ADCON0) == 0x00U, "ADCON0 not zero after DeInit");
    CHECK(EPIC_REG8(PIC_REG_ADCON1) == 0x00U, "ADCON1 not zero after DeInit");

    printf("example_adc: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
