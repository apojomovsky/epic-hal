/**
 * @file    test_encoder.c
 * @brief   Host tests for the quadrature decoder, exercising the exact code
 *          that ships on-target.
 *
 * @details
 *   Glitch-gate cases use `pic8_tick_init` + `pic8_harness_tick()` to
 *   advance real simulated time; pure-decode cases drive `encoder_update`
 *   directly with constructed port bytes. Sign convention: `QUAD_TABLE`
 *   counts `00->10->11->01->00` positive; swap `pin_a`/`pin_b` at init to
 *   invert (see docs/API.md).
 */

#include "encoder.h"
#include "pic8_tick.h"
#include "core/pic8_harness.h"

#include <stdio.h>

#define FOSC_HZ  20000000UL

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m) do { if (c) { g_pass++; } else { printf("FAIL: %s\n", m); g_fail++; } } while (0)

/* ---- time advancement (pump until the tick counter increments by 1) ---- */
static void advance_one_tick(void)
{
    uint32_t t0 = pic8_tick_get();
    while (pic8_tick_get() == t0) { pic8_harness_tick(); }
}
static void advance_ms(uint32_t ms) { for (uint32_t i = 0; i < ms; i++) advance_one_tick(); }

/* ---- port-byte helpers ---- */

/* Put a 2-bit (a<<1)|b `state` at bit positions pin_a/pin_b of a port byte. */
static uint8_t port_byte(uint8_t pin_a, uint8_t pin_b, uint8_t state)
{
    uint8_t a = (uint8_t)((state >> 1) & 1U);
    uint8_t b = (uint8_t)(state & 1U);
    uint8_t v = 0U;
    if (a) v |= (uint8_t)(1U << pin_a);
    if (b) v |= (uint8_t)(1U << pin_b);
    return v;
}

/* Two instances on one port byte: state A in pins 4/5, state B in pins 6/7. */
static uint8_t port_byte2(uint8_t state_a, uint8_t state_b)
{
    return (uint8_t)(port_byte(4, 5, state_a) | port_byte(6, 7, state_b));
}

#define PIN_A  4u
#define PIN_B  5u

/* ================================================================ */
/* 1+2. Full clean rotations, both directions, against QUAD_TABLE.    */
/* ================================================================ */

/* 00->01->11->10->00 x2: the shipped table counts this direction -4/cycle. */
static void test_full_rotation_neg(void)
{
    encoder_t enc;
    encoder_init(&enc, PIN_A, PIN_B, 0, port_byte(PIN_A, PIN_B, 0));

    static const uint8_t seq[8] = { 1, 3, 2, 0, 1, 3, 2, 0 };
    for (int i = 0; i < 8; i++)
        encoder_update(&enc, port_byte(PIN_A, PIN_B, seq[i]));

    CHECK(encoder_get_position(&enc) == -8, "rot_neg: -8 over 2 cycles");
    CHECK(encoder_get_error_count(&enc) == 0, "rot_neg: no errors");
    CHECK(encoder_get_glitch_count(&enc) == 0, "rot_neg: no glitches (gate off)");
}

/* 00->10->11->01->00 x2: the opposite direction, +4/cycle. */
static void test_full_rotation_pos(void)
{
    encoder_t enc;
    encoder_init(&enc, PIN_A, PIN_B, 0, port_byte(PIN_A, PIN_B, 0));

    static const uint8_t seq[8] = { 2, 3, 1, 0, 2, 3, 1, 0 };
    for (int i = 0; i < 8; i++)
        encoder_update(&enc, port_byte(PIN_A, PIN_B, seq[i]));

    CHECK(encoder_get_position(&enc) == 8, "rot_pos: +8 over 2 cycles");
    CHECK(encoder_get_error_count(&enc) == 0, "rot_pos: no errors");
    CHECK(encoder_get_glitch_count(&enc) == 0, "rot_pos: no glitches (gate off)");
}

/* ================================================================ */
/* 3. Impossible transition (both bits flip at once).                */
/* ================================================================ */
static void test_impossible_transition(void)
{
    encoder_t enc;
    encoder_init(&enc, PIN_A, PIN_B, 0, port_byte(PIN_A, PIN_B, 0));  /* last_state=00 */

    /* 00 -> 11: both bits flipped, QUAD_TABLE[3] = 0 -> error, no count. */
    encoder_update(&enc, port_byte(PIN_A, PIN_B, 3));

    CHECK(encoder_get_position(&enc) == 0, "impossible: position unchanged");
    CHECK(encoder_get_error_count(&enc) == 1, "impossible: error_count incremented");
    /* last_state DOES advance (resync to the new sample); a following valid
     * edge from 11 is decoded normally, not stuck against the old 00. */
    encoder_update(&enc, port_byte(PIN_A, PIN_B, 1));  /* 11 -> 01: QUAD_TABLE[13] = +1 */
    CHECK(encoder_get_position(&enc) == 1, "impossible: resync then valid edge counts");
    CHECK(encoder_get_error_count(&enc) == 1, "impossible: no second error");
}

