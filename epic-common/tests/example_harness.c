/* Minimal epic-common smoke: the harness contract. Log/report are
 * omitted: const flash strings hit epic-cc#114 and the target harness
 * is a no-op, so the mdb gate checks liveness, not a marker. */

#include "core/epic_harness.h"

/**
 * @brief Run the harness pump forever.
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
