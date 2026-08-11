/*
 * PIC16 inline-asm add/sub/negate primitives. Mid-range PIC16 has no
 * addwfc/subwfb, so carry propagation uses AN526's idiom: btfsc/btfss
 * STATUS,0 + incfsz to fold carry/borrow into the high byte's addend.
 * STATUS bits by number (C=0, Z=2). Operands live in the shared scratch
 * buffer (pic_math_scratch.h), one banksel per routine.
 */

#include <xc.h>
#include "pic_math.h"
#include "pic_math_scratch.h"

/**
 * @brief  16-bit unsigned add with carry out (PIC16 inline asm).
 * @param  a           augend, 0..65535
 * @param  b           addend, 0..65535
 * @param  carry_out  set true on overflow (sum > 65535); may be NULL.
 * @return (a + b) truncated to 16 bits.
 *
 * 16-bit add. Low bytes add (sets C); the high addend is folded with the
 * carry through the btfsc/incfsz idiom, since movf preserves C. Worked
 * example 0xFFFF+0x0002 -> 0x0001, carry 1; the fold is correct for this
 * example (b_hi=0, so incfsz yields b_hi+1 and addwf sums a_hi, while
 * movf keeps C set for the final carry). Offsets a@0, b@2, r@4, co@6. */
uint16_t pic_math_add_u16(uint16_t a, uint16_t b, bool *carry_out) __at(0x2E0)
{
    pic16_mscratch[0] = (uint8_t)a;           pic16_mscratch[1] = (uint8_t)(a >> 8);
    pic16_mscratch[2] = (uint8_t)b;           pic16_mscratch[3] = (uint8_t)(b >> 8);
    pic16_mscratch[6] = 0u;
    asm("banksel _pic16_mscratch");
    asm("movf  _pic16_mscratch+2,w");
    asm("addwf _pic16_mscratch+0,w");
    asm("movwf _pic16_mscratch+4");
    asm("movf  _pic16_mscratch+3,w");
    asm("btfsc STATUS,0");
    asm("incfsz _pic16_mscratch+3,w");  /* b_hi + carry; skip if wrapped */
    asm("addwf _pic16_mscratch+1,w");
    asm("movwf _pic16_mscratch+5");
    asm("clrf  _pic16_mscratch+6");
    asm("btfsc STATUS,0");
    asm("incf  _pic16_mscratch+6,f");
    if (carry_out) *carry_out = (bool)pic16_mscratch[6];
    return (uint16_t)pic16_mscratch[4] | ((uint16_t)pic16_mscratch[5] << 8);
}

/**
 * @brief  16-bit unsigned subtract with borrow out (PIC16 inline asm).
 * @param  a           minuend, 0..65535
 * @param  b           subtrahend, 0..65535
 * @param  borrow_out  set true on underflow (a < b); may be NULL.
 * @return (a - b) truncated to 16 bits.
 *
 * a - b, borrow_out = (a < b). Low bytes subtract (C=0 on borrow); the
 * high byte subtracts b_hi plus the borrow, folded via the btfss/incfsz
 * idiom. Borrow is the final C inverted. Offsets a@0, b@2, r@4, bo@6. */
uint16_t pic_math_sub_u16(uint16_t a, uint16_t b, bool *borrow_out) __at(0x320)
{
    pic16_mscratch[0] = (uint8_t)a;           pic16_mscratch[1] = (uint8_t)(a >> 8);
    pic16_mscratch[2] = (uint8_t)b;           pic16_mscratch[3] = (uint8_t)(b >> 8);
    pic16_mscratch[6] = 0u;
    asm("banksel _pic16_mscratch");
    asm("movf  _pic16_mscratch+2,w");
    asm("subwf _pic16_mscratch+0,w");
    asm("movwf _pic16_mscratch+4");
    asm("movf  _pic16_mscratch+3,w");
    asm("btfss STATUS,0");
    asm("incfsz _pic16_mscratch+3,w");  /* b_hi + borrow; skip if wrapped */
    asm("subwf _pic16_mscratch+1,w");
    asm("movwf _pic16_mscratch+5");
    asm("clrf  _pic16_mscratch+6");
    asm("btfss STATUS,0");
    asm("incf  _pic16_mscratch+6,f");
    if (borrow_out) *borrow_out = (bool)pic16_mscratch[6];
    return (uint16_t)pic16_mscratch[4] | ((uint16_t)pic16_mscratch[5] << 8);
}

/**
 * @brief  16-bit two's-complement negate (PIC16 inline asm).
 * @param  v  value to negate, -32768..32767
 * @return -v; INT16_MIN negates to itself (two's-complement wrap).
 *
 * -v = ~v + 1: complement both bytes, inc low, and inc high only if the
 * low inc wrapped (Z). INT16_MIN negates to itself. Offsets v@0, r@2. */
int16_t pic_math_negate_s16(int16_t v) __at(0x360)
{
    uint16_t uv = (uint16_t)v;
    pic16_mscratch[0] = (uint8_t)uv;          pic16_mscratch[1] = (uint8_t)(uv >> 8);
    asm("banksel _pic16_mscratch");
    asm("comf  _pic16_mscratch+0,w");
    asm("movwf _pic16_mscratch+2");
    asm("comf  _pic16_mscratch+1,w");
    asm("movwf _pic16_mscratch+3");
    asm("incf  _pic16_mscratch+2,f");
    asm("btfsc STATUS,2");
    asm("incf  _pic16_mscratch+3,f");
    return (int16_t)((uint16_t)pic16_mscratch[2] | ((uint16_t)pic16_mscratch[3] << 8));
}

/**
 * @brief  32-bit two's-complement negate (PIC16 inline asm).
 * @param  v  value to negate, -2147483648..2147483647
 * @return -v; INT32_MIN negates to itself (two's-complement wrap).
 *
 * Same ~v + 1 with the carry cascade across 4 bytes. Offsets v@0-3, r@4-7. */
int32_t pic_math_negate_s32(int32_t v) __at(0x380)
{
    uint32_t uv = (uint32_t)v;
    pic16_mscratch[0] = (uint8_t)uv;          pic16_mscratch[1] = (uint8_t)(uv >> 8);
    pic16_mscratch[2] = (uint8_t)(uv >> 16);  pic16_mscratch[3] = (uint8_t)(uv >> 24);
    asm("banksel _pic16_mscratch");
    asm("comf  _pic16_mscratch+0,w");  asm("movwf _pic16_mscratch+4");
    asm("comf  _pic16_mscratch+1,w");  asm("movwf _pic16_mscratch+5");
    asm("comf  _pic16_mscratch+2,w");  asm("movwf _pic16_mscratch+6");
    asm("comf  _pic16_mscratch+3,w");  asm("movwf _pic16_mscratch+7");
    asm("incf  _pic16_mscratch+4,f");
    asm("btfsc STATUS,2");
    asm("incf  _pic16_mscratch+5,f");
    asm("btfsc STATUS,2");
    asm("incf  _pic16_mscratch+6,f");
    asm("btfsc STATUS,2");
    asm("incf  _pic16_mscratch+7,f");
    return (int32_t)((uint32_t)pic16_mscratch[4] | ((uint32_t)pic16_mscratch[5] << 8)
                   | ((uint32_t)pic16_mscratch[6] << 16) | ((uint32_t)pic16_mscratch[7] << 24));
}
