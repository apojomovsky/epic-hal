/*
 * Host-simulation implementation of EPIC_WDT_Refresh / EPIC_Sleep_Enter
 * (linked by the CMake host build; the target counterpart is
 * `pic18fxx5x_wdt_sleep_target.c`). No PIC18 CPU to stop and no WDT to
 * refresh on the host, so both are no-ops.
 */

#include "core/pic18fxx5x_wdt_sleep.h"

void EPIC_WDT_Refresh(void)
{
}

void EPIC_Sleep_Enter(void)
{
}
