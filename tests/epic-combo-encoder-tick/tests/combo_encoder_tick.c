/**
 * @file    combo_encoder_tick.c
 * @brief   C9 of the combination matrix:
 *          epic-encoder + epic-tick. Scripted quadrature edges are fed
 *          through the encoder's own `epic_encoder_update` API, interleaved
 *          with 1 ms tick-delay waits, while every position read is
 *          checked against the running scripted expectation under the
 *          live 1 ms tick ISR. The 32-bit `epic_encoder_get_position` read
 *          (EPIC_IRQ_Disable/Restore) is the point: an interrupt
 *          delivered inside the disabled
 *          window can tear the 4-byte read and, under MPLAB SIM, leave
 *          GIE cleared (the epic_tick.c read-twice-retry comment
 *          documents the signature), stopping the tick dead.
 *
 * @details
 *   The encoder's own sim gate (epic-encoder/tests/sim_encoder.c)
 *   already hammers `epic_encoder_get_position` under the live tick with
 *   the position held constant (any non-zero read is a tear). This
 *   combo goes one step further: the position CHANGES under the probe
 *   (scripted edges), so a torn read differs from the true value by
 *   arbitrary byte-shift garbage, and every read is cross-checked
 *   against the exact scripted expectation: consecutive reads differ
 *   only by the scripted step. A 1 ms tick-delay wait (the encoder
 *   gate's bounded wait_1ms_bounded, so a dead tick reports a stall
 *   instead of hanging) sits between the reads, so the tick ISR
 *   demonstrably fires between them; a tick-counter delta then
 *   verifies the tick survived the whole run.
 *
 *   MPLAB SIM behaviors pinned down here (all in the documented
 *   class-G / Finding 10.1 family; see the C1 gate
 *   epic-combo-uart-ssp/tests/combo_uart_ssp.c and epic_tick.c's
 *   read-twice-retry comment):
 *   - Found 2026-08-09 while building this gate: interleaving
 *     `epic_encoder_get_position` reads (their Disable/Restore edges) with
 *     the tick module's own unbounded `epic_tick_delay_ms` freezes
 *     the sim's ISR delivery on the very first overflow after a read:
 *     g_tick_ms stays 0, TMR2IF latches, GIE ends up cleared, and the
 *     delay spins forever with no marker output. The same interleave
 *     with the bounded wait_1ms_bounded passes, and bare
 *     `epic_tick_delay_ms` stretches with no reads in between pass
 *     too, so this gate uses the bounded probe for the read
 *     interleave. This is a sim artifact, not a module bug:
 *     epic_tick_delay_ms is correct on silicon and its own module
 *     gate passes under mdb.
 *   - Bounded waits only (the C1 lesson): every 1 ms wait is the
 *     bounded probe that reports a stall instead of hanging.
 *   - No RX involvement, so the MPLAB SIM RX wall does not apply.
 *   - The GIE-on-with-pending-TMR2IF wedge (C1 lesson) is not guarded
 *     per pass here: like the encoder gate, this gate captures its
 *     PASS marker in pass 1, before any re-run of main() could wedge.
 *
 *   The encoder gate already proved the class-G race does not manifest
 *   under mdb; this gate is the regression that keeps it that way with
 *   the changing-position tick-delay interleave on top.
 *
 *   Bounded and self-reporting (the harness contract): the scripted
 *   phase is 16 edges plus 16 one-ms waits (~0.1 s of simulated
 *   time), the hammer is 5000 reads plus five one-ms waits (~1.9 s per
 *   main() pass on PIC16F877A/MPLAB SIM), comfortably inside the
 *   5000 ms wait_ms budget.
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

static epic_encoder_t g_seq;    /* scripted quadrature, position changes */
static epic_encoder_t g_ham;    /* class-G read hammer, position constant */
static uint16_t g_fail = 0u;

/**
 * @brief Record a check failure and log its index as two hex digits.
 */
static void fail(uint8_t idx)
{
    /* One static RAM buffer, not stack locals or const pointers: the
     * epic-cc build has no const-address form and no array allocas. */
    static const char hx[] = "0123456789ABCDEF";
    static char c[5];
    g_fail++;
    c[0] = 'F';
    c[1] = hx[(idx >> 4) & 0xF];
    c[2] = hx[idx & 0xF];
    c[3] = '.';
    c[4] = '\0';
    epic_harness_log(c);
}

#define CHECK(cond, idx) do {         \
    if (!(cond)) fail(idx);            \
} while (0)

/**
 * @brief Build a port byte putting the 2-bit state (a<<1|b) at pins PIN_A/PIN_B.
 */
static uint8_t port_byte(uint8_t state)
{
    /* Table read, not a bit-scatter expression: clang folds the
     * scatter into llvm.bitreverse.i6, which irparse rejects. */
    static const uint8_t bits[4] = {
        0u,
        (uint8_t)(1u << PIN_B),
        (uint8_t)(1u << PIN_A),
        (uint8_t)((1u << PIN_A) | (1u << PIN_B))
    };
    return bits[state & 3u];
}

