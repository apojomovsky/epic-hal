/**
 * @file    example_smoke.c
 * @brief   Trivial smoke test: prove the shared harness contract links and
 *          runs against an empty PIC18 family backend.
 *
 * @details
 *   Exercises only the four-function host/target harness contract
 *   (`core/pic8_harness.h`): init, a bounded tick loop, a log line, a
 *   pass/fail report. No GPIO, Timer0, or interrupts touched; proves
 *   `pic8_harness_*` is genuinely family-blind, since PIC18 links against
 *   the exact same header and contract PIC16 uses.
 */

#include "pic18fxx5x.h"
#include "core/pic8_harness.h"

/** Bounded run length (host only). */
#define SIM_CYCLES  10UL

int main(void)
{
    pic8_harness_init(SIM_CYCLES);

    uint32_t ticks = 0U;
    for (uint32_t i = 0; pic8_harness_running(i); i++) {
        pic8_harness_tick();
        ticks++;
    }

    pic8_harness_log("smoke: %u ticks, device %s\n",
                     (unsigned)ticks, PIC18FXX5X_DEVICE_NAME);
    return pic8_harness_report(ticks == SIM_CYCLES);
}
