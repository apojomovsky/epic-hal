/**
 * HARNESS=sim build for epic-adcfilter: the module's `mdb` gate. Runs
 * the actual compiled epic_adcfilter.c under MPLAB SIM on a 16F877A,
 * feeding a scripted sample stream through the real API and checking
 * the host-test oracle values, then reports PASS/FAIL over the
 * target's real hardware USART (pic16_harness_sim_target.c). Pure
 * computation, no RX injection needed. The oracle, step-settling, and
 * reset contracts are enumerated inline at each check below.
 */

#include "epic_adcfilter.h"
#include "core/epic_harness.h"

#include <stddef.h>     /* NULL */

/** Bounded loop budget: all checks run before the loop; this only
 *  bounds the tail tick loop (a formality for a pure-compute gate). */
#define SIM_ITERATIONS 16UL

static uint16_t g_mock_val;
static uint16_t g_alt_a, g_alt_b;
static uint8_t  g_alt_idx;

static uint16_t mock_const(void *ctx)
{
    (void)ctx;
    return g_mock_val;
}

static uint16_t mock_alternate(void *ctx)
{
    (void)ctx;
    return (g_alt_idx++ & 1u) ? g_alt_b : g_alt_a;
}

int main(void)
{
    /* Phase A: documented oracle values (tests/test_adcfilter.c). */
    uint16_t buf4[4], buf3[3], buf1[1], bufA[4], bufB[2], buf8[8];
    epic_adcfilter_avg_t f, f3, f1, fa, fb, fs;
    uint32_t i;
    int ok;
    int a_ok = 1, b_ok = 1, c_ok = 1;

    epic_harness_init(SIM_ITERATIONS);

    /* (a1) oversample constant: 512 with eb=2 -> 512 << 2 = 2048. */
    g_mock_val = 512;
    if (epic_adcfilter_oversample(mock_const, NULL, 2) != 2048) { a_ok = 0; }

    /* (a2) oversample alternating: 8 x 0 + 8 x 1023 = 8184, >> 2 -> 2046. */
    g_alt_a = 0; g_alt_b = 1023; g_alt_idx = 0;
    if (epic_adcfilter_oversample(mock_alternate, NULL, 2) != 2046) { a_ok = 0; }

    /* (a3) eb=0: single read, returns the sample itself. */
    g_mock_val = 777;
    if (epic_adcfilter_oversample(mock_const, NULL, 0) != 777) { a_ok = 0; }

    /* (a4) warmup: average over what has been pushed, not the window. */
    epic_adcfilter_avg_init(&f, buf4, 4);
    if (epic_adcfilter_avg_push(&f, 100) != 100) { a_ok = 0; }
    if (epic_adcfilter_avg_push(&f, 200) != 150) { a_ok = 0; }
    if (epic_adcfilter_avg_push(&f, 300) != 200) { a_ok = 0; }
    if (epic_adcfilter_avg_push(&f, 400) != 250) { a_ok = 0; }

    /* (a5) full window: push 40 evicts 10 -> 30; push 50 evicts 20 -> 40. */
    epic_adcfilter_avg_init(&f3, buf3, 3);
    epic_adcfilter_avg_push(&f3, 10);
    epic_adcfilter_avg_push(&f3, 20);
    epic_adcfilter_avg_push(&f3, 30);
    if (f3.filled != 3) { a_ok = 0; }
    if (epic_adcfilter_avg_push(&f3, 40) != 30) { a_ok = 0; }
    if (epic_adcfilter_avg_push(&f3, 50) != 40) { a_ok = 0; }

    /* (a6) count=1: tracks the last sample. */
    epic_adcfilter_avg_init(&f1, buf1, 1);
    if (epic_adcfilter_avg_push(&f1, 42) != 42) { a_ok = 0; }
    if (epic_adcfilter_avg_push(&f1, 99) != 99) { a_ok = 0; }

    /* (a7) independence: two filters share no state. */
    epic_adcfilter_avg_init(&fa, bufA, 4);
    epic_adcfilter_avg_init(&fb, bufB, 2);
    epic_adcfilter_avg_push(&fa, 100);
    epic_adcfilter_avg_push(&fa, 200);
    epic_adcfilter_avg_push(&fb, 10);
    epic_adcfilter_avg_push(&fb, 20);
    if (fa.sum != 300 || fb.sum != 30) { a_ok = 0; }
    if (fa.filled != 2 || fb.filled != 2) { a_ok = 0; }
    if (epic_adcfilter_avg_push(&fa, 0) != 100) { a_ok = 0; }
    if (epic_adcfilter_avg_push(&fb, 0) != 10) { a_ok = 0; }

    /* Phase B: step settling within the documented window (count
     * pushes). Prefill an 8-deep window with 8 x 100, then step to
     * 8 x 1000: halfway (4th push) the average is
     * (4 x 1000 + 4 x 100) / 8 = 550; after the 8th push the old
     * values are fully evicted and the output is exactly 1000. */
    {
        uint16_t prev = 0;
        epic_adcfilter_avg_init(&fs, buf8, 8);
        for (i = 0; i < 8; i++) {
            epic_adcfilter_avg_push(&fs, 100);
        }
        for (i = 0; i < 8; i++) {
            uint16_t out = epic_adcfilter_avg_push(&fs, 1000);
            if (out < prev) { b_ok = 0; }  /* monotone toward the step */
            prev = out;
            if (i == 3u && out != 550) { b_ok = 0; }
        }
        if (prev != 1000) { b_ok = 0; }    /* settled within the window */
    }

    /* Phase C: reset path restores the initial state. */
    epic_adcfilter_avg_init(&fs, buf8, 8);
    if (fs.filled != 0 || fs.index != 0 || fs.sum != 0) { c_ok = 0; }
    /* First push after re-init: averaged over 1 sample, not /8. */
    if (epic_adcfilter_avg_push(&fs, 1000) != 1000) { c_ok = 0; }
    if (epic_adcfilter_avg_push(&fs, 2000) != 1500) { c_ok = 0; }
    /* Re-init with a different window length takes effect. */
    epic_adcfilter_avg_init(&fs, buf8, 2);
    if (fs.count != 2 || fs.filled != 0 || fs.sum != 0) { c_ok = 0; }
    if (epic_adcfilter_avg_push(&fs, 100) != 100) { c_ok = 0; }
    if (epic_adcfilter_avg_push(&fs, 200) != 150) { c_ok = 0; }
    if (fs.filled != 2) { c_ok = 0; }

    for (i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
    }

    ok = a_ok && b_ok && c_ok;
    epic_harness_log(a_ok ? "adcfilter sim: oracle values ok\n"
                          : "adcfilter sim: oracle values WRONG\n");
    epic_harness_log(b_ok ? "adcfilter sim: step settles in window\n"
                          : "adcfilter sim: step did NOT settle\n");
    epic_harness_log(c_ok ? "adcfilter sim: reset restores state\n"
                          : "adcfilter sim: reset did NOT restore\n");
    return epic_harness_report(ok);
}
