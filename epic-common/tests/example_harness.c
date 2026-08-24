/* Minimal epic-common smoke: exercise the harness contract (init,
 * tick, running) on every build the manifest produces. The harness is
 * family-blind, so this one program is the module's example for every
 * family, and the same file builds under XC8, epic-cc, and host gcc.
 *
 * The target harness is all no-ops (the CPU starts itself, time
 * advances on its own, no stdout), so on target this loops forever and
 * the mdb gate checks liveness, not a marker. epic_harness_log /
 * epic_harness_report are deliberately not called: they lower to
 * const flash strings, which hit a filed epic-cc isel gap (flash GEP,
 * epic-cc#114). */

#include "core/epic_harness.h"

/**
 * @brief  Run the harness pump forever.
 * @return 0 (never reached on target; the host sim's bounded run
 *         finishes and returns).
 */
int main(void)
{
    epic_harness_init(1000000UL);
    uint32_t i = 0;
    while (epic_harness_running(i)) {
        epic_harness_tick();
        i++;
    }
    return 0;
}
