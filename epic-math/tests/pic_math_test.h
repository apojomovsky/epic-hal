/*
 * Tiny shared host-test harness for epic-math, no external framework: a
 * CHECK macro, a fixed-seed deterministic LCG for reproducible
 * randomized tests, and a pass/fail exit-code reporter. Host-only; not
 * built by the XC8 target Makefiles.
 */

#ifndef PIC_MATH_TEST_H
#define PIC_MATH_TEST_H

#include <stdint.h>
#include <stdio.h>

/** Running failure count, incremented by CHECK. */
static int g_pic_math_failures = 0;

/**
 * @brief Assert @p cond; on failure log the file/line/message and bump the
 *        failure count. Evaluates @p cond exactly once.
 */
#define CHECK(cond, msg)                                       \
    do {                                                       \
        if (!(cond)) {                                         \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); \
            g_pic_math_failures++;                             \
        }                                                      \
    } while (0)

/** @brief Map the failure count to a process exit code (0=pass, 1=fail). */
static inline int pic_math_test_report(void)
{
    return (g_pic_math_failures == 0) ? 0 : 1;
}

/**
 * @brief Fixed-seed 32-bit LCG (Numerical Recipes constants) for
 *        reproducible randomized tests. Returns the next 32-bit value.
 *        Not a quality RNG -- just deterministic fuzz.
 */
static inline uint32_t pic_math_test_rand(uint32_t *state)
{
    *state = (1664525u * (*state) + 1013904223u);
    return *state;
}

#endif /* PIC_MATH_TEST_H */
