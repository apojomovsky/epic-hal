/*
 * Trivial smoke test: prove the shared harness contract links and runs
 * against an empty PIC18 family backend (no GPIO, Timer0, or interrupts
 * touched), so `epic_harness_*` is genuinely family-blind.
 */

#include "pic18fxx5x.h"
#include "core/epic_harness.h"

/** Bounded run length (host only). */
#define SIM_CYCLES  10UL

int main(void)
{
    epic_harness_init(SIM_CYCLES);

    uint32_t ticks = 0U;
    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
        ticks++;
    }

    epic_harness_log("smoke: %u ticks, device %s\n",
                     (unsigned)ticks, PIC18FXX5X_DEVICE_NAME);
    return epic_harness_report(ticks == SIM_CYCLES);
}
