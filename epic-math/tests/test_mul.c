/*
 * Host tests for pic_math_mul_u8 (exhaustive over all 256x256 pairs)
 * and _u16/_s16 (randomized plus boundary values) against native
 * wider-integer arithmetic.
 */

#include "pic_math.h"
#include "pic_math_test.h"

static void test_mul_u8_exhaustive(void)
{
    for (uint32_t a = 0; a <= 0xFFu; a++) {
        for (uint32_t b = 0; b <= 0xFFu; b++) {
            uint16_t got = pic_math_mul_u8((uint8_t)a, (uint8_t)b);
            uint16_t exp = (uint16_t)(a * b);
            if (got != exp) {
                CHECK(0, "mul_u8 mismatch");
                if (g_pic_math_failures < 5)
                    printf("  a=%lu b=%lu got=%u exp=%u\n",
                           (unsigned long)a, (unsigned long)b, got, exp);
            }
        }
    }
}

static const uint16_t U16_BOUNDS[] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0007, 0x0008,
    0x00FF, 0x0100, 0x0101, 0x7FFF, 0x8000, 0xFFFE, 0xFFFF
};
static const int16_t S16_BOUNDS[] = {
    0, 1, 2, 3, 7, 8, 127, 128, 255, 256,
    -1, -2, -128, -129, -255, -256, 32767, -32768
};

static void test_mul_u16(void)
{
    /* Exhaustive over the boundary cross-product (169 pairs). */
    for (size_t i = 0; i < sizeof(U16_BOUNDS)/sizeof(U16_BOUNDS[0]); i++)
        for (size_t j = 0; j < sizeof(U16_BOUNDS)/sizeof(U16_BOUNDS[0]); j++) {
            uint16_t a = U16_BOUNDS[i], b = U16_BOUNDS[j];
            uint32_t got = pic_math_mul_u16(a, b);
            uint32_t exp = (uint32_t)a * (uint32_t)b;
            CHECK(got == exp, "mul_u16 boundary mismatch");
        }

    /* Randomized fuzz, reproducible. */
    uint32_t st = 0xC0FFEE01u;
    for (int n = 0; n < 200000; n++) {
        uint16_t a = (uint16_t)pic_math_test_rand(&st);
        uint16_t b = (uint16_t)pic_math_test_rand(&st);
        uint32_t got = pic_math_mul_u16(a, b);
        uint32_t exp = (uint32_t)a * (uint32_t)b;
        CHECK(got == exp, "mul_u16 random mismatch");
    }
}

static void test_mul_s16(void)
{
    /* Boundary cross-product (324 pairs), incl. INT16_MIN*INT16_MIN and
     * INT16_MIN*-1 (the cases that stress signed abs/negate). */
    for (size_t i = 0; i < sizeof(S16_BOUNDS)/sizeof(S16_BOUNDS[0]); i++)
        for (size_t j = 0; j < sizeof(S16_BOUNDS)/sizeof(S16_BOUNDS[0]); j++) {
            int16_t a = S16_BOUNDS[i], b = S16_BOUNDS[j];
            int32_t got = pic_math_mul_s16(a, b);
            int32_t exp = (int32_t)a * (int32_t)b;
            CHECK(got == exp, "mul_s16 boundary mismatch");
        }

    uint32_t st = 0x5EA12345u;
    for (int n = 0; n < 200000; n++) {
        int16_t a = (int16_t)(uint16_t)pic_math_test_rand(&st);
        int16_t b = (int16_t)(uint16_t)pic_math_test_rand(&st);
        int32_t got = pic_math_mul_s16(a, b);
        int32_t exp = (int32_t)a * (int32_t)b;
        CHECK(got == exp, "mul_s16 random mismatch");
    }
}

int main(void)
{
    test_mul_u8_exhaustive();
    test_mul_u16();
    test_mul_s16();
    printf("test_mul: %u checks failed\n", (unsigned)g_pic_math_failures);
    return pic_math_test_report();
}
