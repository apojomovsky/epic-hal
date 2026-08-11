/*
 * Host tests for the BCD primitives against the host reference backend:
 * exhaustive over the valid BCD range for each width, plus the
 * documented invalid-nibble behavior. The per-family asm backends are
 * validated separately (hand-traces).
 */

#include "pic_math.h"
#include "pic_math_test.h"

/* Reference helpers (decimal), independent of the implementation under test. */
static uint8_t ref_bcd8(uint8_t v) { return (uint8_t)(((v/10u)<<4)|(v%10u)); }
static uint8_t ref_bin8(uint8_t b) { return (uint8_t)((b>>4)*10u + (b&0x0Fu)); }

static void test_bcd8_roundtrip(void)
{
    for (uint32_t v = 0; v <= 99u; v++) {
        uint8_t bcd = pic_math_bin_to_bcd8((uint8_t)v);
        CHECK(bcd == ref_bcd8((uint8_t)v), "bin_to_bcd8 value");
        CHECK(pic_math_bcd8_to_bin(bcd) == v, "bcd8_to_bin roundtrip");
        CHECK(pic_math_bcd8_to_bin(ref_bcd8((uint8_t)v)) == v, "bcd8_to_bin value");
    }
}

static void test_bcd8_invalid_nibble(void)
{
    /* Documented: each nibble processed arithmetically. 0x0A -> 0*10+10 = 10. */
    CHECK(pic_math_bcd8_to_bin(0x0A) == 10u, "bcd8_to_bin invalid low nibble");
    CHECK(pic_math_bcd8_to_bin(0xA0) == 100u, "bcd8_to_bin invalid high nibble");
    CHECK(pic_math_bcd8_to_bin(0xAA) == 110u, "bcd8_to_bin both nibbles invalid");
}

static void test_bcd16_roundtrip(void)
{
    /* Exhaustive over the whole uint16_t input range 0..65535 (the binary
     * side is 16-bit; bin_to_bcd16's "16" names that width). The 5-digit BCD
     * can represent up to 99999, but a uint16_t only reaches 65535. */
    for (uint32_t v = 0; v <= 65535u; v++) {
        uint32_t bcd = pic_math_bin_to_bcd16((uint16_t)v);
        uint16_t bin = pic_math_bcd16_to_bin(bcd);
        if (bin != (uint16_t)v) {
            CHECK(0, "bcd16 roundtrip mismatch");
            if (g_pic_math_failures < 5)
                printf("  v=%lu bcd=0x%05lX bin=%u\n",
                       (unsigned long)v, (unsigned long)bcd, bin);
        }
    }
}

static void test_bcd16_invalid_nibble(void)
{
    /* 0x0000A -> 10; 0xABCDE has every nibble >9 -> 10+11*10+12*100+13*1000+14*10000. */
    CHECK(pic_math_bcd16_to_bin(0x0000Au) == 10u, "bcd16 invalid nibble");
    uint32_t bcd = 0xABCDEu;
    uint32_t exp = 0xEu + 0xDu*10u + 0xCu*100u + 0xBu*1000u + 0xAu*10000u;
    CHECK(pic_math_bcd16_to_bin(bcd) == (uint16_t)exp, "bcd16 all-invalid nibbles");
    /* bin_to_bcd16 of a valid value always yields valid (<=9) nibbles.
     * 65535 (the uint16_t max) -> 0x65535. bcd16_to_bin of 0x99999 (99999,
     * beyond uint16_t) truncates to 99999 mod 65536 = 34463. */
    CHECK(pic_math_bin_to_bcd16(65535u) == 0x65535u, "bin_to_bcd16 max");
    CHECK(pic_math_bin_to_bcd16(0u) == 0u, "bin_to_bcd16 zero");
    CHECK(pic_math_bcd16_to_bin(0x99999u) == (uint16_t)99999u, "bcd16 oversize truncates");
}

static void test_bcd_add8(void)
{
    for (uint32_t a = 0; a <= 99u; a++)
        for (uint32_t b = 0; b <= 99u; b++) {
            bool co = true;
            uint8_t got = pic_math_bcd_add8(ref_bcd8((uint8_t)a),
                                            ref_bcd8((uint8_t)b), &co);
            uint32_t sum = a + b;
            uint8_t exp = ref_bcd8((uint8_t)(sum % 100u));
            bool exp_co = (sum >= 100u);
            CHECK(got == exp, "bcd_add8 result");
            CHECK(co == exp_co, "bcd_add8 carry");
        }
    /* NULL carry pointer must not crash. */
    (void)pic_math_bcd_add8(0x99u, 0x01u, NULL);
}

static void test_bcd_sub8(void)
{
    for (uint32_t a = 0; a <= 99u; a++)
        for (uint32_t b = 0; b <= 99u; b++) {
            bool bo = true;
            uint8_t got = pic_math_bcd_sub8(ref_bcd8((uint8_t)a),
                                            ref_bcd8((uint8_t)b), &bo);
            int32_t diff = (int32_t)a - (int32_t)b;
            uint32_t lo = (uint32_t)((diff < 0) ? diff + 100 : diff);
            uint8_t exp = ref_bcd8((uint8_t)(lo % 100u));
            bool exp_bo = (diff < 0);
            CHECK(got == exp, "bcd_sub8 result");
            CHECK(bo == exp_bo, "bcd_sub8 borrow");
        }
    (void)pic_math_bcd_sub8(0x00u, 0x01u, NULL);
}

int main(void)
{
    test_bcd8_roundtrip();
    test_bcd8_invalid_nibble();
    test_bcd16_roundtrip();
    test_bcd16_invalid_nibble();
    test_bcd_add8();
    test_bcd_sub8();
    printf("test_bcd: %u checks failed\n", (unsigned)g_pic_math_failures);
    return pic_math_test_report();
}
