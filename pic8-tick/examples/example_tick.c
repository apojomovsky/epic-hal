/**
 * @file    example_tick.c
 * @brief   pic8-tick smoke test: delays 10 ms and 5 ms, checks the elapsed
 *          counts land within one tick of the requested value, then
 *          reports PASS/FAIL via the harness. Builds and runs on both the
 *          host simulator and a real target.
 */

#include "pic8_tick.h"
#include "core/pic8_harness.h"

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define SIM_CYCLES 4000000UL

int main(void)
{
    pic8_harness_init(SIM_CYCLES);
    pic8_tick_init(FOSC_HZ);

    uint32_t t0 = pic8_tick_get();
    pic8_tick_delay_ms(10u);
    uint32_t e10 = pic8_tick_get() - t0;
    pic8_harness_log("tick: delay(10) -> %lu ms\n", (unsigned long)e10);

    uint32_t s = pic8_tick_get();
    pic8_tick_delay_ms(5u);
    uint32_t e5 = pic8_tick_elapsed_since(s);
    pic8_harness_log("tick: delay(5)  -> %lu ms\n", (unsigned long)e5);

    int ok = (e10 >= 10u) && (e10 <= 12u) && (e5 >= 5u) && (e5 <= 7u);
    return pic8_harness_report(ok);
}
