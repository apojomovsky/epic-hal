#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "peripherals/pic16f193x_srlatch.h"
#include "core/epic_harness.h"
extern void pic16f193x_harness_halt(void);
int main(void)
{
    epic_harness_init(1UL);
    EPIC_SRLATCH_Enable();
    uint8_t con0 = PIC8_REG8(PIC_REG_SRCON0);
    epic_harness_log("SRCON0=0x%02X\n", con0);
    int pass = (con0 == 0x80U);
    int rc = epic_harness_report(pass);
    pic16f193x_harness_halt();
    return rc;
}
