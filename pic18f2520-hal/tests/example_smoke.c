/*
 * Trivial smoke test: prove the shared harness contract links and runs
 * against an empty PIC18F2520 family backend (no GPIO, Timer0, or
 * interrupts touched), so `epic_harness_*` is genuinely family-blind.
 */

#include "pic18f2520_hal.h"
#include "core/epic_harness.h"

/** Bounded run length (host only). */
#define SIM_CYCLES  10UL

/** @brief  Trivial harness contract smoke test.
 *
 *          Runs SIM_CYCLES ticks against an empty PIC18F2520 family
 *          backend, proving the shared harness is family-blind.
 */
int main(void)
{
    epic_harness_init(SIM_CYCLES);

    uint32_t ticks = 0U;
    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
        ticks++;
    }

    epic_harness_log("smoke: %u ticks, device %s\n",
                     (unsigned)ticks, PIC18F2520_DEVICE_NAME);
    return epic_harness_report(ticks == SIM_CYCLES);
}
