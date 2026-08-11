/*
 * PIC16 inline-asm BCD primitives. Conversions are C built on this
 * backend's asm mul/div; only the digit adjust is asm, because mid-range
 * PIC16 has no daw instruction: bcd_add8 does a manual +6 nibble adjust
 * and bcd_sub8 a nibble-wise subtract with borrow. Defined for valid BCD
 * operands (0..99) only.
 */

#include <xc.h>
#include "pic_math.h"
#include "pic_math_scratch.h"

uint8_t pic_math_bcd8_to_bin(uint8_t bcd2)
{
    uint8_t tens = (uint8_t)(bcd2 >> 4);
    uint8_t ones = (uint8_t)(bcd2 & 0x0Fu);
    return (uint8_t)(pic_math_mul_u8(tens, 10u) + ones);
}

uint8_t pic_math_bin_to_bcd8(uint8_t value)
{
    pic_math_udiv16_t d = pic_math_divmod_u16((uint16_t)value, 10u, NULL);
    return (uint8_t)((d.quotient << 4) | (d.remainder & 0x0Fu));
}

uint16_t pic_math_bcd16_to_bin(uint32_t bcd5)
{
    uint32_t bin = 0u;
    uint16_t place = 1u;
    for (int i = 0; i < 5; i++) {
        bin += pic_math_mul_u16((uint16_t)(bcd5 & 0x0Fu), place);
        bcd5 >>= 4;
        place = (uint16_t)(place * 10u);
    }
    return (uint16_t)bin;
}

uint32_t pic_math_bin_to_bcd16(uint16_t value)
{
    uint32_t bcd = 0u;
    for (int i = 0; i < 5; i++) {
        pic_math_udiv16_t d = pic_math_divmod_u16(value, 10u, NULL);
        bcd |= (uint32_t)(d.remainder & 0x0Fu) << (i * 4);
        value = d.quotient;
    }
    return bcd;
}

/* Manual DAW: add nibbles separately, +6 on any nibble sum > 9, rippling
 * the carry into the high nibble and out. r = (hi<<4)|lo; co set when the
 * high nibble adjusted. Offsets a@0, b@1, r@2, co@3, lo@4, hi@5. */
uint8_t pic_math_bcd_add8(uint8_t a, uint8_t b, bool *carry_out) __at(0x400)
{
    pic16_mscratch[0] = a; pic16_mscratch[1] = b;
    pic16_mscratch[3] = 0u;
    asm("banksel _pic16_mscratch");
    asm("movf  _pic16_mscratch+0,w");
    asm("andlw 0x0F");
    asm("movwf _pic16_mscratch+4");          /* lo = a_lo                            */
    asm("movf  _pic16_mscratch+1,w");
    asm("andlw 0x0F");
    asm("addwf _pic16_mscratch+4,f");        /* lo = a_lo + b_lo                     */
    /* if lo > 9: lo += 6, carry=1 */
    asm("clrf  _pic16_mscratch+3");           /* co = 0 (reused as the low carry)     */
    asm("movlw 10");
    asm("subwf _pic16_mscratch+4,w");        /* w = lo - 10, C=1 if lo>=10           */
    asm("btfss STATUS,0");
    asm("goto  _ba_lo_ok");
    asm("movlw 6");
    asm("addwf _pic16_mscratch+4,f");        /* lo += 6                              */
    asm("incf  _pic16_mscratch+3,f");        /* carry = 1                            */
    asm("_ba_lo_ok:");
    /* hi = (a>>4) + (b>>4) + carry */
    asm("swapf _pic16_mscratch+0,w");
    asm("andlw 0x0F");
    asm("movwf _pic16_mscratch+5");          /* hi = a_hi                            */
    asm("swapf _pic16_mscratch+1,w");
    asm("andlw 0x0F");
    asm("addwf _pic16_mscratch+5,f");        /* hi += b_hi                           */
    asm("movf  _pic16_mscratch+3,w");
    asm("addwf _pic16_mscratch+5,f");        /* hi += carry                          */
    /* if hi > 9: hi += 6, co=1 */
    asm("clrf  _pic16_mscratch+3");           /* co = 0                               */
    asm("movlw 10");
    asm("subwf _pic16_mscratch+5,w");        /* w = hi - 10, C=1 if hi>=10           */
    asm("btfss STATUS,0");
    asm("goto  _ba_hi_ok");
    asm("movlw 6");
    asm("addwf _pic16_mscratch+5,f");        /* hi += 6                              */
    asm("incf  _pic16_mscratch+3,f");        /* co = 1                               */
    asm("_ba_hi_ok:");
    /* r = ((hi&0xF)<<4) | (lo&0xF) */
    asm("movf  _pic16_mscratch+5,w");
    asm("andlw 0x0F");
    asm("movwf _pic16_mscratch+2");
    asm("swapf _pic16_mscratch+2,f");        /* r = adjusted_high << 4              */
    asm("movf  _pic16_mscratch+4,w");
    asm("andlw 0x0F");
    asm("iorwf _pic16_mscratch+2,f");        /* r |= adjusted_low                    */
    if (carry_out) *carry_out = (bool)pic16_mscratch[3];
    return pic16_mscratch[2];
}

