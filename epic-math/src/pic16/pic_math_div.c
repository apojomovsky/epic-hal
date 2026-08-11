/*
 * PIC16 inline-asm divide/modulo primitives: a restoring shift-subtract
 * loop (same algorithm as the PIC18 backend and AN526/AN544), using
 * mid-range PIC16 instructions (no subwfb, so borrow is the btfss
 * STATUS,0/incf idiom). Mirrors the machine-verified C reference in
 * tests/test_div.c. STATUS bits by number (C=0, Z=2).
 */

#include <xc.h>
#include "pic_math.h"
#include "pic_math_scratch.h"

/**
 * @brief  Unsigned 16/16 divide with remainder (PIC16 inline asm).
 * @param  num  numerator,   0..65535
 * @param  den  denominator, 1..65535 (0 -> *ok=false, fields zeroed)
 * @param  ok   out: true if den != 0; may be NULL
 * @return { quotient = num/den, remainder = num%den }.
 *
 * 16/16 restoring division, 16 iterations. Offsets num@0-1, rem@2-3,
 * den@4-5, cnt@6. One iteration, mirroring ref_divmod_u16_algo:
 *   c = num>>15; num <<= 1; rem = (rem<<1)|c;
 *   if (rem>=den) { rem -= den; num |= 1; }
 * For a 16-bit dividend the partial remainder never reaches 0x8000, so
 * the rem shift never carries out and the 16-bit compare is exact. */
pic_math_udiv16_t pic_math_divmod_u16(uint16_t num, uint16_t den, bool *ok) __at(0x1C0)
{
    pic_math_udiv16_t res = { 0u, 0u };
    if (den == 0u) { if (ok) *ok = false; return res; }
    if (ok) *ok = true;

    pic16_mscratch[0] = (uint8_t)num;          pic16_mscratch[1] = (uint8_t)(num >> 8);
    pic16_mscratch[4] = (uint8_t)den;          pic16_mscratch[5] = (uint8_t)(den >> 8);
    asm("banksel _pic16_mscratch");
    asm("clrf  _pic16_mscratch+2");            /* rem = 0                              */
    asm("clrf  _pic16_mscratch+3");
    asm("movlw 16");
    asm("movwf _pic16_mscratch+6");            /* cnt = 16                             */
    asm("_du16_loop:");
    asm("bcf   STATUS,0");
    asm("rlf   _pic16_mscratch+0,f");
    asm("rlf   _pic16_mscratch+1,f");
    asm("rlf   _pic16_mscratch+2,f");
    asm("rlf   _pic16_mscratch+3,f");
    /* subtract den from rem in place; C=1 if rem>=den */
    asm("movf  _pic16_mscratch+4,w");
    asm("subwf _pic16_mscratch+2,f");
    asm("movf  _pic16_mscratch+5,w");
    asm("btfss STATUS,0");
    asm("incfsz _pic16_mscratch+5,w");  /* den_hi + borrow; skip if wrapped */
    asm("subwf _pic16_mscratch+3,f");
    /* C=1: set quotient bit; C=0: restore rem += den */
    asm("btfss STATUS,0");
    asm("goto  _du16_restore");
    asm("bsf   _pic16_mscratch+0,0");
    asm("goto  _du16_next");
    asm("_du16_restore:");
    asm("movf  _pic16_mscratch+4,w");
    asm("addwf _pic16_mscratch+2,f");
    asm("movf  _pic16_mscratch+5,w");
    asm("btfsc STATUS,0");
    asm("incfsz _pic16_mscratch+5,w");  /* den_hi + carry; skip if wrapped */
    asm("addwf _pic16_mscratch+3,f");
    asm("_du16_next:");
    asm("decfsz _pic16_mscratch+6,f");
    asm("goto  _du16_loop");

    res.quotient  = (uint16_t)pic16_mscratch[0] | ((uint16_t)pic16_mscratch[1] << 8);
    res.remainder = (uint16_t)pic16_mscratch[2] | ((uint16_t)pic16_mscratch[3] << 8);
    return res;
}

/**
 * @brief  Wide unsigned 32/16 divide with remainder (PIC16 inline asm);
 *         quotient truncated to 16 bits (documented contract).
 * @param  num  numerator,   0..0xFFFFFFFF
 * @param  den  denominator, 1..65535 (0 -> *ok=false, fields zeroed)
 * @param  ok   out: true if den != 0; may be NULL
 * @return { quotient = low 16 bits of num/den, remainder = num%den }.
 *
 * 32/16 restoring division, 32 iterations. Offsets num@0-3, rem@4-5,
 * den@6-7, cnt@8. The partial remainder can reach 0x8000, so the rem
 * shift can carry out: the extended remainder is C:rem (17 bits). If C=1
 * after the shift, subtract unconditionally (rem wraps to the low-16 of
 * extended-den) and set the bit; else do the plain 16-bit restoring
 * subtract. Quotient truncated to 16 bits (low word). Mirrors
 * ref_divmod_u32_16_algo. */
