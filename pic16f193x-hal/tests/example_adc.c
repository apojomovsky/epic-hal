/**
 * ADC smoke test: init (errata-safe FRC clock), confirm control-register
 * state. Expected register image (after init):
 *   ADCON0 = 0x01   (ADON=1, CHS=00000)
 *   ADCON1 = 0xB0   (ADFM=1, ADCS=011 FRC, ADPREF=00, ADNREF=0)
 */

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "peripherals/pic16f193x_adc.h"
#include "core/epic_harness.h"

extern void pic16f193x_harness_halt(void);

int main(void)
{
    epic_harness_init(1UL);

    ADC_HandleTypeDef adc = ADC_HANDLE_DEFAULT;
    EPIC_ADC_Init(&adc);

    uint8_t con0 = EPIC_REG8(PIC_REG_ADCON0);
    uint8_t con1 = EPIC_REG8(PIC_REG_ADCON1);

    epic_harness_log("ADCON0=0x%02X ADCON1=0x%02X\n", con0, con1);
    int pass = (con0 == 0x01U) && (con1 == 0xB0U);
    int rc = epic_harness_report(pass);
    pic16f193x_harness_halt();
    return rc;
}
