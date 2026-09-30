/*
 * Host reference backend: portable-C multiply primitives, linked by the
 * CMake host build; the PIC16/PIC18 asm backends provide the same
 * symbols for XC8 target builds, the build selects one. Native wider
 * types make this the independent oracle the tests cross-check the asm
 * algorithms against.
 */

#include "epic_math.h"

/**
 * @brief  8x8 -> 16 unsigned multiply (host oracle).
 * @param  a  multiplicand, 0..255
 * @param  b  multiplier,   0..255
 * @return a*b as a 16-bit value, 0..65535.
 */
uint16_t epic_math_mul_u8(uint8_t a, uint8_t b)
{
    return (uint16_t)a * (uint16_t)b;
}

/**
 * @brief  16x16 -> 32 unsigned multiply (host oracle).
 * @param  a  multiplicand, 0..65535
 * @param  b  multiplier,   0..65535
 * @return a*b as a 32-bit value.
 */
uint32_t epic_math_mul_u16(uint16_t a, uint16_t b)
{
    return (uint32_t)a * (uint32_t)b;
}

/**
 * @brief  16x16 -> 32 signed multiply (host oracle); cannot overflow
 *         since int is 32-bit on the host.
 * @param  a  multiplicand, -32768..32767
 * @param  b  multiplier,   -32768..32767
 * @return (int32_t)a*b.
 */
int32_t epic_math_mul_s16(int16_t a, int16_t b)
{
    /* int is 32-bit on the host, so this cannot overflow. */
    return (int32_t)a * (int32_t)b;
}