/* ================================================================ */
/* 4. No-op on unchanged pins (another instance's pins changed).      */
/* ================================================================ */
static void test_noop_on_unchanged(void)
{
    encoder_t enc;
    encoder_init(&enc, PIN_A, PIN_B, 0, port_byte(PIN_A, PIN_B, 0));
    encoder_update(&enc, port_byte(PIN_A, PIN_B, 1));  /* 00->01, pos=-1 */

    /* Same port_value again: this instance's pins unchanged -> no-op. */
    encoder_update(&enc, port_byte(PIN_A, PIN_B, 1));

    CHECK(encoder_get_position(&enc) == -1, "noop: position unchanged");
    CHECK(encoder_get_error_count(&enc) == 0, "noop: no error");
    CHECK(encoder_get_glitch_count(&enc) == 0, "noop: no glitch");
}

/* ================================================================ */
/* 5. Glitch gate rejects a too-soon edge, then accepts after window. */
/* ================================================================ */
static void test_glitch_gate_rejects(void)
{
    const uint16_t gate = 5u;
    encoder_t enc;
    encoder_init(&enc, PIN_A, PIN_B, gate, port_byte(PIN_A, PIN_B, 0));
    /* last_edge_tick seeded at init = T0. Advance past the gate so the first
     * real edge is accepted (at T0 elapsed=0 < gate would be rejected). */
    advance_ms(gate);

    /* 00 -> 01: accepted (elapsed since init >= gate). */
    encoder_update(&enc, port_byte(PIN_A, PIN_B, 1));
    CHECK(encoder_get_position(&enc) == -1, "gate: first edge accepted");
    CHECK(encoder_get_glitch_count(&enc) == 0, "gate: no glitch yet");

    /* 01 -> 11 immediately, no time advanced: too soon -> rejected. */
    encoder_update(&enc, port_byte(PIN_A, PIN_B, 3));
    CHECK(encoder_get_position(&enc) == -1, "gate: too-soon edge dropped (pos same)");
    CHECK(encoder_get_glitch_count(&enc) == 1, "gate: glitch_count incremented");
    CHECK(enc.last_state == 1, "gate: last_state NOT advanced to rejected sample");

    /* Advance past the gate, repeat the same edge (01->11): now accepted. */
    advance_ms(gate);
    encoder_update(&enc, port_byte(PIN_A, PIN_B, 3));
    CHECK(encoder_get_position(&enc) == -2, "gate: edge accepted after window");
    CHECK(encoder_get_glitch_count(&enc) == 1, "gate: glitch_count unchanged on accept");
    CHECK(enc.last_state == 3, "gate: last_state advanced to accepted sample");
    CHECK(encoder_get_error_count(&enc) == 0, "gate: no impossible transitions");
}

/* ================================================================ */
/* 6. Glitch gate disabled by default (min_edge_interval_ms == 0).   */
/* ================================================================ */
static void test_glitch_gate_disabled(void)
{
    encoder_t enc;
    encoder_init(&enc, PIN_A, PIN_B, 0, port_byte(PIN_A, PIN_B, 0));

    /* Two transitions on consecutive calls, no time advancement at all. */
    encoder_update(&enc, port_byte(PIN_A, PIN_B, 1));  /* 00->01 */
    encoder_update(&enc, port_byte(PIN_A, PIN_B, 3));  /* 01->11 */

    CHECK(encoder_get_position(&enc) == -2, "nogate: both edges accepted");
    CHECK(encoder_get_glitch_count(&enc) == 0, "nogate: glitch_count stays 0");
    CHECK(encoder_get_error_count(&enc) == 0, "nogate: no errors");
}

/* ================================================================ */
/* 7. encoder_init / encoder_reset resync last_state from port_value. */
/* ================================================================ */
static void test_init_reset_resync(void)
{
    encoder_t enc;
    /* Init at state 10 (a=1,b=0). If last_state wrongly defaulted to 0, the
     * next edge 10->11 would be misread; resync makes 10->11 a valid step. */
    encoder_init(&enc, PIN_A, PIN_B, 0, port_byte(PIN_A, PIN_B, 2));
    CHECK(enc.last_state == 2, "resync: init seeds last_state from port");

    encoder_update(&enc, port_byte(PIN_A, PIN_B, 3));  /* 10->11: QUAD_TABLE[11]=+1 */
    CHECK(encoder_get_position(&enc) == 1, "resync: first edge valid, not impossible");
    CHECK(encoder_get_error_count(&enc) == 0, "resync: no spurious error on first edge");

    /* Reset resyncs to a fresh state and zeroes accumulators; gains unchanged. */
    encoder_reset(&enc, port_byte(PIN_A, PIN_B, 2));   /* back to state 10 */
    CHECK(encoder_get_position(&enc) == 0, "resync: reset zeroes position");
    CHECK(encoder_get_error_count(&enc) == 0, "resync: reset zeroes error_count");
    CHECK(encoder_get_glitch_count(&enc) == 0, "resync: reset zeroes glitch_count");
    CHECK(enc.last_state == 2, "resync: reset re-seeds last_state");
    CHECK(enc.pin_a == PIN_A && enc.pin_b == PIN_B, "resync: pins preserved");
    CHECK(enc.min_edge_interval_ms == 0, "resync: gate config preserved");

    encoder_update(&enc, port_byte(PIN_A, PIN_B, 3));  /* 10->11 valid again */
    CHECK(encoder_get_position(&enc) == 1, "resync: valid edge after reset, no error");
    CHECK(encoder_get_error_count(&enc) == 0, "resync: still no error after reset");
}

