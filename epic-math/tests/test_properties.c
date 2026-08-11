/*
 * Host property tests for the fixed-point primitives: algebraic
 * identities that must hold for every input, checked over deterministic
 * random streams. Complements the oracle tests (test_mul/test_div/
 * test_addsub/test_bcd compare against wider native arithmetic); these
 * check the identities the module's own docs promise: add and sub are
 * inverses modulo 2^16, mul is commutative, divmod round-trips through
 * n = q*d + r, and the BCD add/sub pair are inverses modulo 100.
 */

#include "pic_math.h"
#include "pic_math_test.h"

static const uint16_t U16_BOUNDS[] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0007, 0x0008,
    0x00FF, 0x0100, 0x0101, 0x7FFF, 0x8000, 0xFFFE, 0xFFFF
};

/* Reference decimal helper (independent of the implementation). */
static uint8_t ref_bcd8(uint8_t v) { return (uint8_t)(((v/10u)<<4)|(v%10u)); }

/**
 * @brief  (a + b) - b == a for every a, b, including pairs whose sum
 *         overflows (carry out). The compensating subtract must borrow
 *         exactly when the add carried, so co == bo is part of the
 *         identity: a symmetric carry/borrow bug in either direction
 *         fails this check.
 */
static void test_addsub_roundtrip(void)
{
    /* Boundary cross-product, then random. */
    for (size_t i = 0; i < sizeof(U16_BOUNDS)/sizeof(U16_BOUNDS[0]); i++)
        for (size_t j = 0; j < sizeof(U16_BOUNDS)/sizeof(U16_BOUNDS[0]); j++) {
            uint16_t a = U16_BOUNDS[i], b = U16_BOUNDS[j];
            bool co = false, bo = false;
            uint16_t s = pic_math_add_u16(a, b, &co);
            uint16_t r = pic_math_sub_u16(s, b, &bo);
            CHECK(r == a, "addsub roundtrip boundary");
            CHECK(co == bo, "addsub carry == borrow boundary");
        }
    uint32_t st = 0xAC1D0001u;
    for (int n = 0; n < 200000; n++) {
        uint16_t a = (uint16_t)pic_math_test_rand(&st);
        uint16_t b = (uint16_t)pic_math_test_rand(&st);
        bool co = false, bo = false;
        uint16_t s = pic_math_add_u16(a, b, &co);
        uint16_t r = pic_math_sub_u16(s, b, &bo);
        CHECK(r == a, "addsub roundtrip random");
        CHECK(co == bo, "addsub carry == borrow random");
    }
}

/**
 * @brief  a + b == b + a including the carry flag, and a * b == b * a
 *         for both signed and unsigned 16-bit multiply.
 */
static void test_commutativity(void)
{
    uint32_t st = 0x0C0FFEE1u;
    for (int n = 0; n < 200000; n++) {
        uint16_t a = (uint16_t)pic_math_test_rand(&st);
        uint16_t b = (uint16_t)pic_math_test_rand(&st);
        bool ca = false, cb = false;
        uint16_t sa = pic_math_add_u16(a, b, &ca);
        uint16_t sb = pic_math_add_u16(b, a, &cb);
        CHECK(sa == sb, "add_u16 commutative result");
        CHECK(ca == cb, "add_u16 commutative carry");
        CHECK(pic_math_mul_u16(a, b) == pic_math_mul_u16(b, a),
              "mul_u16 commutative");
        CHECK(pic_math_mul_s16((int16_t)(uint16_t)a, (int16_t)(uint16_t)b) ==
              pic_math_mul_s16((int16_t)(uint16_t)b, (int16_t)(uint16_t)a),
              "mul_s16 commutative");
    }
}

/**
 * @brief  n == q*d + r with r < d for every divisor; the same identity
 *         holds for the signed variant with C truncation semantics
 *         (|r| < |d|). Checks ok == true for d != 0.
 */
