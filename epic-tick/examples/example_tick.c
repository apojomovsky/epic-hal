/**
 * epic-tick smoke test: delay 10 ms and 5 ms, check the elapsed counts
 * land within one tick of the requested value, report PASS/FAIL via the
 * harness. Builds and runs on host sim and a real target.
 */

#include "epic_tick.h"
#include "core/epic_harness.h"

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define SIM_CYCLES 4000000UL

int main(void)
{
    epic_harness_init(SIM_CYCLES);
    epic_tick_init(FOSC_HZ);

    uint32_t t0 = epic_tick_get();
    epic_tick_delay_ms(10u);
    uint32_t e10 = epic_tick_get() - t0;
    epic_harness_log("tick: delay(10) -> %lu ms\n", (unsigned long)e10);

    uint32_t s = epic_tick_get();
    epic_tick_delay_ms(5u);
    uint32_t e5 = epic_tick_elapsed_since(s);
    epic_harness_log("tick: delay(5)  -> %lu ms\n", (unsigned long)e5);

    int ok = (e10 >= 10u) && (e10 <= 12u) && (e5 >= 5u) && (e5 <= 7u);
    return epic_harness_report(ok);
}
