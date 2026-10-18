/**
 * Host tests for lib/tick: delay, elapsed and counter-wraparound paths
 * through the host-sim Timer2 backend (real ISR, pumped by
 * epic_harness_tick). The 32-bit counter itself is file-static, so the
 * wraparound case drives epic_tick_elapsed_since with a synthetic
 * future timestamp instead of pumping 2^32 ticks.
 */

#include "epic_tick.h"
#include "core/epic_harness.h"

#include <stdint.h>
#include <stdio.h>

#define FOSC_HZ 20000000UL

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m) do { if (c) { g_pass++; } else { printf("FAIL: %s\n", m); g_fail++; } } while (0)

/** @brief  Fresh init leaves the tick counter at zero. */
static void test_get_starts_at_zero(void)
{
    CHECK(epic_tick_get() == 0u, "tick counter starts at zero");
}

/** @brief  Back-to-back reads agree when no pump runs between them. */
static void test_get_stable_without_pump(void)
{
    uint32_t a = epic_tick_get();
    uint32_t b = epic_tick_get();
    CHECK(a == b, "consecutive tick reads agree");
}

/** @brief  The counter never runs backwards across pumped ticks. */
static void test_get_monotonic(void)
{
    uint32_t prev = epic_tick_get();
    for (int i = 0; i < 5; i++)
    {
        epic_tick_delay_ms(1u);
        uint32_t now = epic_tick_get();
        if (now < prev)
        {
            CHECK(0, "tick counter is monotonic");
            return;
        }
        prev = now;
    }
    CHECK(1, "tick counter is monotonic");
}

/** @brief  A zero delay returns without advancing the counter. */
static void test_delay_zero(void)
{
    uint32_t t0 = epic_tick_get();
    epic_tick_delay_ms(0u);
    CHECK(epic_tick_get() - t0 == 0u, "delay(0) advances zero ticks");
}

/** @brief  delay(N) advances at least N ticks, overshooting by one at most. */
static void test_delay_bounded(uint32_t ms)
{
    uint32_t t0 = epic_tick_get();
    epic_tick_delay_ms(ms);
    uint32_t e = epic_tick_get() - t0;
    CHECK(e >= ms, "delay reaches the requested count");
    CHECK(e <= ms + 1u, "delay overshoots by at most one tick");
}

/** @brief  elapsed_since matches get()-t0 and tracks a pumped delay. */
static void test_elapsed_since_basic(void)
{
    uint32_t s = epic_tick_get();
    CHECK(epic_tick_elapsed_since(s) == 0u, "elapsed at the timestamp is zero");
    epic_tick_delay_ms(5u);
    uint32_t e = epic_tick_elapsed_since(s);
    CHECK(e >= 5u && e <= 6u, "elapsed covers a 5 ms delay");
    CHECK(e == epic_tick_get() - s, "elapsed matches get()-t0");
}

/** @brief  Unsigned wrap: a future t0 reads as 2^32 ahead, a past one exact. */
static void test_elapsed_wraparound(void)
{
    uint32_t now = epic_tick_get();
    CHECK(epic_tick_elapsed_since(now + 10u) == (uint32_t)(0u - 10u),
          "elapsed wraps for a future timestamp");
    CHECK(epic_tick_elapsed_since(now - 3u) == 3u,
          "elapsed is exact for a past timestamp");
    CHECK(epic_tick_elapsed_since(now + 0x80000000u) == 0x80000000u,
          "elapsed wraps at the half-range boundary");
}

/** @brief  Run all tick scenarios and report pass/fail counts. */
int main(void)
{
    epic_harness_init(4000000UL);
    epic_tick_init(FOSC_HZ);

    test_get_starts_at_zero();
    test_get_stable_without_pump();
    test_get_monotonic();
    test_delay_zero();
    test_delay_bounded(1u);
    test_delay_bounded(10u);
    test_elapsed_since_basic();
    test_elapsed_wraparound();

    printf("test_tick: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
