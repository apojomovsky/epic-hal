#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "peripherals/pic16f193x_srlatch.h"
#include "core/epic_harness.h"
/**
 * @brief Freeze the target so the harness PASS marker stays set; no-op
 * on the host build.
 */
extern void pic16f193x_harness_halt(void);
/**
 * @brief SR latch smoke test: enable the latch and verify the SRCON0
 * register state.
 */
int main(void)
{
    epic_harness_init(1UL);
    EPIC_SRLATCH_Enable();
    uint8_t con0 = EPIC_REG8(PIC_REG_SRCON0);
    epic_harness_log("SRCON0=0x%02X\n", con0);
    int pass = (con0 == 0x80U);
    int rc = epic_harness_report(pass);
    pic16f193x_harness_halt();
    return rc;
}
