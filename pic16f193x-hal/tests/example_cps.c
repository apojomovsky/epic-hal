#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "peripherals/pic16f193x_cps.h"
#include "core/epic_harness.h"
extern void pic16f193x_harness_halt(void);
int main(void)
{
    epic_harness_init(1UL);
    CPS_HandleTypeDef cps = CPS_HANDLE_DEFAULT;
    EPIC_CPS_Init(&cps);
    uint8_t con0 = EPIC_REG8(PIC_REG_CPSCON0);
    uint8_t con1 = EPIC_REG8(PIC_REG_CPSCON1);
    epic_harness_log("CPSCON0=0x%02X CPSCON1=0x%02X\n", con0, con1);
    /* CPSOUT (bit 1) is read-only; mask it. CPSON=1 + T0XCS=1 = 0x81 */
    int pass = ((con0 & 0xFDU) == 0x81U) && (con1 == 0x00U);
    int rc = epic_harness_report(pass);
    pic16f193x_harness_halt();
    return rc;
}