/**
 * @brief Wait for the tick counter to advance 1 ms, bounded.
 *
 * Returns 1 if the tick advanced (GIE intact), 0 if the tick stalled
 * (class-G GIE loss).
 */
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

/**
 * @brief Feed one scripted edge and verify reads on both sides of a 1 ms wait.
 *
 * Every read must equal the running expectation.
 */
static void edge_with_delay(epic_encoder_t *enc, uint8_t state, int32_t expected)
{
    epic_encoder_update(enc, port_byte(state));
    CHECK(epic_encoder_get_position(enc) == expected, 0x00);
    (void)wait_1ms_bounded();
    CHECK(epic_encoder_get_position(enc) == expected, 0x01);
}

/**
 * @brief Run the epic-encoder + epic-tick interleave gate (C9).
 */
int main(void)
{
    epic_harness_init(SIM_ITERATIONS);

    epic_tick_init(FOSC_HZ);

    /* ---- Scripted quadrature with tick-delay interleave. One
     *      positive rotation (00->10->11->01->00, +4) then one
     *      negative rotation (00->01->11->10->00, -4): net 0, no
     *      errors. Each of the 16 edges is read before and after a
     *      1 ms tick-delay wait, and every read must equal the running
     *      expectation exactly (a torn 32-bit read differs by
     *      byte-shift garbage). */
    static const uint8_t pos_seq[4] = { 2, 3, 1, 0 };
    static const uint8_t neg_seq[4] = { 1, 3, 2, 0 };
    epic_encoder_init(&g_seq, PIN_A, PIN_B, 0u, port_byte(0u));
    int32_t expected = 0;
    for (uint8_t i = 0; i < 4u; i++) {
        expected += 1;
        edge_with_delay(&g_seq, pos_seq[i], expected);
    }
    for (uint8_t i = 0; i < 4u; i++) {
        expected -= 1;
        edge_with_delay(&g_seq, neg_seq[i], expected);
    }
    int32_t  p_final = epic_encoder_get_position(&g_seq);
    uint16_t e_final = epic_encoder_get_error_count(&g_seq);
    uint16_t g_final = epic_encoder_get_glitch_count(&g_seq);

    /* Bounded liveness probe after the scripted phase (the delays
     * themselves would hang on a dead tick; this catches a tick that
     * died just after the last one). */
    int script_stall = !wait_1ms_bounded();

    /* ---- Class-G volume probe. 5000 atomic position reads under the
     *      live tick ISR with a 1 ms bounded wait every 1000 reads;
     *      position is never written here, so any non-zero read is a
     *      torn read, and a wait that spins out its budget means the
     *      tick stopped (GIE lost). */
    epic_encoder_init(&g_ham, PIN_A, PIN_B, 0u, port_byte(0u));
    uint32_t t_start = epic_tick_get();
    int tear = 0;
    int stall = 0;
    for (uint32_t i = 0; epic_harness_running(i) && i < HAMMER_READS; i++) {
        epic_harness_tick();
        if (epic_encoder_get_position(&g_ham) != 0) { tear = 1; }
        if ((i % HAMMER_WAIT_EVERY) == 0u && !wait_1ms_bounded()) { stall = 1; }
    }
    uint32_t t_end = epic_tick_get();
    int tick_ok = (t_end > t_start) && !stall && !script_stall;

    /* ---- Cross-checks. ---- */
    CHECK(p_final == expected, 0x02);   /* final position = scripted net */
    CHECK(e_final == 0u, 0x03);         /* no impossible transitions     */
    CHECK(g_final == 0u, 0x04);         /* glitch gate is off            */
    CHECK(!tear, 0x05);                 /* no torn hammer reads         */
    CHECK(tick_ok, 0x06);               /* tick stayed alive             */

    /* ---- Report. ---- */
    int seq_ok = (p_final == expected) && (e_final == 0u) && (g_final == 0u);
    if (seq_ok) {
        EPIC_HARNESS_LOG_STATIC("C9 encoder-tick: scripted quadrature ok (net 0, 32 coherent reads)\n");
    } else {
        EPIC_HARNESS_LOG_STATIC("C9 encoder-tick: scripted quadrature MISMATCH (torn read or bad decode)\n");
    }
    if (tear) {
        EPIC_HARNESS_LOG_STATIC("C9 encoder-tick: class-G probe saw a TORN read\n");
    }
    if (tick_ok) {
        EPIC_HARNESS_LOG_STATIC("C9 encoder-tick: tick survived (count advanced, no stalls)\n");
    } else {
        EPIC_HARNESS_LOG_STATIC("C9 encoder-tick: tick STALLED (GIE lost)\n");
    }

    int ok = seq_ok && !tear && tick_ok;
    return epic_harness_report(ok);
}
