/*
 * A/D converter driver smoke test on the PIC18 host sim: init register
 * programming (ADCON0/1/2, DS39632E §21.0), start/complete via GO/DONE
 * and ADIF, 10-bit read in both ADFM justifications, Vref/channel
 * select, deinit. Host sim only; the XC8 target build uses example_blink.
 */

#include "epic_hal.h"
#include "core/epic_harness.h"
#include "pic18fxx5x_sim.h"

#define CHECK(cond, msg) do { \
    if (!(cond)) { epic_harness_log("FAIL: %s\n", msg); return epic_harness_report(0); } \
} while (0)

int main(void)
{
    epic_harness_init(16U);
    pic18_sim_set_irq_callback(NULL);

    /* 1. Init: AN2, Fosc/8, 2 Tad, right-justified, VDD/VSS, PCFG=0.
     *    ADCON0 = ADON(0x01) | CHS=AN2(0x02<<2=0x08)        = 0x09
     *    ADCON1 = VDD_VSS(0x00) | PCFG0(0x00)               = 0x00
     *    ADCON2 = ADFM(0x80) | ACQ_2TAD(0x01<<3=0x08) | ADCS_FOSC_8(0x01) = 0x89 */
    ADC_HandleTypeDef h = ADC_HANDLE_DEFAULT;
    h.Channel      = ADC_CHANNEL_AN2;
    h.ClockSource  = ADC_CLOCK_FOSC_8;
    h.Acquisition  = ADC_ACQ_2TAD;
    h.ResultFormat = ADC_FORMAT_RIGHT;
    h.VReference   = ADC_VREF_VDD_VSS;
    h.PinConfig    = 0x0U;
    EPIC_ADC_Init(&h);

    CHECK(epic_sfr_read8(PIC_REG_ADCON0) == 0x09U, "ADCON0 not 0x09 for AN2/Fosc8");
    CHECK(epic_sfr_read8(PIC_REG_ADCON1) == 0x00U, "ADCON1 not 0x00 for VDD/VSS, PCFG0");
    CHECK(epic_sfr_read8(PIC_REG_ADCON2) == 0x89U, "ADCON2 not 0x89 for ADFM|ACQ2|Fosc8");

    /* 2. Start a conversion. */
    CHECK(EPIC_ADC_Start() == 0U, "EPIC_ADC_Start returned error");
    CHECK(EPIC_ADC_IsConversionInProgress() == 1U, "GO/DONE not set after Start");

    /* 3. Sim conversion completion (0x1A3 = 419), right-justified. */
    pic18_sim_drive_adc_done(0x1A3U);
    CHECK(EPIC_ADC_IsConversionInProgress() == 0U, "GO/DONE not cleared after done");
    CHECK(EPIC_ADC_IsConversionDone() == 1U, "ADIF not set after done");
    CHECK(EPIC_ADC_Read() == 0x1A3U, "Read did not return 0x1A3 (right-justified)");
    EPIC_ADC_ClearITFlag();
    CHECK(EPIC_ADC_IsConversionDone() == 0U, "ADIF not cleared");

    /* 4. Left-justified: the sim stores left, the driver right-shifts by 6. */
    h.ResultFormat = ADC_FORMAT_LEFT;
    EPIC_ADC_Init(&h);
    pic18_sim_drive_adc_done(0x1A3U);
    CHECK(EPIC_ADC_Read() == 0x1A3U, "Read did not return 0x1A3 (left-justified)");

    /* 5. Vref = AN3/VSS -> ADCON1<VCFG0> (bit 4) set. */
    h.ResultFormat = ADC_FORMAT_RIGHT;
    h.VReference   = ADC_VREF_AN3_VSS;
    EPIC_ADC_Init(&h);
    CHECK((epic_sfr_read8(PIC_REG_ADCON1) & PIC_ADCON1_VCFG0) != 0U,
          "VCFG0 not set for Vref+ = AN3");

    /* 6. SelectChannel(AN5) -> CHS = 5<<2 = 0x14, ADON stays -> 0x15. */
    EPIC_ADC_SelectChannel(ADC_CHANNEL_AN5);
    CHECK(epic_sfr_read8(PIC_REG_ADCON0) == 0x15U, "ADCON0 CHS not AN5 (0x15)");

    /* 7. DeInit restores 0x00. */
    EPIC_ADC_DeInit();
    CHECK(epic_sfr_read8(PIC_REG_ADCON0) == 0x00U, "ADCON0 not 0x00 after DeInit");
    CHECK(epic_sfr_read8(PIC_REG_ADCON2) == 0x00U, "ADCON2 not 0x00 after DeInit");

    epic_harness_log("OK: ADC driver, init (3 ADCON regs), start/done, read (both "
                     "justifications), Vref, channel select all pass.\n");
    return epic_harness_report(1);
}
