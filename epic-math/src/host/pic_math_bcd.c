/**
 * @file    pic_math_bcd.c (host reference backend)
 * @brief   Portable-C BCD primitives, the independent oracle for the
 *          PIC16/PIC18 asm backends. BCD is packed (one nibble per
 *          digit: decimal 42 is the byte 0x42); an invalid nibble (> 9)
 *          is processed arithmetically, e.g. `bcd8_to_bin(0x0A) == 10`,
 *          the documented behavior for out-of-range input.
 */

#include "pic_math.h"

uint8_t pic_math_bcd8_to_bin(uint8_t bcd2)
{
    return (uint8_t)(((bcd2 >> 4) * 10u) + (bcd2 & 0x0Fu));
}

uint8_t pic_math_bin_to_bcd8(uint8_t value)
{
    return (uint8_t)(((value / 10u) << 4) | (value % 10u));
}

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

uint32_t pic_math_bin_to_bcd16(uint16_t value)
{
    uint32_t bcd = 0u;
    for (int i = 0; i < 5; i++) {
        bcd |= (uint32_t)(value % 10u) << (i * 4);
        value = (uint16_t)(value / 10u);
    }
    return bcd;
}

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
