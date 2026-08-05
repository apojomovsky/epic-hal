/**
 * @file    test_div.c
 * @brief   Host tests for `pic_math_divmod_u16`/`_s16`/`_u32_16`: the
 *          oracle against native `/`/`%`, a C reference of the restoring
 *          shift-subtract algorithm the PIC16/PIC18 asm mirrors (so a
 *          correct hand-trace against this implies a correct asm body),
 *          and the documented edge contracts (divide-by-zero, `INT16_MIN
 *          / -1`, u32_16 truncation).
 */

#include "pic_math.h"
#include "pic_math_test.h"

/* ─── 2. C reference of the restoring algorithm the asm mirrors ──── */

/* 16/16: AN526 layout -- acca=num becomes the quotient, accb=0 becomes the
 * remainder; each iteration shifts accb:acca left (acca MSB -> accb LSB),
 * compares accb to den, subtracts and ORs a 1 into acca LSB if it fits. */
static pic_math_udiv16_t ref_divmod_u16_algo(uint16_t num, uint16_t den)
{
    uint16_t acca = num, accb = 0;
    for (int i = 0; i < 16; i++) {
        uint16_t c = (uint16_t)((acca >> 15) & 1u);
        acca = (uint16_t)(acca << 1);          /* LSB now 0: room for quotient bit */
        accb = (uint16_t)((accb << 1) | c);    /* dividend bit into remainder    */
        if (accb >= den) { accb = (uint16_t)(accb - den); acca |= 1u; }
    }
    pic_math_udiv16_t r = { acca, accb };
    return r;
}

/* 32/16: 32 iterations; the 32-bit quotient builds in `acc`, the remainder
 * in `rem`. The remainder is kept in a 32-bit variable because the shift
 * `rem = rem*2 + bit` can produce up to 2*den-1, which overflows 16 bits
 * when den > 0x8000 -- the 17th bit is the carry the asm version handles via
 * STATUS.C. After the subtract, rem < den <= 0xFFFF so the low 16 bits are
 * the remainder. We return the low 16 bits of the quotient (truncated per the
 * documented contract). */
static pic_math_udiv16_t ref_divmod_u32_16_algo(uint32_t num, uint16_t den)
{
    uint32_t acc = num;
    uint32_t rem = 0;
    for (int i = 0; i < 32; i++) {
        uint32_t top = (acc >> 31) & 1u;
        acc <<= 1;                              /* LSB 0: room for quotient bit */
        rem = (rem << 1) | top;                 /* 17-bit-safe: rem < 2*den     */
        if (rem >= (uint32_t)den) { rem -= den; acc |= 1u; }
    }
    pic_math_udiv16_t r = { (uint16_t)acc, (uint16_t)rem };
    return r;
}

/* ─── 1 + 2. oracle and algorithm vs native ─────────────────────── */

static const uint16_t U16_BOUNDS[] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0007, 0x0100, 0x7FFF, 0x8000,
    0xFFFE, 0xFFFF
};

static void test_divmod_u16(void)
{
    /* Exhaustive numerator x boundary denominator, plus randomized pairs. */
    for (size_t d = 0; d < sizeof(U16_BOUNDS)/sizeof(U16_BOUNDS[0]); d++) {
        uint16_t den = U16_BOUNDS[d];
        if (den == 0) continue;
        for (uint32_t n = 0; n <= 0xFFFFu; n++) {
            uint16_t num = (uint16_t)n;
            pic_math_udiv16_t got = pic_math_divmod_u16(num, den, NULL);
            pic_math_udiv16_t alg = ref_divmod_u16_algo(num, den);
            CHECK(got.quotient  == num / den, "u16 oracle quotient");
            CHECK(got.remainder == num % den, "u16 oracle remainder");
            CHECK(alg.quotient  == num / den, "u16 algo quotient");
            CHECK(alg.remainder == num % den, "u16 algo remainder");
        }
    }
    uint32_t st = 0xD1A00001u;
    for (int n = 0; n < 200000; n++) {
        uint16_t num = (uint16_t)pic_math_test_rand(&st);
        uint16_t den = (uint16_t)pic_math_test_rand(&st);
        if (den == 0) continue;
        pic_math_udiv16_t got = pic_math_divmod_u16(num, den, NULL);
        pic_math_udiv16_t alg = ref_divmod_u16_algo(num, den);
        CHECK(got.quotient  == num / den, "u16 random oracle quotient");
        CHECK(got.remainder == num % den, "u16 random oracle remainder");
        CHECK(alg.quotient  == num / den, "u16 random algo quotient");
        CHECK(alg.remainder == num % den, "u16 random algo remainder");
    }
}

