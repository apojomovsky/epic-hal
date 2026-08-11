/*
 * Host tests for pic_math_add_u16/sub_u16/negate_s16/negate_s32:
 * randomized plus boundary cases (including INT16_MIN/INT32_MIN, which
 * negate to themselves) against a 32-bit reference.
 */

#include "pic_math.h"
#include "pic_math_test.h"

static const uint16_t U16_BOUNDS[] = {
    0x0000, 0x0001, 0x0002, 0x7FFF, 0x8000, 0xFFFE, 0xFFFF
};

/** @brief Boundary cross-product plus randomized pairs for pic_math_add_u16. */
static void test_add_u16(void)
{
    for (size_t i = 0; i < sizeof(U16_BOUNDS)/sizeof(U16_BOUNDS[0]); i++)
        for (size_t j = 0; j < sizeof(U16_BOUNDS)/sizeof(U16_BOUNDS[0]); j++) {
            uint16_t a = U16_BOUNDS[i], b = U16_BOUNDS[j];
            bool co = false;
            uint16_t got = pic_math_add_u16(a, b, &co);
            uint32_t s = (uint32_t)a + (uint32_t)b;
            CHECK(got == (uint16_t)s, "add_u16 result mismatch");
            CHECK(co == (s > 0xFFFFu), "add_u16 carry mismatch");
        }
    uint32_t st = 0xADD10011u;
    for (int n = 0; n < 200000; n++) {
        uint16_t a = (uint16_t)pic_math_test_rand(&st);
        uint16_t b = (uint16_t)pic_math_test_rand(&st);
        bool co = false;
        uint16_t got = pic_math_add_u16(a, b, &co);
        uint32_t s = (uint32_t)a + (uint32_t)b;
        CHECK(got == (uint16_t)s, "add_u16 random result mismatch");
        CHECK(co == (s > 0xFFFFu), "add_u16 random carry mismatch");
    }
    /* NULL carry_out pointer must not crash. */
    (void)pic_math_add_u16(0xFFFFu, 0x0001u, NULL);
}

/** @brief Boundary cross-product plus randomized pairs for pic_math_sub_u16. */
static void test_sub_u16(void)
{
    for (size_t i = 0; i < sizeof(U16_BOUNDS)/sizeof(U16_BOUNDS[0]); i++)
        for (size_t j = 0; j < sizeof(U16_BOUNDS)/sizeof(U16_BOUNDS[0]); j++) {
            uint16_t a = U16_BOUNDS[i], b = U16_BOUNDS[j];
            bool bo = false;
            uint16_t got = pic_math_sub_u16(a, b, &bo);
            CHECK(got == (uint16_t)(a - b), "sub_u16 result mismatch");
            CHECK(bo == (a < b), "sub_u16 borrow mismatch");
        }
    uint32_t st = 0x5B100011u;
    for (int n = 0; n < 200000; n++) {
        uint16_t a = (uint16_t)pic_math_test_rand(&st);
        uint16_t b = (uint16_t)pic_math_test_rand(&st);
        bool bo = false;
        uint16_t got = pic_math_sub_u16(a, b, &bo);
        CHECK(got == (uint16_t)(a - b), "sub_u16 random result mismatch");
        CHECK(bo == (a < b), "sub_u16 random borrow mismatch");
    }
    (void)pic_math_sub_u16(0x0000u, 0x0001u, NULL);
}

/** @brief Boundary values incl. INT16_MIN/INT32_MIN for pic_math_negate_s16/_s32. */
static void test_negate(void)
{
    const int16_t s16[] = { 0, 1, -1, 2, -2, 127, -128, 128, -129,
                            32767, -32768, 12345, -12345 };
    for (size_t i = 0; i < sizeof(s16)/sizeof(s16[0]); i++) {
        int16_t v = s16[i];
        int16_t got = pic_math_negate_s16(v);
        int16_t exp = (int16_t)(0u - (uint16_t)v);
        CHECK(got == exp, "negate_s16 mismatch");
        /* INT16_MIN negates to itself (two's complement wrap). */
        if (v == INT16_MIN) CHECK(got == INT16_MIN, "negate_s16 INT16_MIN");
    }
    const int32_t s32[] = { 0, 1, -1, 32767, -32768, 2147483647L,
                            -2147483647L - 1L /* INT32_MIN */, 123456789L,
                            -123456789L };
    for (size_t i = 0; i < sizeof(s32)/sizeof(s32[0]); i++) {
        int32_t v = s32[i];
        int32_t got = pic_math_negate_s32(v);
        int32_t exp = (int32_t)(0u - (uint32_t)v);
        CHECK(got == exp, "negate_s32 mismatch");
        if (v == INT32_MIN) CHECK(got == INT32_MIN, "negate_s32 INT32_MIN");
    }
}

/** @brief Run all add/sub/negate tests and report the failure count. */
int main(void)
{
    test_add_u16();
    test_sub_u16();
    test_negate();
    printf("test_addsub: %u checks failed\n", (unsigned)g_pic_math_failures);
    return pic_math_test_report();
}
