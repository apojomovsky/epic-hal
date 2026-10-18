/**
 * Real-target EPIC_WDT_Refresh and EPIC_Sleep_Enter, linked by the XC8
 * build (the host twin is pic16f188xx_wdt_sleep_sim.c). Both are native
 * instructions on the Enhanced Mid-range core (DS40001802D §9.5, §8.2).
 */
#include "core/pic16f188xx_wdt_sleep.h"

/**
 * @brief Refresh the WDT: read WDTCON0, then `clrwdt`.
 * @details In windowed mode the WDT must be armed by a WDTCON0 read before
 * a CLRWDT counts, or the CLRWDT is a window violation (§9.5.1). The read
 * costs nothing with the window open.
 */
void EPIC_WDT_Refresh(void)
{
    (void)EPIC_REG8(PIC_REG_WDTCON0);
    asm("clrwdt");
}

/**
 * @brief Enter Sleep (Idle with CPUDOZE<IDLEN> set).
 * @details The `nop` is the instruction §8.2.1 says to place after SLEEP:
 * a pending interrupt runs the instruction after SLEEP first.
 */
void EPIC_Sleep_Enter(void)
{
    asm("sleep");
    asm("nop");
}
