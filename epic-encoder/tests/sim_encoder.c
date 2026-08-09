/**
 * @file    sim_encoder.c
 * @brief   Bounded, self-reporting HARNESS=sim build: epic-encoder's
 *          real `mdb` gate (PIC16F877A/MPLAB SIM). Feeds a scripted
 *          quadrature sequence through the module's own
 *          `encoder_update` API (the module decodes a caller-supplied
 *          port byte, so unlike a GPIO-pin module there is no MPLAB
 *          SIM pin-injection limitation), verifying the x4 position
 *          count in both directions, the impossible-transition error
 *          counter, and the glitch gate against real simulated time.
 *          Then hammers `encoder_get_position` (the 32-bit read under
 *          EPIC_IRQ_Disable/Restore, docs/toolchain-coverage.md class
 *          G) under the live 1 ms tick ISR, checking that every read
 *          is consistent and that the tick survived (a latched
 *          interrupt delivered inside the GIE=0 critical section can
 *          tear the read and leave GIE cleared, stopping the tick
 *          dead, the exact signature epic-tick's sim gate froze with;
 *          see epic_tick.c's read-twice-retry comment). Reports
 *          PASS/FAIL over the target's real USART the same way every
 *          other family's own `.sim` variant does (see
 *          pic16f87xa-hal/src/core/pic16_harness_sim_target.c).
 *
 * @details
 *   Phase 1 replays the host tests' (tests/test_encoder.c) two full
 *   clean rotations: states 00->01->11->10->00 twice (-8, the shipped
 *   table's negative direction) then 00->10->11->01->00 twice (+8),
 *   ending at position 0 with no errors or glitches. Phase 2 feeds the
 *   impossible 00->11 transition (both bits flipped: error_count 1,
 *   position unchanged) then a valid 11->01 edge (resync proof, +1).
 *   Phase 3 arms the glitch gate (min_edge_interval_ms = 10) on a
 *   fresh instance and replays the host glitch test against real tick
 *   time: first edge accepted, a too-soon second edge dropped
 *   (glitch_count 1, position unchanged), the same edge accepted after
 *   the window (position -2). Phase 4 is the class-G probe: 5000
 *   `encoder_get_position` reads under the live tick ISR with a
 *   1 ms wait every 1000 reads so the ISR demonstrably fires inside
 *   the probe window; position is never written during the probe, so
 *   any non-zero read is a torn read, and a bounded 1 ms wait that
 *   spins out its budget means the tick stopped (GIE lost).
 *
 *   Loop-iteration bounds: phase 3 waits ~24 ms and the hammer is
 *   5000 reads plus five one-ms waits, roughly 0.6M instructions at
 *   20 MHz; comfortably inside the 5000 ms wait_ms budget on
 *   PIC16F877A/MPLAB SIM (20000 reads was ~7 s per main() pass and
 *   blew the budget). Every wait and loop is bounded so the build
 *   always terminates and reaches epic_harness_report.
 */

#include "encoder.h"
#include "epic_tick.h"
#include "core/epic_harness.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define PIN_A 4u
#define PIN_B 5u

#define HAMMER_READS      5000UL
#define HAMMER_WAIT_EVERY 1000UL
/** Bound for a 1 ms tick wait (~5000 instructions when healthy); only
 *  the class-G failure path (dead tick) ever gets near it. */
#define WAIT_SPIN_BOUND   100000UL
/** Harness iteration budget: the sim target's epic_harness_running is
 *  `iteration < cycles` (0 would make every harness loop a no-op), so
 *  this must exceed HAMMER_READS. */
#define SIM_ITERATIONS    100000UL

static encoder_t g_dec;    /* phase 1+2: decode + impossible transition */
static encoder_t g_gate;   /* phase 3:   glitch gate against real time   */
static encoder_t g_ham;    /* phase 4:   class-G read hammer             */

/* Build a port byte putting the 2-bit state (a<<1|b) at pins PIN_A/PIN_B. */
static uint8_t port_byte(uint8_t state)
{
    uint8_t v = 0U;
    if (state & 0x2U) v |= (uint8_t)(1U << PIN_A);
    if (state & 0x1U) v |= (uint8_t)(1U << PIN_B);
    return v;
}

/* Wait for the tick counter to advance 1 ms, bounded. Returns 1 if the
 * tick advanced (GIE intact), 0 if the tick stalled (class-G GIE loss). */
static uint8_t wait_1ms_bounded(void)
{
    uint32_t t0 = epic_tick_get();
    uint32_t spins = 0u;
    while (epic_tick_elapsed_since(t0) < 1u) {
        epic_harness_tick();
        if (++spins >= WAIT_SPIN_BOUND) { return 0u; }
    }
    return 1u;
}