static void test_divmod_u32_16(void)
{
    /* Boundary denominators x randomized 32-bit numerators (incl. overflow
     * cases where num >= den*65536 -- quotient must truncate to low 16). */
    for (size_t d = 0; d < sizeof(U16_BOUNDS)/sizeof(U16_BOUNDS[0]); d++) {
        uint16_t den = U16_BOUNDS[d];
        if (den == 0) continue;
        uint32_t st = 0xF32A0001u ^ ((uint32_t)den << 16);
        for (int n = 0; n < 20000; n++) {
            uint32_t num = pic_math_test_rand(&st);
            pic_math_udiv16_t got = pic_math_divmod_u32_16(num, den, NULL);
            pic_math_udiv16_t alg = ref_divmod_u32_16_algo(num, den);
            uint16_t exp_q = (uint16_t)(num / (uint32_t)den);
            uint16_t exp_r = (uint16_t)(num % (uint32_t)den);
            CHECK(got.quotient  == exp_q, "u32_16 oracle quotient");
            CHECK(got.remainder == exp_r, "u32_16 oracle remainder");
            CHECK(alg.quotient  == exp_q, "u32_16 algo quotient");
            CHECK(alg.remainder == exp_r, "u32_16 algo remainder");
        }
    }
}

static const int16_t S16_BOUNDS[] = {
    0, 1, -1, 2, -2, 127, -127, 128, -128, 255, -255, 256, -256,
    32767, -32768
};

static void test_divmod_s16(void)
{
    /* Boundary cross-product + randomized, vs native int32 division. */
    for (size_t i = 0; i < sizeof(S16_BOUNDS)/sizeof(S16_BOUNDS[0]); i++)
        for (size_t j = 0; j < sizeof(S16_BOUNDS)/sizeof(S16_BOUNDS[0]); j++) {
            int16_t num = S16_BOUNDS[i], den = S16_BOUNDS[j];
            if (den == 0) continue;
            pic_math_sdiv16_t got = pic_math_divmod_s16(num, den, NULL);
            int32_t exp_q = (int32_t)num / (int32_t)den;
            int32_t exp_r = (int32_t)num % (int32_t)den;
            CHECK(got.quotient  == (int16_t)exp_q, "s16 boundary quotient");
            CHECK(got.remainder == (int16_t)exp_r, "s16 boundary remainder");
        }
    uint32_t st = 0x51660001u;
    for (int n = 0; n < 200000; n++) {
        int16_t num = (int16_t)(uint16_t)pic_math_test_rand(&st);
        int16_t den = (int16_t)(uint16_t)pic_math_test_rand(&st);
        if (den == 0) continue;
        pic_math_sdiv16_t got = pic_math_divmod_s16(num, den, NULL);
        int32_t exp_q = (int32_t)num / (int32_t)den;
        int32_t exp_r = (int32_t)num % (int32_t)den;
        CHECK(got.quotient  == (int16_t)exp_q, "s16 random quotient");
        CHECK(got.remainder == (int16_t)exp_r, "s16 random remainder");
    }
}

/* ─── 3. documented edge contracts ──────────────────────────────── */

static void test_div_edges(void)
{
    bool ok = true;
    /* Divide-by-zero: *ok=false, zeroed fields (all three forms). */
    pic_math_udiv16_t u = pic_math_divmod_u16(1234u, 0u, &ok);
    CHECK(ok == false, "u16 div0 ok flag");
    CHECK(u.quotient == 0 && u.remainder == 0, "u16 div0 zeroed");

    ok = true;
    pic_math_sdiv16_t s = pic_math_divmod_s16(1234, 0, &ok);
    CHECK(ok == false, "s16 div0 ok flag");
    CHECK(s.quotient == 0 && s.remainder == 0, "s16 div0 zeroed");

    ok = true;
    pic_math_udiv16_t w = pic_math_divmod_u32_16(0xDEADBEEFu, 0u, &ok);
    CHECK(ok == false, "u32_16 div0 ok flag");
    CHECK(w.quotient == 0 && w.remainder == 0, "u32_16 div0 zeroed");

    /* NULL ok pointer: must not crash, still zeroes. */
    u = pic_math_divmod_u16(1234u, 0u, NULL);
    CHECK(u.quotient == 0 && u.remainder == 0, "u16 div0 NULL ok");

    /* INT16_MIN / -1: true quotient 32768 wraps to INT16_MIN, remainder 0
     * (documented; the one signed divide that can overflow int16). */
    ok = false;
    s = pic_math_divmod_s16(INT16_MIN, -1, &ok);
    CHECK(ok == true, "INT16_MIN/-1 ok flag");
    CHECK(s.quotient == INT16_MIN, "INT16_MIN/-1 quotient wraps");
    CHECK(s.remainder == 0, "INT16_MIN/-1 remainder");

    /* u32_16 truncation: 0x00020000 / 1 = 0x20000 -> low 16 = 0. */
    w = pic_math_divmod_u32_16(0x00020000u, 1u, &ok);
    CHECK(ok == true, "u32_16 truncate ok");
    CHECK(w.quotient == 0, "u32_16 truncates high quotient bits");
}

int main(void)
{
    test_divmod_u16();
    test_divmod_u32_16();
    test_divmod_s16();
    test_div_edges();
    printf("test_div: %u checks failed\n", (unsigned)g_pic_math_failures);
    return pic_math_test_report();
}
