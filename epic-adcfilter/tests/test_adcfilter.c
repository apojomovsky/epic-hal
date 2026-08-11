/**
 * Host tests for the ADC oversampling and moving-average filter.
 * Zero HAL dependency, pure host; tests the exact code that ships.
 */

#include "epic_adcfilter.h"
#include <stdio.h>

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m) do { if (c) { g_pass++; } else { printf("FAIL: %s\n", m); g_fail++; } } while (0)

static uint16_t g_mock_val;
static int      g_mock_calls;
static uint16_t mock_const(void *ctx) { (void)ctx; g_mock_calls++; return g_mock_val; }

static uint16_t g_alt_a, g_alt_b;
static int      g_alt_idx;
static uint16_t mock_alternate(void *ctx) {
    (void)ctx; g_mock_calls++;
    uint16_t v = (g_alt_idx % 2 == 0) ? g_alt_a : g_alt_b;
    g_alt_idx++;
    return v;
}

static void test_oversample_constant(void)
{
    g_mock_val = 512;
    for (uint8_t eb = 0; eb <= 4; eb++) {
        g_mock_calls = 0;
        uint16_t r = epic_adcfilter_oversample(mock_const, NULL, eb);
        uint32_t expected_calls = 1UL << (eb * 2u);
        /* Oversample-and-decimate scales by 2^eb: 512 with eb=2 -> 2048
         * (a 12-bit mid-scale reading, same physical value, more resolution). */
        CHECK(r == (uint16_t)(512u << eb), "oversample constant: result == 512 << eb");
        CHECK((uint32_t)g_mock_calls == expected_calls, "oversample constant: call count");
    }
}

static void test_oversample_alternating(void)
{
    g_alt_a = 0; g_alt_b = 1023; g_alt_idx = 0; g_mock_calls = 0;
    uint16_t r = epic_adcfilter_oversample(mock_alternate, NULL, 2); /* 16 samples */
    /* 8 zeros + 8 * 1023 = 8184, >> 2 = 2046 */
    CHECK(r == 2046, "oversample alternating: mean >> 2");
    CHECK(g_mock_calls == 16, "oversample alternating: 16 calls");

    g_alt_idx = 0; g_mock_calls = 0;
    r = epic_adcfilter_oversample(mock_alternate, NULL, 0); /* 1 sample */
    CHECK(g_mock_calls == 1, "oversample eb=0: 1 call");
    /* first sample is g_alt_a = 0 */
    CHECK(r == 0, "oversample eb=0: returns first sample");
}

static void test_oversample_eb0(void)
{
    g_mock_val = 777;
    g_mock_calls = 0;
    uint16_t r = epic_adcfilter_oversample(mock_const, NULL, 0);
    CHECK(r == 777, "eb=0: returns the single sample");
    CHECK(g_mock_calls == 1, "eb=0: exactly 1 call");
}

static void test_avg_warmup(void)
{
    uint16_t buf[4];
    epic_adcfilter_avg_t f;
    epic_adcfilter_avg_init(&f, buf, 4);

    /* Push 2 samples: average should be over 2, not 4 */
    CHECK(epic_adcfilter_avg_push(&f, 100) == 100, "avg warmup 1: avg == 100");
    CHECK(epic_adcfilter_avg_push(&f, 200) == 150, "avg warmup 2: avg == 150 (not 75)");
    CHECK(epic_adcfilter_avg_push(&f, 300) == 200, "avg warmup 3: avg == 200 (not 150)");
    CHECK(epic_adcfilter_avg_push(&f, 400) == 250, "avg warmup 4: avg == 250 (full window)");
}

static void test_avg_full_window(void)
{
    uint16_t buf[3];
    epic_adcfilter_avg_t f;
    epic_adcfilter_avg_init(&f, buf, 3);

    epic_adcfilter_avg_push(&f, 10);
    epic_adcfilter_avg_push(&f, 20);
    epic_adcfilter_avg_push(&f, 30);
    CHECK(f.filled == 3, "avg full: filled == 3");

    /* Window is [10, 20, 30], avg = 20. Push 40 -> evict 10 -> [40, 20, 30], avg = 30 */
    CHECK(epic_adcfilter_avg_push(&f, 40) == 30, "avg full: push 40 evicts 10");
    /* Push 50 -> evict 20 -> [40, 50, 30], avg = 40 */
    CHECK(epic_adcfilter_avg_push(&f, 50) == 40, "avg full: push 50 evicts 20");
}

static void test_avg_count1(void)
{
    uint16_t buf[1];
    epic_adcfilter_avg_t f;
    epic_adcfilter_avg_init(&f, buf, 1);
    CHECK(epic_adcfilter_avg_push(&f, 42) == 42, "avg count=1: returns sample");
    CHECK(epic_adcfilter_avg_push(&f, 99) == 99, "avg count=1: returns last sample");
}

static void test_avg_independence(void)
{
    uint16_t bufA[4], bufB[2];
    epic_adcfilter_avg_t fa, fb;
    epic_adcfilter_avg_init(&fa, bufA, 4);
    epic_adcfilter_avg_init(&fb, bufB, 2);

    epic_adcfilter_avg_push(&fa, 100);
    epic_adcfilter_avg_push(&fa, 200);
    epic_adcfilter_avg_push(&fb, 10);
    epic_adcfilter_avg_push(&fb, 20);

    CHECK(fa.sum == 300, "indep: fa sum == 300");
    CHECK(fb.sum == 30, "indep: fb sum == 30");
    CHECK(fa.filled == 2, "indep: fa filled == 2");
    CHECK(fb.filled == 2, "indep: fb filled == 2 (full)");
    /* fa avg = 150, fb avg = 15 */
    CHECK(epic_adcfilter_avg_push(&fa, 0) == 100, "indep: fa unaffected by fb");
    CHECK(epic_adcfilter_avg_push(&fb, 0) == 10, "indep: fb unaffected by fa");
}

int main(void)
{
    test_oversample_constant();
    test_oversample_alternating();
    test_oversample_eb0();
    test_avg_warmup();
    test_avg_full_window();
    test_avg_count1();
    test_avg_independence();

    printf("test_adcfilter: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
