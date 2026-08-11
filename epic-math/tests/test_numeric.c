/*
 * Host tests for pic_math_diff3 and pic_math_integrate_simpson38
 * against analytic functions: both are exact up to their respective
 * polynomial degree (diff3: <=2, Simpson's 3/8: <=3), checked for an
 * exact Q-format result there and within a documented error bound for
 * higher-degree inputs.
 */

#include "pic_math.h"
#include "pic_math_test.h"
#include <math.h>

/* h = 1 throughout, so 1/(2h) = 0.5 -> Q8.8 = 0x0080 = 128. */
#define INV_2H_Q8   ((int16_t)128)
/* 3h/8 = 3/8 = 0.375 -> Q16.16 = 0x00006000 = 24576. */
#define THREE_H_OVER_8_Q16 ((int32_t)0x00006000L)

static void test_diff3_linear(void)
{
    /* f(x) = m*x, derivative = m (constant). Central diff is exact. */
    const int m[] = { 0, 1, 2, 3, 5, 7, 10, 17, 31, 64, 100, 127 };
    for (size_t i = 0; i < sizeof(m)/sizeof(m[0]); i++) {
        int16_t x0 = (int16_t)(0 * m[i]);
        int16_t x2 = (int16_t)(2 * m[i]);
        int16_t got = pic_math_diff3(x0, (int16_t)m[i], x2, INV_2H_Q8);
        int16_t exp = (int16_t)(m[i] * 256);   /* m in Q8.8 */
        CHECK(got == exp, "diff3 linear");
    }
}

static void test_diff3_quadratic(void)
{
    /* f(x) = x^2, derivative = 2x. Central diff (f(x+h)-f(x-h))/2h = 2x exact. */
    for (int x = 0; x <= 60; x++) {
        int16_t xm1 = (int16_t)((x - 1) * (x - 1));
        int16_t xp1 = (int16_t)((x + 1) * (x + 1));
        int16_t got = pic_math_diff3(xm1, (int16_t)(x * x), xp1, INV_2H_Q8);
        int16_t exp = (int16_t)(2 * x * 256);   /* 2x in Q8.8 */
        CHECK(got == exp, "diff3 quadratic");
    }
}

static void test_diff3_cubic_bound(void)
{
    /* f(x) = x^3. Central diff = (f(x+h)-f(x-h))/2h = 3x^2 + h^2 (the O(h^2)
     * error), so for h=1 the Q8.8 result is (3x^2+1)*256, within 1.0 (real)
     * = 256 Q8.8 of the true derivative 3x^2*256. */
    for (int x = 0; x <= 20; x++) {
        int16_t xm1 = (int16_t)((x - 1) * (x - 1) * (x - 1));
        int16_t xp1 = (int16_t)((x + 1) * (x + 1) * (x + 1));
        int16_t got = pic_math_diff3(xm1, (int16_t)(x * x * x), xp1, INV_2H_Q8);
        int16_t true_q8 = (int16_t)(3 * x * x * 256);
        int err = got - true_q8;
        if (err < 0) err = -err;
        CHECK(err <= 256, "diff3 cubic O(h^2) bound");
    }
}

static void test_diff3_noninteger_slope(void)
{
    /* Fractional-slope sanity: (4-1)*128 = 384 Q8.8, confirming the raw
     * integer product (the caller scales to real units). */
    int16_t got = pic_math_diff3(1, 2, 4, INV_2H_Q8);
    CHECK(got == 384, "diff3 formula arithmetic");
}

static void test_simpson38_exact(void)
{
    /* f(x)=c (constant), [0,3], h=1: integral = 3c. */
    for (int c = 0; c <= 20; c++) {
        int32_t got = pic_math_integrate_simpson38((int16_t)c, (int16_t)c,
                                                   (int16_t)c, (int16_t)c,
                                                   THREE_H_OVER_8_Q16);
        int32_t exp = (int32_t)(3.0 * c * 65536.0);   /* 3c in Q16.16 */
        CHECK(got == exp, "simpson38 constant");
    }
    /* f(x)=x, [0,3]: integral = 4.5. f0=0,f1=1,f2=2,f3=3. */
    {
        int32_t got = pic_math_integrate_simpson38(0, 1, 2, 3, THREE_H_OVER_8_Q16);
        int32_t exp = (int32_t)(4.5 * 65536.0);
        CHECK(got == exp, "simpson38 linear");
    }
    /* f(x)=x^3, [0,3]: integral = 20.25. f0=0,f1=1,f2=8,f3=27. */
    {
        int32_t got = pic_math_integrate_simpson38(0, 1, 8, 27, THREE_H_OVER_8_Q16);
        int32_t exp = (int32_t)(20.25 * 65536.0);
        CHECK(got == exp, "simpson38 cubic");
    }
}

static void test_simpson38_quartic_bound(void)
{
    /* f(x)=x^4, [0,3], h=1: true integral = 243/5 = 48.6. Simpson 3/8 is
     * degree-3 exact, so a degree-4 integrand has O(h^4) error; the Q16.16
     * error is within ~1.0 (65536) for this panel. */
    int32_t got = pic_math_integrate_simpson38(0, 1, 16, 81, THREE_H_OVER_8_Q16);
    double true_q16 = (243.0 / 5.0) * 65536.0;
    double err = (double)got - true_q16;
    if (err < 0) err = -err;
    CHECK(err <= 65536.0, "simpson38 quartic O(h^4) bound");
}

int main(void)
{
    test_diff3_linear();
    test_diff3_quadratic();
    test_diff3_cubic_bound();
    test_diff3_noninteger_slope();
    test_simpson38_exact();
    test_simpson38_quartic_bound();
    printf("test_numeric: %u checks failed\n", (unsigned)g_pic_math_failures);
    return pic_math_test_report();
}
