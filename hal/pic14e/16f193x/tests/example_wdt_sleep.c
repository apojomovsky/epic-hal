/*
 * BOR / POR status host probe: PCON<POR> and PCON<BOR> are active low
 * (nPOR / nBOR, DS41364B Register 3-3), so the bits are driven the way
 * hardware does (cleared on the event) and the helpers must report the
 * event and re-arm by setting the bit.
 */

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "pic16f193x_sim.h"
#include "core/pic16f193x_wdt_sleep.h"
#include "core/epic_harness.h"

/**
 * @brief  Drive the PCON reset flags like hardware and check the helpers.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;

    epic_harness_init(1UL);
    pic16f193x_sim_reset();

    /* The sim's reset image reports a Power-on Reset (POR=0). */
    ok &= (EPIC_POR_GetStatus() == 1U);

    /* Neither event recorded: both bits set. */
    EPIC_REG8(PIC_REG_PCON) |= (uint8_t)(PIC_PCON_POR | PIC_PCON_BOR);
    ok &= (EPIC_POR_GetStatus() == 0U);
    ok &= (EPIC_BOR_GetStatus() == 0U);

    /* Hardware clears the bit on the event. */
    EPIC_REG8(PIC_REG_PCON) &= (uint8_t)~PIC_PCON_POR;
    ok &= (EPIC_POR_GetStatus() == 1U);
    ok &= (EPIC_BOR_GetStatus() == 0U);
    EPIC_REG8(PIC_REG_PCON) &= (uint8_t)~PIC_PCON_BOR;
    ok &= (EPIC_BOR_GetStatus() == 1U);

    /* ClearFlag re-arms by setting the bit back, one flag at a time. */
    EPIC_POR_ClearFlag();
    ok &= (EPIC_POR_GetStatus() == 0U);
    ok &= (EPIC_BOR_GetStatus() == 1U);
    ok &= ((EPIC_REG8(PIC_REG_PCON) & (PIC_PCON_POR | PIC_PCON_BOR)) ==
           PIC_PCON_POR);
    EPIC_BOR_ClearFlag();
    ok &= (EPIC_BOR_GetStatus() == 0U);
    ok &= ((EPIC_REG8(PIC_REG_PCON) & (PIC_PCON_POR | PIC_PCON_BOR)) ==
           (PIC_PCON_POR | PIC_PCON_BOR));

    epic_harness_log("wdt_sleep: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
