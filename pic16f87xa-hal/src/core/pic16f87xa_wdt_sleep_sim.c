/**
 * @file    pic16f87xa_wdt_sleep_sim.c
 * @brief   Host-simulation implementation of EPIC_WDT_Refresh / EPIC_Sleep_Enter.
 *
 * @details
 *   Linked by the CMake host build. The companion target implementation
 *   is pic16f87xa_wdt_sleep_target.c; the build picks one, so neither
 *   this file nor the examples need `#ifdef`. On the host there is no
 *   watchdog and execution does not stop, so both are no-ops. The shared
 *   BOR/POR status helpers live in pic16f87xa_wdt_sleep.c.
 */

#include "core/pic16f87xa_wdt_sleep.h"

void EPIC_WDT_Refresh(void)
{
    /* No-op: the sim does not model a watchdog timer. */
}

void EPIC_Sleep_Enter(void)
{
    /* No-op: the sim does not stop execution. Callers should keep driving
     * pic16f87xa_sim_step() to advance time. */
}
