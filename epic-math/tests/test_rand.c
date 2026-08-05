/**
 * @file    test_rand.c
 * @brief   Host tests asserting `pic_math_rand_next`'s documented LFSR
 *          properties (maximal-length period, never returns 0, escapes a
 *          zero seed, two states advance independently) and a coarse
 *          bell-shape sanity check on `rand_gauss`, not a chi-square test.
 */

#include "pic_math.h"
#include "pic_math_test.h"

#define PIC_MATH_RAND_SEED 0xACE1u

static void test_rand_period_and_zero(void)
{
    /* Period: 65535 steps return to the seed, and 0 never appears. */
    uint16_t state = PIC_MATH_RAND_SEED;
    uint16_t first = pic_math_rand_next(&state);
    (void)first;
    uint32_t count = 1u;
    int saw_zero = 0;
    while (state != PIC_MATH_RAND_SEED) {
        uint16_t v = pic_math_rand_next(&state);
        if (v == 0u) saw_zero = 1;
        count++;
        if (count > 70000u) { CHECK(0, "period runaway"); break; }
    }
    CHECK(count == 65535u, "LFSR period is 2^16-1");
    CHECK(saw_zero == 0, "rand_next never returns 0 over the period");
}

static void test_rand_zero_state_escape(void)
{
    /* A zero state maps to the seed and is not stuck. */
    uint16_t state = 0u;
    uint16_t v1 = pic_math_rand_next(&state);
    CHECK(v1 != 0u, "zero state -> nonzero (seed) on first call");
    /* The escaped sequence is the same as starting from the seed directly:
     * one call from zero (mapped to seed) == one call from the seed. */
    uint16_t s2 = PIC_MATH_RAND_SEED;
    (void)pic_math_rand_next(&s2);
    CHECK(state == s2, "zero-state escape joins the seed's sequence");
    uint16_t v2 = pic_math_rand_next(&state);
    CHECK(v2 != 0u, "not stuck at zero after the escape");
}

static void test_rand_reentrance(void)
{
    uint16_t a = 1, b = 2;
    uint16_t sa = 0, sb = 0;
    for (int i = 0; i < 100; i++) {
        sa = pic_math_rand_next(&a);
        sb = pic_math_rand_next(&b);
    }
    CHECK(sa != sb, "two independent states diverge (reentrant)");
}

static void test_gauss_distribution(void)
{
    /* Coarse bell-shape sanity: 32 buckets across int16, central bucket
     * more populated than the extreme tail buckets. */
    enum { NB = 32, NSAMP = 200000 };
    uint32_t hist[NB];
    for (int i = 0; i < NB; i++) hist[i] = 0u;
    uint16_t state = 12345u;
    int16_t lo = 0, hi = 0;
    for (long n = 0; n < NSAMP; n++) {
        int16_t g = pic_math_rand_gauss(&state);
        if (g < lo) lo = g;
        if (g > hi) hi = g;
        unsigned b = (unsigned)(g + 32768) >> 11;   /* 0..65535 -> 0..31 (5 bits) */
        if (b >= NB) b = (g < 0) ? 0u : (unsigned)(NB - 1);
        hist[b]++;
    }
    /* Central bucket (around 0) vs extreme tail buckets. */
    unsigned center = hist[NB / 2];
    unsigned left_tail = hist[1];
    unsigned right_tail = hist[NB - 2];
    CHECK(center > left_tail, "gauss center > left tail (bell)");
    CHECK(center > right_tail, "gauss center > right tail (bell)");
    /* Samples span a meaningful range (not degenerate). */
    CHECK(hi - lo > 1000, "gauss samples span a range");
    /* All buckets get some samples (no dead zone in the middle). */
    int dead = 0;
    for (int i = 2; i < NB - 2; i++) if (hist[i] == 0u) dead++;
    CHECK(dead == 0, "no empty buckets in the central range");
}

int main(void)
{
    test_rand_period_and_zero();
    test_rand_zero_state_escape();
    test_rand_reentrance();
    test_gauss_distribution();
    printf("test_rand: %u checks failed\n", (unsigned)g_pic_math_failures);
    return pic_math_test_report();
}