/* a - b, BCD, nibble-wise subtract with borrow. Each nibble pair
 * subtracts; on borrow the digit is +10-adjusted and the borrow carries
 * into the high nibble, becoming bo. Offsets a@0, b@1, r@2, bo@3, aL@4,
 * bL@5, br@6, aH@7. */
uint8_t pic_math_bcd_sub8(uint8_t a, uint8_t b, bool *borrow_out) __at(0x450)
{
    pic16_mscratch[0] = a; pic16_mscratch[1] = b;
    pic16_mscratch[3] = 0u; pic16_mscratch[6] = 0u;
    asm("banksel _pic16_mscratch");
    asm("movf  _pic16_mscratch+0,w");
    asm("andlw 0x0F");
    asm("movwf _pic16_mscratch+4");          /* aL = a_lo                            */
    asm("movf  _pic16_mscratch+1,w");
    asm("andlw 0x0F");
    asm("movwf _pic16_mscratch+5");          /* bL = b_lo                            */
    asm("movf  _pic16_mscratch+5,w");
    asm("subwf _pic16_mscratch+4,f");        /* aL = aL - bL, C=1 no borrow          */
    asm("movlw 10");
    asm("btfsc STATUS,0");
    asm("goto  _bs_lo_ok");
    asm("addwf _pic16_mscratch+4,f");        /* aL += 10                            */
    asm("incf  _pic16_mscratch+6,f");        /* br = 1                              */
    asm("_bs_lo_ok:");
    asm("swapf _pic16_mscratch+0,w");
    asm("andlw 0x0F");
    asm("movwf _pic16_mscratch+7");          /* aH = tens_a                          */
    asm("swapf _pic16_mscratch+1,w");
    asm("andlw 0x0F");                       /* W = tens_b                          */
    asm("addwf _pic16_mscratch+6,w");        /* W = tens_b + br                      */
    asm("subwf _pic16_mscratch+7,f");        /* aH = tens_a - (tens_b + br), C=1 ok */
    asm("movlw 10");
    asm("btfsc STATUS,0");
    asm("goto  _bs_hi_ok");
    asm("addwf _pic16_mscratch+7,f");        /* aH += 10                            */
    asm("incf  _pic16_mscratch+3,f");        /* bo = 1                              */
    asm("_bs_hi_ok:");
    asm("swapf _pic16_mscratch+7,w");        /* W = aH << 4                          */
    asm("iorwf _pic16_mscratch+4,w");        /* W |= aL (dL, in low nibble)           */
    asm("movwf _pic16_mscratch+2");
    if (borrow_out) *borrow_out = (bool)pic16_mscratch[3];
    return pic16_mscratch[2];
}
