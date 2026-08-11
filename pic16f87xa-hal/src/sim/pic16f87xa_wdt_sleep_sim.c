/* Host-simulation implementation of EPIC_WDT_Refresh / EPIC_Sleep_Enter.
 * Linked by the CMake host build; the target twin is the _target.c.
 * No watchdog and no halted execution on the host, so both are no-ops. */

#include "core/pic16f87xa_wdt_sleep.h"

/**
 * @brief Refresh the Watchdog Timer. No-op on host: the sim does not
 *        model a watchdog timer.
 */
void EPIC_WDT_Refresh(void)
{
    /* No-op: the sim does not model a watchdog timer. */
}

/**
 * @brief Enter Sleep. No-op on host: the sim does not stop execution;
 *        callers should keep driving pic16f87xa_sim_step() to advance
 *        time.
 */
void EPIC_Sleep_Enter(void)
{
    /* No-op: the sim does not stop execution. Callers should keep driving
     * pic16f87xa_sim_step() to advance time. */
}
