#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "peripherals/pic16f193x_fvr.h"
#include "core/pic8_harness.h"
extern void pic16f193x_harness_halt(void);
int main(void)
{
    pic8_harness_init(1UL);
    FVR_HandleTypeDef fvr = FVR_HANDLE_DEFAULT;
    HAL_FVR_Init(&fvr);
    uint8_t con = PIC8_REG8(PIC_REG_FVRCON);
    pic8_harness_log("FVRCON=0x%02X\n", con);
    /* FVREN=1 (bit7), FVRRDY=1 (bit6, read-only hw sets it), ADFVR=2 (bits1:0) */
    /* Driver writes 0x82, hw adds FVRRDY -> 0xC2. Mask FVRRDY for portability. */
    int pass = ((con & 0xBFU) == 0x8AU);
    int rc = pic8_harness_report(pass);
    pic16f193x_harness_halt();
    return rc;
}
