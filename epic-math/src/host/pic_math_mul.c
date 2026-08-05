/**
 * @file    pic_math_mul.c (host reference backend)
 * @brief   Portable-C multiply primitives, linked by the CMake host
 *          build; the PIC16/PIC18 asm backends provide the same symbols
 *          for XC8 target builds, the build selects one. Native wider
 *          types make this the independent oracle the tests cross-check
 *          the asm algorithms against.
 */

#include "pic_math.h"

uint16_t pic_math_mul_u8(uint8_t a, uint8_t b)
{
    return (uint16_t)a * (uint16_t)b;
}

uint32_t pic_math_mul_u16(uint16_t a, uint16_t b)
{
    return (uint32_t)a * (uint32_t)b;
}

int32_t pic_math_mul_s16(int16_t a, int16_t b)
{
    /* int is 32-bit on the host, so this cannot overflow. */
    return (int32_t)a * (int32_t)b;
}
