/*
 * BOR / POR status host probe: RCON<POR> and RCON<BOR> are active low,
 * so the bits are driven the way hardware does (cleared on the event) and
 * the helpers must report the event and re-arm by setting the bit.
 */

#include "pic18f6520_hal.h"
#include "pic18f6520_sfr.h"
#include "pic18f6520_sim.h"
#include "core/pic18f6520_wdt_sleep.h"
#include "core/epic_harness.h"

/**
 * @brief  Drive the RCON reset flags like hardware and check the helpers.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;

    epic_harness_init(1UL);
    pic18_sim_reset();

    /* Neither event recorded: both bits set. */
    EPIC_REG8(PIC_REG_RCON) |= (uint8_t)(PIC_RCON_POR | PIC_RCON_BOR);
    ok &= (EPIC_POR_GetStatus() == 0U);
    ok &= (EPIC_BOR_GetStatus() == 0U);

    /* Hardware clears the bit on the event. */
    EPIC_REG8(PIC_REG_RCON) &= (uint8_t)~PIC_RCON_POR;
    ok &= (EPIC_POR_GetStatus() == 1U);
    ok &= (EPIC_BOR_GetStatus() == 0U);
    EPIC_REG8(PIC_REG_RCON) &= (uint8_t)~PIC_RCON_BOR;
    ok &= (EPIC_BOR_GetStatus() == 1U);

    /* ClearFlag re-arms by setting the bit back, one flag at a time. */
    EPIC_POR_ClearFlag();
    ok &= (EPIC_POR_GetStatus() == 0U);
    ok &= (EPIC_BOR_GetStatus() == 1U);
    ok &= ((EPIC_REG8(PIC_REG_RCON) & (PIC_RCON_POR | PIC_RCON_BOR)) ==
           PIC_RCON_POR);
    EPIC_BOR_ClearFlag();
    ok &= (EPIC_BOR_GetStatus() == 0U);
    ok &= ((EPIC_REG8(PIC_REG_RCON) & (PIC_RCON_POR | PIC_RCON_BOR)) ==
           (PIC_RCON_POR | PIC_RCON_BOR));

    epic_harness_log("wdt_sleep: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
