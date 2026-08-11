/*
 * Host reference backend: portable-C BCD primitives, the independent
 * oracle for the PIC16/PIC18 asm backends. BCD is packed (one nibble per
 * digit: decimal 42 is the byte 0x42); an invalid nibble (> 9) is
 * processed arithmetically, e.g. bcd8_to_bin(0x0A) == 10, the documented
 * behavior for out-of-range input.
 */

#include "pic_math.h"

/**
 * @brief  2-digit packed BCD -> binary (host oracle).
 * @param  bcd2  2-digit packed BCD (one nibble per digit), 0..0x99
 * @return binary value of the BCD input, 0..99.
 */
uint8_t pic_math_bcd8_to_bin(uint8_t bcd2)
{
    return (uint8_t)(((bcd2 >> 4) * 10u) + (bcd2 & 0x0Fu));
}

/**
 * @brief  binary 0..99 -> 2-digit packed BCD (host oracle).
 * @param  value  binary value to convert, 0..99
 * @return 2-digit packed BCD representation of @p value.
 */
uint8_t pic_math_bin_to_bcd8(uint8_t value)
{
    return (uint8_t)(((value / 10u) << 4) | (value % 10u));
}

/**
 * @brief  5-digit packed BCD -> binary (host oracle); BCD above 65535
 *         truncates to the low 16 bits, the documented behavior.
 * @param  bcd5  5-digit packed BCD (one nibble per digit), 0..0x99999
 * @return binary value of the BCD input, truncated to 16 bits.
 */
uint16_t pic_math_bcd16_to_bin(uint32_t bcd5)
{
    /* Accumulate in uint32_t so a 5-digit BCD (up to 99999) is computed
     * exactly, then truncate to the uint16_t binary width: BCD representing
     * 0..65535 returns exactly; 65536..99999 wrap to the low 16 bits
     * (documented -- the binary side is 16-bit). */
    uint32_t bin = 0u;
    uint32_t mult = 1u;
    for (int i = 0; i < 5; i++) {
        bin += (bcd5 & 0x0Fu) * mult;
        bcd5 >>= 4;
        mult *= 10u;
    }
    return (uint16_t)bin;
}

/**
 * @brief  binary 0..65535 -> 5-digit packed BCD (host oracle).
 * @param  value  binary value to convert, 0..65535
 * @return 5-digit packed BCD representation of @p value, 0..0x65535.
 */
uint32_t pic_math_bin_to_bcd16(uint16_t value)
{
    uint32_t bcd = 0u;
    for (int i = 0; i < 5; i++) {
        bcd |= (uint32_t)(value % 10u) << (i * 4);
        value = (uint16_t)(value / 10u);
    }
    return bcd;
}

/**
 * @brief  Packed-BCD 2-digit add with carry out (host oracle).
 * @param  a           BCD augend, 0..0x99 (valid BCD)
 * @param  b           BCD addend, 0..0x99 (valid BCD)
 * @param  carry_out  set true if the BCD sum exceeds 99; may be NULL.
 * @return packed-BCD 2-digit sum.
 */
uint8_t pic_math_bcd_add8(uint8_t a, uint8_t b, bool *carry_out)
{
    /* Unpack to decimal, add (0..99 + 0..99 = 0..198), repack the low 2
     * digits; carry out if the sum exceeded 99. Invalid nibbles are carried
     * through arithmetically. */
    uint16_t da = (uint16_t)((a >> 4) * 10u) + (a & 0x0Fu);
    uint16_t db = (uint16_t)((b >> 4) * 10u) + (b & 0x0Fu);
    uint16_t sum = da + db;                  /* 0..198 (or more w/ bad nibbles) */
    uint16_t lo = sum % 100u;
    if (carry_out) *carry_out = (sum >= 100u);
    uint8_t tens = (uint8_t)(lo / 10u);
    uint8_t ones = (uint8_t)(lo % 10u);
    return (uint8_t)((tens << 4) | ones);
}

/**
 * @brief  Packed-BCD 2-digit subtract with borrow out (host oracle).
 * @param  a           BCD minuend, 0..0x99 (valid BCD)
 * @param  b           BCD subtrahend, 0..0x99 (valid BCD)
 * @param  borrow_out  set true on BCD underflow (a < b in BCD); may be NULL.
 * @return packed-BCD 2-digit difference (modulo 100 on underflow).
 */
uint8_t pic_math_bcd_sub8(uint8_t a, uint8_t b, bool *borrow_out)
{
    /* Unpack, subtract; borrow out if a < b (in decimal). The result wraps
     * modulo 100 on underflow (mirroring the binary sub_u16 wrap). */
    int32_t da = (int32_t)((a >> 4) * 10u) + (a & 0x0Fu);
    int32_t db = (int32_t)((b >> 4) * 10u) + (b & 0x0Fu);
    int32_t diff = da - db;
    if (borrow_out) *borrow_out = (diff < 0);
    if (diff < 0) diff += 100;
    uint8_t tens = (uint8_t)(diff / 10);
    uint8_t ones = (uint8_t)(diff % 10);
    return (uint8_t)((tens << 4) | ones);
}
