/*
 * Host-simulation implementation of EPIC_WDT_Refresh / EPIC_Sleep_Enter
 * (linked by the CMake host build; the target counterpart is
 * `pic18fxx5x_wdt_sleep_target.c`). No PIC18 CPU to stop and no WDT to
 * refresh on the host, so both are no-ops.
 */

#include "core/pic18fxx5x_wdt_sleep.h"

/**
 * @brief  Refresh the Watchdog Timer (host-sim build): there is no WDT to
 *         refresh on the host, so this is a no-op.
 */
void EPIC_WDT_Refresh(void)
{
}

/**
 * @brief  Enter Power-down (Sleep) mode (host-sim build): there is no CPU
 *         to stop on the host, so this is a no-op; callers should
 *         continue to drive pic18_sim_step().
 */
void EPIC_Sleep_Enter(void)
{
}
