/*
 * Host reference backend: portable-C add/sub/negate primitives, the
 * independent oracle for the PIC16/PIC18 asm backends. Negation works in
 * unsigned to avoid -INT_MIN overflow, so negate(INT16_MIN) == INT16_MIN
 * (two's-complement wrap, documented).
 */

#include "pic_math.h"

/**
 * @brief  16-bit unsigned add with carry out (host oracle).
 * @param  a           augend, 0..65535
 * @param  b           addend, 0..65535
 * @param  carry_out  set true on overflow (sum > 65535); may be NULL.
 * @return (a + b) truncated to 16 bits.
 */
uint16_t pic_math_add_u16(uint16_t a, uint16_t b, bool *carry_out)
{
    uint32_t s = (uint32_t)a + (uint32_t)b;
    if (carry_out) *carry_out = (s > 0xFFFFu);
    return (uint16_t)s;
}

/**
 * @brief  16-bit unsigned subtract with borrow out (host oracle).
 * @param  a           minuend, 0..65535
 * @param  b           subtrahend, 0..65535
 * @param  borrow_out  set true on underflow (a < b); may be NULL.
 * @return (a - b) truncated to 16 bits.
 */
uint16_t pic_math_sub_u16(uint16_t a, uint16_t b, bool *borrow_out)
{
    if (borrow_out) *borrow_out = (a < b);
    return (uint16_t)(a - b);
}

/**
 * @brief  16-bit two's-complement negate (host oracle).
 * @param  v  value to negate, -32768..32767
 * @return -v; INT16_MIN negates to itself (two's-complement wrap).
 */
int16_t pic_math_negate_s16(int16_t v)
{
    return (int16_t)(0u - (uint16_t)v);
}

/**
 * @brief  32-bit two's-complement negate (host oracle).
 * @param  v  value to negate, -2147483648..2147483647
 * @return -v; INT32_MIN negates to itself (two's-complement wrap).
 */
int32_t pic_math_negate_s32(int32_t v)
{
    return (int32_t)(0u - (uint32_t)v);
}