pic_math_udiv16_t pic_math_divmod_u32_16(uint32_t num, uint16_t den, bool *ok) __at(0x240)
{
    pic_math_udiv16_t res = { 0u, 0u };
    if (den == 0u) { if (ok) *ok = false; return res; }
    if (ok) *ok = true;

    pic16_mscratch[0] = (uint8_t)num;           pic16_mscratch[1] = (uint8_t)(num >> 8);
    pic16_mscratch[2] = (uint8_t)(num >> 16);   pic16_mscratch[3] = (uint8_t)(num >> 24);
    pic16_mscratch[6] = (uint8_t)den;           pic16_mscratch[7] = (uint8_t)(den >> 8);
    asm("banksel _pic16_mscratch");
    asm("clrf  _pic16_mscratch+4");            /* rem = 0                              */
    asm("clrf  _pic16_mscratch+5");
    asm("movlw 32");
    asm("movwf _pic16_mscratch+8");            /* cnt = 32                             */
    asm("_du32_loop:");
    asm("bcf   STATUS,0");
    asm("rlf   _pic16_mscratch+0,f");
    asm("rlf   _pic16_mscratch+1,f");
    asm("rlf   _pic16_mscratch+2,f");
    asm("rlf   _pic16_mscratch+3,f");
    asm("rlf   _pic16_mscratch+4,f");
    asm("rlf   _pic16_mscratch+5,f");           /* C = old MSB of rem (the 17th bit)     */
    /* if C=1: force subtract (extended >= 0x10000 > den) */
    asm("btfss STATUS,0");
    asm("goto  _du32_cmp");
    asm("movf  _pic16_mscratch+6,w");
    asm("subwf _pic16_mscratch+4,f");
    asm("movf  _pic16_mscratch+7,w");
    asm("btfss STATUS,0");
    asm("incfsz _pic16_mscratch+7,w");
    asm("subwf _pic16_mscratch+5,f");
    asm("bsf   _pic16_mscratch+0,0");
    asm("goto  _du32_next");
    /* C=0: plain 16-bit restoring subtract */
    asm("_du32_cmp:");
    asm("movf  _pic16_mscratch+6,w");
    asm("subwf _pic16_mscratch+4,f");
    asm("movf  _pic16_mscratch+7,w");
    asm("btfss STATUS,0");
    asm("incfsz _pic16_mscratch+7,w");
    asm("subwf _pic16_mscratch+5,f");
    asm("btfss STATUS,0");
    asm("goto  _du32_restore");
    asm("bsf   _pic16_mscratch+0,0");
    asm("goto  _du32_next");
    asm("_du32_restore:");
    asm("movf  _pic16_mscratch+6,w");
    asm("addwf _pic16_mscratch+4,f");
    asm("movf  _pic16_mscratch+7,w");
    asm("btfsc STATUS,0");
    asm("incfsz _pic16_mscratch+7,w");
    asm("addwf _pic16_mscratch+5,f");
    asm("_du32_next:");
    asm("decfsz _pic16_mscratch+8,f");
    asm("goto  _du32_loop");

    res.quotient  = (uint16_t)pic16_mscratch[0] | ((uint16_t)pic16_mscratch[1] << 8);
    res.remainder = (uint16_t)pic16_mscratch[4] | ((uint16_t)pic16_mscratch[5] << 8);
    return res;
}

/**
 * @brief  Signed 16/16 divide with remainder over the asm unsigned path
 *         (PIC16).
 * @param  num  numerator,   -32768..32767
 * @param  den  denominator, nonzero (0 -> *ok=false, fields zeroed)
 * @param  ok   out: true if den != 0; may be NULL
 * @return { quotient = num/den, remainder = num%den }.
 *
 * Quotient sign = sign(num) ^ sign(den); remainder sign follows the
 * dividend (C99 truncated division). INT16_MIN / -1 -> quotient 0x8000
 * (wrap of 32768), remainder 0. */
pic_math_sdiv16_t pic_math_divmod_s16(int16_t num, int16_t den, bool *ok)
{
    pic_math_sdiv16_t res = { 0, 0 };
    if (den == 0) { if (ok) *ok = false; return res; }
    if (ok) *ok = true;

    int neg_q = ((num < 0) != 0) ^ ((den < 0) != 0);
    uint16_t unum = (num < 0) ? (uint16_t)(0u - (uint16_t)num) : (uint16_t)num;
    uint16_t uden = (den < 0) ? (uint16_t)(0u - (uint16_t)den) : (uint16_t)den;

    pic_math_udiv16_t u = pic_math_divmod_u16(unum, uden, NULL);
    res.quotient  = (int16_t)(neg_q ? (uint16_t)(0u - u.quotient)  : u.quotient);
    res.remainder = (int16_t)((num < 0) ? (uint16_t)(0u - u.remainder) : u.remainder);
    return res;
}
