/**
 * @file    sim_target_swuart.c
 * @brief   Bounded, self-reporting HARNESS=sim build: epic-swuart's
 *          first-ever real `mdb` gate. Writes one byte through the
 *          real CCP2 compare-driven TX state machine and confirms it
 *          drains, then reports PASS/FAIL over the target's real
 *          hardware USART the same way every other family's own
 *          `.sim` variant does (see
 *          pic16f87xa-hal/src/core/pic16_harness_sim_target.c).
 *
 * @details
 *   Distinct from `tests/example_swuart.c` (the real-target loopback
 *   demo: an unbounded loop, no `core/epic_harness.h` dependency,
 *   meant to run forever on real silicon with a real RC1->RC2 jumper).
 *   This file follows the bounded host/target-agnostic contract
 *   `core/epic_harness.h` defines instead, the same pattern
 *   `pic16f193x-hal/tests/example_timer1.c` uses for its own `.sim`
 *   entry.
 *
 *   TX-only, not the full TX+RX loopback the task brief asked for
 *   first: MPLAB SIM has no physical board, so there is no real
 *   RC1->RC2 jumper, and this plan's own Task 2 probe already showed
 *   `mdb` CAN drive an input pin's level from script (`write pin RC2
 *   high|low`), which is what made a scripted stand-in for that wire
 *   look viable. Two real, different obstacles turned up actually
 *   building it, both confirmed by hand against this exact `.hex`
 *   under `mdb` before giving up on them (see
 *   docs/superpowers/plans/2026-08-07-swuart-v3.md Task 8 section for
 *   the full write-up):
 *
 *   1. An MPLAB SIM SCL stimulus process (`RC2 <= RC1;` on `wait on
 *      RC1;` or on a tight `wait for N ic;` poll) never registered a
 *      CCP1 capture at all: `h.rx_state` stayed `RX_IDLE` for the
 *      whole run. `scripts/sim-mdb-run.sh`'s fixed
 *      device/program/run/wait/halt shape also has no slot to load a
 *      `.scl` file before `run` in the first place (the one hook it
 *      exposes, `extra_mdb`, only runs after `halt`), so using this
 *      approach at all would have meant extending a script this
 *      task's brief does not list, on top of the capture never
 *      firing.
 *   2. A breakpoint at the real `CCP2_IRQHandler` entry, hit once per
 *      bit period with a `write pin RC2 <level>` matching whatever
 *      `tx_compare_event` had just placed on RC1 (verified bit-exact
 *      against `PORTC` at all 10 real hardware events for this exact
 *      byte before wiring up RC2 at all), did make CCP1 capture a
 *      real edge and run the whole confirm/sample/stop chain with
 *      `error_count` staying 0, i.e. genuinely exercised the real RX
 *      hardware path end to end. But the byte that came out
 *      (`h.rx_shift`, later `h.rx_ring[0]`) did not match what went
 *      in, and not by any single-bit or shift/reverse pattern that
 *      would point at an off-by-one in the write sequence itself
 *      (which the PORTC trace had already ruled out). Chasing the
 *      remaining timing gap between mdb's own pin-write latency and
 *      the CCP1 sample deadlines any further was not worth this
 *      task's remaining budget, and `scripts/sim-mdb-run.sh` still
 *      has no slot for per-bit breakpoint scripting like this even if
 *      it had worked.
 *
 *   So: TX-only, matching the task brief's own pre-approved fallback.
 *   `h.tx_count` (see epic_swuart.h; a peek at the handle's internals
 *   the same way `epic-swuart/tests/test_swuart_dual.c`'s own
 *   "channel A finished transmitting" check already does) reaches 0
 *   the moment `EPIC_SWUART_Write` dequeues the byte into the active
 *   shift register, before the real CCP hardware has toggled a single
 *   bit; the SIM_ITERATIONS budget below still lets those hardware
 *   compare events actually run (proven separately, byte- and bit-
 *   exact, via the PORTC trace mentioned above), so a hang or a
 *   crash taking down the CCP2 ISR path would still fail this build
 *   by never reaching epic_harness_report at all. What this build
 *   does NOT prove is that a byte written on real hardware is
 *   received correctly on real hardware; that remains open (see
 *   docs/API.md's disclosure once Task 9 lands it).
 */
#include "epic_swuart.h"
#include "core/epic_harness.h"

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

/** Loop-iteration bound, not a real time unit (see
 *  core/epic_harness.h). 200000 confirmed (by hand, under `mdb`) to
 *  finish and reach epic_harness_report well inside a 60000 ms
 *  wait_ms budget on PIC16F877A/MPLAB SIM. */
#define SIM_ITERATIONS 200000UL

#define TEST_BYTE 0x41u

int main(void)
{
    epic_harness_init(SIM_ITERATIONS);

    EPIC_SWUART_HandleTypeDef h;
    EPIC_SWUART_Init(&h, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_2, FOSC_HZ, 9600u);

    uint8_t tx_byte = TEST_BYTE;
    size_t queued = EPIC_SWUART_Write(&h, &tx_byte, 1);

    int drained = 0;
    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
        EPIC_WDT_Refresh();
        if (!drained && h.tx_count == 0u) {
            drained = 1;
        }
    }

    epic_harness_log((queued == 1u && drained)
                          ? "swuart sim: tx drained\n"
                          : "swuart sim: tx did not drain\n");
    return epic_harness_report(queued == 1u && drained);
}
