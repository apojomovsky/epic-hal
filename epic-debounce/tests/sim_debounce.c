/**
 * Bounded, self-reporting HARNESS=sim build: epic-debounce's real mdb
 * gate. `sim_level()` synthesizes the raw level from `epic_tick_get()`
 * time, since the sim loop polls faster than once per ms (bouncy press
 * 10..15 ms, stable high to 39, bouncy release 40..45, low after).
 * With DB_MS=20 the engine must emit PRESSED at t=36 and RELEASED at
 * t=66, once each, then report PASS/FAIL over the target's real USART.
 */

#include "debounce.h"
#include "epic_tick.h"
#include "core/epic_harness.h"

#include <stddef.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define DB_MS           20u
#define PRESS_BOUNCE_MS 16u   /* raw stable high from t=16 (bounces at 10..15) */
#define REL_BOUNCE_MS   46u   /* raw stable low  from t=46 (bounces at 40..45) */
#define SCENARIO_END_MS 100u  /* past the expected RELEASED at ~66 */
#define SIM_ITERATIONS  200000UL

/**
 * @brief  Scripted raw input level as a function of simulated time (ms).
 *         Mirrors the host tests' bouncy-transition scripts, keyed to
 *         time instead of poll count.
 */
static bool sim_level(void *ctx)
{
    (void)ctx;
    uint32_t t = epic_tick_get();

    if (t < 10u) { return false; }
    if (t < 16u) { return ((t & 1u) == 0u); }   /* 10..15: 1,0,1,0,1,0 */
    if (t < 40u) { return true; }
    if (t < 46u) { return ((t & 1u) != 0u); }   /* 40..45: 0,1,0,1,0,1 */
    return false;
}

/**
 * @brief  Run the scripted bounce scenario and report PASS/FAIL.
 */
int main(void)
{
    epic_harness_init(SIM_ITERATIONS);
    epic_tick_init(FOSC_HZ);

    debounce_t db;
    debounce_init(&db, sim_level, NULL, DB_MS);

    int presses = 0, releases = 0;
    uint32_t press_t = 0u, release_t = 0u;

    for (uint32_t i = 0;
         epic_harness_running(i) && (epic_tick_get() < SCENARIO_END_MS);
         i++) {
        epic_harness_tick();

        debounce_event_t ev = debounce_poll(&db);
        if (ev == DEBOUNCE_EVENT_PRESSED) {
            presses++;
            press_t = epic_tick_get();
        } else if (ev == DEBOUNCE_EVENT_RELEASED) {
            releases++;
            release_t = epic_tick_get();
        }
    }

    /* Exactly one edge per transition (the bounces must not flap), each
     * committed no earlier than the full window after the last bounce,
     * and the instance must end in the released state. */
    int ok = (presses == 1) && (releases == 1) &&
             (press_t >= PRESS_BOUNCE_MS + DB_MS) &&
             (release_t >= REL_BOUNCE_MS + DB_MS) &&
             !debounce_is_active(&db);

    if (ok) {
        epic_harness_log("debounce sim: 1 press, 1 release, window respected\n");
    } else {
        epic_harness_log("debounce sim: checks failed\n");
    }
    return epic_harness_report(ok);
}
