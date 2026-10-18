/**
 * Host EPIC_WDT_Refresh and EPIC_Sleep_Enter, linked by the CMake build
 * (the target twin is pic16f188xx_wdt_sleep_target.c; the build picks
 * one, no `#ifdef`). The simulator has no WDT time-out and execution does
 * not stop, so both only count; probes read the counts back and keep
 * driving pic16f188xx_sim_step() to advance time.
 */
#include "core/pic16f188xx_wdt_sleep.h"
#include "pic16f188xx_sim_priv.h"

/**
 * @brief Count a WDT refresh in the simulator.
 */
void EPIC_WDT_Refresh(void)
{
    pic16f188xx_sim_wdt_note_refresh();
}

/**
 * @brief Count a Sleep entry in the simulator (an Idle entry too when
 *        CPUDOZE<IDLEN> is set).
 */
void EPIC_Sleep_Enter(void)
{
    pic16f188xx_sim_sleep_note();
}