static void test_divmod_roundtrip(void)
{
    /* Unsigned 16/16. */
    uint32_t st = 0xD1F00001u;
    for (int n = 0; n < 200000; n++) {
        uint16_t num = (uint16_t)pic_math_test_rand(&st);
        uint16_t den = (uint16_t)pic_math_test_rand(&st);
        if (den == 0u) continue;
        bool ok = false;
        pic_math_udiv16_t r = pic_math_divmod_u16(num, den, &ok);
        CHECK(ok == true, "u16 divmod ok");
        CHECK((uint32_t)r.quotient * (uint32_t)den + r.remainder == num,
              "u16 divmod n == q*d + r");
        CHECK(r.remainder < den, "u16 divmod r < d");
    }

    /* Wide 32/16: the returned quotient is the low 16 bits of the true
     * quotient (documented contract), so the identity carries the
     * truncation term: num == q16*d + r + (q_full >> 16)*d*65536. When
     * the true quotient fits in 16 bits this reduces to the plain
     * n == q*d + r round trip. */
    st = 0x32F10001u;
    for (int n = 0; n < 100000; n++) {
        uint32_t num = pic_math_test_rand(&st) | ((uint32_t)pic_math_test_rand(&st) << 16);
        uint16_t den = (uint16_t)pic_math_test_rand(&st);
        if (den == 0u) continue;
        bool ok = false;
        pic_math_udiv16_t r = pic_math_divmod_u32_16(num, den, &ok);
        uint32_t q_full = num / (uint32_t)den;
        CHECK(ok == true, "u32_16 divmod ok");
        CHECK(r.quotient == (uint16_t)q_full, "u32_16 quotient truncation");
        CHECK(r.remainder == (uint16_t)(num % (uint32_t)den), "u32_16 remainder");
        uint64_t lhs = (uint64_t)r.quotient * (uint64_t)den + r.remainder +
                       (uint64_t)(q_full >> 16) * (uint64_t)den * 65536u;
        CHECK(lhs == num, "u32_16 divmod n == q*d + r (with truncation)");
        CHECK(r.remainder < den, "u32_16 divmod r < d");
    }

    /* Signed 16/16. */
    st = 0x5E100001u;
    for (int n = 0; n < 200000; n++) {
        int16_t num = (int16_t)(uint16_t)pic_math_test_rand(&st);
        int16_t den = (int16_t)(uint16_t)pic_math_test_rand(&st);
        if (den == 0) continue;
        bool ok = false;
        pic_math_sdiv16_t r = pic_math_divmod_s16(num, den, &ok);
        CHECK(ok == true, "s16 divmod ok");
        CHECK((int32_t)r.quotient * (int32_t)den + r.remainder == num,
              "s16 divmod n == q*d + r");
        int32_t ar = (r.remainder < 0) ? -r.remainder : r.remainder;
        int32_t ad = (den < 0) ? -den : den;
        CHECK(ar < ad, "s16 divmod |r| < |d|");
    }
}

/**
 * @brief  BCD add and sub are inverses modulo 100: subtracting the
 *         addend from the sum always recovers the augend, and adding
 *         the subtrahend back to the difference always recovers the
 *         minuend, regardless of carry/borrow. Exhaustive over the
 *         full 100x100 valid-BCD space (the invalid-nibble behavior is
 *         documented separately in test_bcd.c).
 */
static void test_bcd_inverse(void)
{
    for (uint32_t a = 0; a <= 99u; a++) {
        for (uint32_t b = 0; b <= 99u; b++) {
            bool co = false, bo = false;
            uint8_t s = pic_math_bcd_add8(ref_bcd8((uint8_t)a), ref_bcd8((uint8_t)b), &co);
            uint8_t r = pic_math_bcd_sub8(s, ref_bcd8((uint8_t)b), &bo);
            CHECK(r == ref_bcd8((uint8_t)a), "bcd sub(add(a,b),b) == a");
            CHECK(co == bo, "bcd add carry == sub borrow");

            bool bo2 = false, co2 = false;
            uint8_t d = pic_math_bcd_sub8(ref_bcd8((uint8_t)a), ref_bcd8((uint8_t)b), &bo2);
            uint8_t s2 = pic_math_bcd_add8(d, ref_bcd8((uint8_t)b), &co2);
            CHECK(s2 == ref_bcd8((uint8_t)a), "bcd add(sub(a,b),b) == a");
            CHECK(bo2 == co2, "bcd sub borrow == add carry");
        }
    }
}

int main(void)
{
    test_addsub_roundtrip();
    test_commutativity();
    test_divmod_roundtrip();
    test_bcd_inverse();
    printf("test_properties: %u checks failed\n", (unsigned)g_pic_math_failures);
    return pic_math_test_report();
}
