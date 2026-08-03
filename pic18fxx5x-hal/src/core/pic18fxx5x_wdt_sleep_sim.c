/**
 * @file    pic18fxx5x_wdt_sleep_sim.c
 * @brief   Host-simulation implementation of HAL_WDT_Refresh /
 *          HAL_Sleep_Enter.
 *
 * @details
 *   Linked by the CMake host build (the target counterpart is
 *   `pic18fxx5x_wdt_sleep_target.c`). No PIC18 CPU to stop and no WDT to
 *   refresh on the host, so both are no-ops.
 */

#include "core/pic18fxx5x_wdt_sleep.h"

void HAL_WDT_Refresh(void)
{
    /* No-op on the host sim: no WDT to refresh. */
}

void HAL_Sleep_Enter(void)
{
    /* No-op on the host sim: no CPU to halt. */
}
