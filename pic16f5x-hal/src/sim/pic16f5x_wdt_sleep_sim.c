/* PIC16F5x host-simulation implementation of EPIC_WDT_Refresh /
 * EPIC_Sleep_Enter. Linked by the CMake host build; the target twin is
 * pic16f5x_wdt_sleep_target.c. No watchdog and no halted execution on
 * the host, so both are no-ops. */

#include "core/pic16f5x_wdt_sleep.h"

/**
 * @brief  Feed the watchdog timer.
 * @note   No-op: the sim does not model a watchdog timer.
 */
void EPIC_WDT_Refresh(void)
{
    /* No-op: the sim does not model a watchdog timer. */
}

/**
 * @brief  Enter sleep.
 * @note   No-op: the sim does not stop execution; callers should keep
 *         driving the family sim_step() to advance time.
 */
void EPIC_Sleep_Enter(void)
{
    /* No-op: the sim does not stop execution; callers should keep
     * driving the family sim_step() to advance time. */
}
