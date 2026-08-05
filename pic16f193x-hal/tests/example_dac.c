#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "peripherals/pic16f193x_dac.h"
#include "core/epic_harness.h"
extern void pic16f193x_harness_halt(void);
int main(void)
{
    epic_harness_init(1UL);
    DAC_HandleTypeDef dac = DAC_HANDLE_DEFAULT;
    EPIC_DAC_Init(&dac);
    uint8_t con0 = EPIC_REG8(PIC_REG_DACCON0);
    uint8_t con1 = EPIC_REG8(PIC_REG_DACCON1);
    epic_harness_log("DACCON0=0x%02X DACCON1=0x%02X\n", con0, con1);
    int pass = (con0 == 0x80U) && (con1 == 0x0FU);
    int rc = epic_harness_report(pass);
    pic16f193x_harness_halt();
    return rc;
}