int main(void)
{
    epic_harness_init(SIM_ITERATIONS);
    epic_tick_init(FOSC_HZ);

    /* ---- Phase 1: two full rotations each way, gate off (host test
     *      sequence verbatim). -8 then +8 -> back to 0, no errors. */
    static const uint8_t neg_seq[8] = { 1, 3, 2, 0, 1, 3, 2, 0 };
    static const uint8_t pos_seq[8] = { 2, 3, 1, 0, 2, 3, 1, 0 };
    encoder_init(&g_dec, PIN_A, PIN_B, 0u, port_byte(0u));
    for (uint8_t i = 0; i < 8u; i++) {
        encoder_update(&g_dec, port_byte(neg_seq[i]));
    }
    for (uint8_t i = 0; i < 8u; i++) {
        encoder_update(&g_dec, port_byte(pos_seq[i]));
    }
    int32_t p_dec   = encoder_get_position(&g_dec);
    uint16_t e_dec  = encoder_get_error_count(&g_dec);
    uint16_t g_dec0 = encoder_get_glitch_count(&g_dec);

    /* ---- Phase 2: impossible 00->11 (both bits flip), then resync. */
    encoder_update(&g_dec, port_byte(3u));   /* 00->11: error, no count */
    int32_t p_imp = encoder_get_position(&g_dec);
    uint16_t e_imp = encoder_get_error_count(&g_dec);
    encoder_update(&g_dec, port_byte(1u));   /* 11->01: valid +1 edge  */
    int32_t p_resync = encoder_get_position(&g_dec);

    /* ---- Phase 3: glitch gate on real tick time (host test script). */
    encoder_init(&g_gate, PIN_A, PIN_B, 10u, port_byte(0u));
    for (uint8_t i = 0; i < 10u; i++) wait_1ms_bounded();  /* clear seed gate */
    encoder_update(&g_gate, port_byte(1u));                /* 00->01 accepted */
    int32_t pg_first = encoder_get_position(&g_gate);
    wait_1ms_bounded();
    wait_1ms_bounded();                                    /* +2 ms          */
    encoder_update(&g_gate, port_byte(3u));                /* 01->11 too soon */
    uint16_t gg_drop = encoder_get_glitch_count(&g_gate);
    int32_t pg_drop  = encoder_get_position(&g_gate);
    for (uint8_t i = 0; i < 12u; i++) wait_1ms_bounded();  /* +12 ms         */
    encoder_update(&g_gate, port_byte(3u));                /* 01->11 accepted */
    int32_t pg_accept = encoder_get_position(&g_gate);
    uint16_t gg_after = encoder_get_glitch_count(&g_gate);
    uint16_t eg_gate  = encoder_get_error_count(&g_gate);

    /* ---- Phase 4: class-G probe. Hammer the 32-bit position read
     *      under the live tick ISR. Position is never written here, so
     *      any non-zero read is a torn read; a wait that spins out its
     *      budget means the tick stopped (GIE left cleared). */
    encoder_init(&g_ham, PIN_A, PIN_B, 0u, port_byte(0u));
    uint32_t t_start = epic_tick_get();
    int tear = 0;
    int stall = 0;
    for (uint32_t i = 0; epic_harness_running(i) && i < HAMMER_READS; i++) {
        epic_harness_tick();
        if (encoder_get_position(&g_ham) != 0) { tear = 1; }
        if ((i % HAMMER_WAIT_EVERY) == 0u && !wait_1ms_bounded()) { stall = 1; }
    }
    uint32_t t_end = epic_tick_get();
    int tick_ok = (t_end > t_start) && !stall;

    /* ---- Report. ---- */
    int phase1_ok = (p_dec == 0) && (e_dec == 0) && (g_dec0 == 0);
    int phase2_ok = (p_imp == 0) && (e_imp == 1) && (p_resync == 1);
    int phase3_ok = (pg_first == -1) && (gg_drop == 1) && (pg_drop == -1) &&
                    (pg_accept == -2) && (gg_after == 1) && (eg_gate == 0);

    if (phase1_ok) {
        epic_harness_log("encoder sim: rotations ok (-8 then +8, no errors)\n");
    } else {
        epic_harness_log("encoder sim: rotation decode MISMATCH\n");
    }
    if (phase2_ok) {
        epic_harness_log("encoder sim: impossible transition + resync ok\n");
    } else {
        epic_harness_log("encoder sim: impossible transition MISMATCH\n");
    }
    if (phase3_ok) {
        epic_harness_log("encoder sim: glitch gate ok (window respected)\n");
    } else {
        epic_harness_log("encoder sim: glitch gate MISMATCH\n");
    }
    if (tear) {
        epic_harness_log("encoder sim: class-G probe saw a TORN read\n");
    }
    if (tick_ok) {
        epic_harness_log("encoder sim: class-G probe: tick survived\n");
    } else {
        epic_harness_log("encoder sim: class-G probe: tick STALLED (GIE lost)\n");
    }

    int ok = phase1_ok && phase2_ok && phase3_ok && !tear && tick_ok;
    return epic_harness_report(ok);
}
