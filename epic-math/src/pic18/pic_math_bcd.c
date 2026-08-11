/*
 * PIC18 inline-asm BCD primitives. Conversions are C built on this
 * backend's asm mul/div; only the digit adjust is asm: PIC18 has daw
 * (one-instruction packed-BCD adjust after an add), so bcd_add8 is
 * addwf + daw; bcd_sub8 has no hardware equivalent on any core, so it
 * does the manual nibble-wise subtract-with-borrow.
 */

#include <xc.h>
#include "pic_math.h"

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

/* addwf then daw (PIC18 packed-BCD adjust), C set if the BCD sum > 99.
 * Scratch struct m_ba offsets: a@0, b@1, r@2, co@3 (4 bytes). */
static volatile struct { uint8_t a, b, r, co; } m_ba;

uint8_t pic_math_bcd_add8(uint8_t a, uint8_t b, bool *carry_out)
{
    m_ba.a = a; m_ba.b = b; m_ba.co = 0;
    asm("banksel _m_ba");
    asm("movf  (_m_ba)+0,w");        /* W = a                              */
    asm("addwf (_m_ba)+1,w");        /* W = a + b (binary)                 */
    asm("daw");                       /* packed-BCD adjust W; C if >99      */
    asm("movwf (_m_ba)+2");          /* r = BCD sum                        */
    asm("clrf  (_m_ba)+3");
    asm("btfsc STATUS,0");           /* if C=1 (BCD carry out)             */
    asm("incf  (_m_ba)+3,f");        /* co = 1                             */
    if (carry_out) *carry_out = (bool)m_ba.co;
    return m_ba.r;
}

/* a - b, BCD, nibble-wise subtract with borrow. borrow_out = (a < b) in
 * decimal. Scratch struct m_bs offsets: a@0, b@1, r@2, bo@3, aL@4, bL@5,
 * br@6, aH@7 (8 bytes).
 *   dL = aL - bL; if borrow: dL += 10, br=1
 *   dH = aH - bH - br; if borrow: dH += 10, bo=1
 *   r = (dH<<4)|dL
 * Worked example 0x12-0x34 -> 0x78, borrow 1 pins the +10 adjust and
 * the borrow ripple from low to high nibble. */
static volatile struct { uint8_t a, b, r, bo, aL, bL, br, aH; } m_bs;

uint8_t pic_math_bcd_sub8(uint8_t a, uint8_t b, bool *borrow_out)
{
    m_bs.a = a; m_bs.b = b; m_bs.bo = 0; m_bs.br = 0;
    asm("banksel _m_bs");
    /* aL = a & 0x0F, bL = b & 0x0F */
    asm("movf  (_m_bs)+0,w");
    asm("andlw 0x0F");
    asm("movwf (_m_bs)+4");
    asm("movf  (_m_bs)+1,w");
    asm("andlw 0x0F");
    asm("movwf (_m_bs)+5");
    /* dL = aL - bL (in aL), +10 on borrow, br */
    asm("movf  (_m_bs)+5,w");
    asm("subwf (_m_bs)+4,f");        /* aL = aL - bL, C=1 no borrow        */
    asm("movlw 10");
    asm("btfsc STATUS,0");           /* if C=1 (no borrow), skip +10        */
    asm("bra   _bs_lo_ok");
    asm("addwf (_m_bs)+4,f");        /* aL += 10                           */
    asm("incf  (_m_bs)+6,f");         /* br = 1                             */
    asm("_bs_lo_ok:");
    /* aH = tens_a; W = tens_b + br; aH = tens_a - W (the full high subtract) */
    asm("swapf (_m_bs)+0,w");
    asm("andlw 0x0F");
    asm("movwf (_m_bs)+7");          /* aH = tens_a                        */
    asm("swapf (_m_bs)+1,w");
    asm("andlw 0x0F");              /* W = tens_b                         */
    asm("addwf (_m_bs)+6,w");        /* W = tens_b + br                    */
    asm("subwf (_m_bs)+7,f");        /* aH = tens_a - (tens_b + br), C=1 ok*/
    asm("movlw 10");
    asm("btfsc STATUS,0");           /* if C=1 (no borrow), skip +10      */
    asm("bra   _bs_hi_ok");
    asm("addwf (_m_bs)+7,f");        /* aH += 10                           */
    asm("incf  (_m_bs)+3,f");        /* bo = 1                             */
    asm("_bs_hi_ok:");
    /* r = (aH << 4) | aL */
    asm("swapf (_m_bs)+7,w");        /* W = aH << 4 (low nibble -> high)   */
    asm("iorwf (_m_bs)+4,w");        /* W |= aL                            */
    asm("movwf (_m_bs)+2");
    if (borrow_out) *borrow_out = (bool)m_bs.bo;
    return m_bs.r;
}
