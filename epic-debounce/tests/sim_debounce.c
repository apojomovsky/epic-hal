/**
 * @file    sim_debounce.c
 * @brief   Bounded, self-reporting HARNESS=sim build: epic-debounce's
 *          real `mdb` gate. Feeds a scripted noisy level sequence (a
 *          bouncy press and a bouncy release, rapid toggles around
 *          each transition) through the module's own `debounce_read_fn`
 *          callback and verifies the debounced output settles exactly
 *          like the module's host tests (tests/test_debounce.c) say it
 *          must: one PRESSED and one RELEASED, each only after the full
 *          debounce window, no mid-bounce flapping, then reports
 *          PASS/FAIL over the target's real USART the same way every
 *          other family's own `.sim` variant does (see
 *          pic16f87xa-hal/src/core/pic16_harness_sim_target.c).
 *
 * @details
 *   The debounce engine reads its input through a caller-supplied
 *   callback, so unlike a GPIO-pin module there is nothing MPLAB SIM
 *   cannot drive (no pin-injection limitation applies): `sim_level()`
 *   synthesizes the raw pin level from `epic_tick_get()` simulated
 *   time, the same trick the host tests use, re-keyed from a call-count
 *   script to a time script because this sim loop polls far faster than
 *   once per simulated ms. Script (simulated ms: raw level):
 *     0-9:   0 (idle)
 *     10-15: 1,0,1,0,1,0 (bouncy press)
 *     16-39: 1 (stably held)
 *     40-45: 0,1,0,1,0,1 (bouncy release)
 *     46+:   0 (released)
 *   With DB_MS=20 the engine must commit PRESSED at t=36 (20 ms after
 *   the last bounce at t=16) and RELEASED at t=66 (20 ms after the
 *   last bounce at t=46), exactly once each.
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

/* Scripted raw input level as a function of simulated time (ms).
 * Mirrors the host tests' bouncy-transition scripts, keyed to time
 * instead of poll count. */
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
