/* Real-target implementation of EPIC_WDT_Refresh / EPIC_Sleep_Enter:
 * the native clrwdt / sleep instructions. Linked by the XC8 build; the
 * host twin is the _sim.c. */

#include "core/pic16f87xa_wdt_sleep.h"

void EPIC_WDT_Refresh(void)
{
    asm("clrwdt");
}

void EPIC_Sleep_Enter(void)
{
    asm("sleep");
}