/* ================================================================ */
/* 8. Getters read back exactly what direct struct inspection shows. */
/* ================================================================ */
static void test_getters_match_struct(void)
{
    encoder_t enc;
    encoder_init(&enc, PIN_A, PIN_B, 0, port_byte(PIN_A, PIN_B, 0));
    /* A scripted sequence leaving known position / counters. */
    encoder_update(&enc, port_byte(PIN_A, PIN_B, 1));  /* 00->01, pos=-1 */
    encoder_update(&enc, port_byte(PIN_A, PIN_B, 3));  /* 01->11, pos=-2 */
    encoder_update(&enc, port_byte(PIN_A, PIN_B, 0));  /* 11->00: QUAD_TABLE[12]=0 -> error */

    CHECK(encoder_get_position(&enc) == enc.position,
          "getters: get_position matches struct");
    CHECK(encoder_get_error_count(&enc) == enc.error_count,
          "getters: get_error_count matches struct");
    CHECK(encoder_get_glitch_count(&enc) == enc.glitch_count,
          "getters: get_glitch_count matches struct");
    /* And the expected values from the script: -2, one error, no glitches. */
    CHECK(encoder_get_position(&enc) == -2, "getters: position value sanity");
    CHECK(encoder_get_error_count(&enc) == 1, "getters: error value sanity");
    CHECK(encoder_get_glitch_count(&enc) == 0, "getters: glitch value sanity");
}

/* ================================================================ */
/* 9. Two independent instances on one port byte never cross-affect.  */
/* ================================================================ */
static void test_two_instances_independent(void)
{
    encoder_t a, b;
    encoder_init(&a, 4, 5, 0, port_byte2(0, 0));   /* A on RB4/RB5 */
    encoder_init(&b, 6, 7, 0, port_byte2(0, 0));   /* B on RB6/RB7 */

    /* Interleaved: A rotates one direction (00->01->11->10->00 x2 = -8),
     * B rotates the other (00->10->11->01->00 x2 = +8), step by step on a
     * shared byte. Plus one step where only A changes (B must no-op) and
     * one where only B changes (A must no-op). */
    static const uint8_t sa_seq[8] = { 1, 3, 2, 0, 1, 3, 2, 0 };  /* A: -8 */
    static const uint8_t sb_seq[8] = { 2, 3, 1, 0, 2, 3, 1, 0 };  /* B: +8 */
    for (int i = 0; i < 8; i++) {
        uint8_t byte = port_byte2(sa_seq[i], sb_seq[i]);
        encoder_update(&a, byte);
        encoder_update(&b, byte);
    }

    CHECK(encoder_get_position(&a) == -8, "two: A counts its own rotation");
    CHECK(encoder_get_error_count(&a) == 0, "two: A no errors");
    CHECK(encoder_get_position(&b) == 8, "two: B counts its own rotation");
    CHECK(encoder_get_error_count(&b) == 0, "two: B no errors");

    /* Now a step where only A changes (00->01) and B holds (stays 0): B no-ops. */
    uint8_t byte = port_byte2(1, 0);
    encoder_update(&a, byte);   /* A: 00->01 accepted */
    encoder_update(&b, byte);   /* B: 00==00 no-op */
    CHECK(encoder_get_position(&a) == -9, "two: A advances alone");
    CHECK(encoder_get_position(&b) == 8, "two: B unchanged when A moves");
    CHECK(encoder_get_error_count(&b) == 0, "two: B no error from A's move");

    /* A step where only B changes (00->10) and A holds (stays 01): A no-ops. */
    byte = port_byte2(1, 2);
    encoder_update(&a, byte);   /* A: 01==01 no-op */
    encoder_update(&b, byte);   /* B: 00->10 accepted */
    CHECK(encoder_get_position(&a) == -9, "two: A unchanged when B moves");
    CHECK(encoder_get_position(&b) == 9, "two: B advances alone");
    CHECK(encoder_get_error_count(&a) == 0, "two: A no error from B's move");
}

/* ================================================================ */

int main(void)
{
    pic8_harness_init(2000000UL);
    pic8_tick_init(FOSC_HZ);

    test_full_rotation_neg();
    test_full_rotation_pos();
    test_impossible_transition();
    test_noop_on_unchanged();
    test_glitch_gate_rejects();
    test_glitch_gate_disabled();
    test_init_reset_resync();
    test_getters_match_struct();
    test_two_instances_independent();

    printf("test_encoder: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
