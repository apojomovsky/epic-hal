/*
 * Host reference backend: portable-C add/sub/negate primitives, the
 * independent oracle for the PIC16/PIC18 asm backends. Negation works in
 * unsigned to avoid -INT_MIN overflow, so negate(INT16_MIN) == INT16_MIN
 * (two's-complement wrap, documented).
 */

#include "pic_math.h"

uint16_t pic_math_add_u16(uint16_t a, uint16_t b, bool *carry_out)
{
    uint32_t s = (uint32_t)a + (uint32_t)b;
    if (carry_out) *carry_out = (s > 0xFFFFu);
    return (uint16_t)s;
}

uint16_t pic_math_sub_u16(uint16_t a, uint16_t b, bool *borrow_out)
{
    if (borrow_out) *borrow_out = (a < b);
    return (uint16_t)(a - b);
}

int16_t pic_math_negate_s16(int16_t v)
{
    return (int16_t)(0u - (uint16_t)v);
}

int32_t pic_math_negate_s32(int32_t v)
{
    return (int32_t)(0u - (uint32_t)v);
}
