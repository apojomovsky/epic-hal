/*
 * PIC16 inline-asm multiply primitives via AN526's shift-and-add
 * (PIC16F87XA has no hardware multiply): accumulate a<<i for each set bit
 * i of the multiplier. All operands live in the shared scratch buffer
 * (epic_math_scratch.h), one banksel per routine. STATUS bits by number
 * (C=0, Z=2).
 */

#include <xc.h>
#include "epic_math.h"
#include "epic_math_scratch.h"

/* 8x8 -> 16 shift-add. tmp = a (16-bit, shifted left each step -> a<<i);
 * for each set bit i of b, r += tmp. Offsets a@0, b@1, bk@2, cnt@3,
 * r@4-5, t@6-7. */

/* Straight-line (speed) form: one inlined step per multiplier bit. */
#define MUL8_STEP(bit) do {                                   \
    asm("btfss _pic16_mscratch+1," #bit);                      \
    asm("goto  _m8_skip_" #bit);                               \
    asm("movf  _pic16_mscratch+6,w");                          \
    asm("addwf _pic16_mscratch+4,f");   /* rLO += tLO, C */      \
    asm("movf  _pic16_mscratch+7,w");                          \
    asm("btfsc STATUS,0");                                     \
    asm("incfsz _pic16_mscratch+7,w");  /* tHI + carry; skip if wrapped */ \
    asm("addwf _pic16_mscratch+5,f");   /* rHI += tHI + carry */  \
    asm("_m8_skip_" #bit ":");                                \
    asm("bcf   STATUS,0");                                     \
    asm("rlf   _pic16_mscratch+6,f");   /* tmp <<= 1 */          \
    asm("rlf   _pic16_mscratch+7,f");                          \
} while (0)

/**
 * @brief  8x8 -> 16 unsigned multiply via AN526 shift-add (PIC16 inline
 *         asm; PIC16F87XA has no hardware multiply).
 * @param  a  multiplicand, 0..255
 * @param  b  multiplier,   0..255
 * @return a*b as a 16-bit value, 0..65535.
 */
uint16_t epic_math_mul_u8(uint8_t a, uint8_t b) __at(0x100)
{
    pic16_mscratch[0] = a;
    pic16_mscratch[1] = b;
    asm("banksel _pic16_mscratch");
    asm("clrf  _pic16_mscratch+4");          /* r = 0                              */
    asm("clrf  _pic16_mscratch+5");
    asm("clrf  _pic16_mscratch+6");          /* t = 0                              */
    asm("clrf  _pic16_mscratch+7");
    asm("movf  _pic16_mscratch+0,w");
    asm("movwf _pic16_mscratch+6");          /* t = a (16-bit, low)                */

#if EPIC_MATH_OPTIMIZE_FOR_SIZE
    /* Looped form: shift b right, test its LSB each pass. */
    asm("movf  _pic16_mscratch+1,w");        /* bk = b                             */
    asm("movwf _pic16_mscratch+2");
    asm("movlw 8");
    asm("movwf _pic16_mscratch+3");          /* cnt = 8                            */
    asm("_m8_loop:");
    asm("btfss _pic16_mscratch+2,0");
    asm("goto  _m8_lskip");
    asm("movf  _pic16_mscratch+6,w");
    asm("addwf _pic16_mscratch+4,f");
    asm("movf  _pic16_mscratch+7,w");
    asm("btfsc STATUS,0");
    asm("incfsz _pic16_mscratch+7,w");
    asm("addwf _pic16_mscratch+5,f");
    asm("_m8_lskip:");
    asm("bcf   STATUS,0");
    asm("rlf   _pic16_mscratch+6,f");         /* tmp <<= 1                          */
    asm("rlf   _pic16_mscratch+7,f");
    asm("bcf   STATUS,0");
    asm("rrf   _pic16_mscratch+2,f");        /* bk >>= 1                           */
    asm("decfsz _pic16_mscratch+3,f");
    asm("goto  _m8_loop");
#else
    MUL8_STEP(0); MUL8_STEP(1); MUL8_STEP(2); MUL8_STEP(3);
    MUL8_STEP(4); MUL8_STEP(5); MUL8_STEP(6); MUL8_STEP(7);
#endif

    return (uint16_t)pic16_mscratch[4] | ((uint16_t)pic16_mscratch[5] << 8);
}

/**
 * @brief  16x16 -> 32 unsigned multiply via AN526 shift-add (PIC16
 *         inline asm).
 * @param  a  multiplicand, 0..65535
 * @param  b  multiplier,   0..65535
 * @return a*b as a 32-bit value.
 *
 * 16x16 -> 32 shift-add, 16 iterations. tmp = a in a 32-bit register
 * (shifted left -> a<<i); for each set bit of b, r += tmp (32-bit add,
 * carry idiom across 4 bytes). Offsets a@0-1 (a_lo reused as cnt after
 * the t copy), b@2-3, bk@4-5, r@6-9, t@10-13. */
uint32_t epic_math_mul_u16(uint16_t a, uint16_t b) __at(0x2C0)
{
    pic16_mscratch[0] = (uint8_t)a;           pic16_mscratch[1] = (uint8_t)(a >> 8);
    pic16_mscratch[2] = (uint8_t)b;           pic16_mscratch[3] = (uint8_t)(b >> 8);
    asm("banksel _pic16_mscratch");
    asm("clrf  _pic16_mscratch+4");           /* r = 0                              */
    asm("clrf  _pic16_mscratch+5");
    asm("clrf  _pic16_mscratch+6");
    asm("clrf  _pic16_mscratch+7");
    asm("clrf  _pic16_mscratch+8");          /* t = 0                              */
    asm("clrf  _pic16_mscratch+9");
    asm("clrf  _pic16_mscratch+10");
    asm("clrf  _pic16_mscratch+11");
    asm("movf  _pic16_mscratch+0,w");
    asm("movwf _pic16_mscratch+8");
    asm("movf  _pic16_mscratch+1,w");
    asm("movwf _pic16_mscratch+9");          /* t = a (32-bit, low 16)              */
    /* bk = b: b lives at 2-3, which is bk's own slot. */
    asm("movlw 16");
    asm("movwf _pic16_mscratch+0");           /* cnt = 16 (a_lo slot, dead after
                                                 the t = a copy above)              */
    asm("_m16_loop:");
    /* if bk LSB set, r += tmp (32-bit add, carry idiom across 4 bytes) */
    asm("btfss _pic16_mscratch+2,0");
    asm("goto  _m16_skip");
    asm("movf  _pic16_mscratch+8,w");
    asm("addwf _pic16_mscratch+4,f");
    asm("movf  _pic16_mscratch+9,w");
    asm("btfsc STATUS,0");
    asm("incfsz _pic16_mscratch+9,w");
    asm("addwf _pic16_mscratch+5,f");
    asm("movf  _pic16_mscratch+10,w");
    asm("btfsc STATUS,0");
    asm("incfsz _pic16_mscratch+10,w");
    asm("addwf _pic16_mscratch+6,f");
    asm("movf  _pic16_mscratch+11,w");
    asm("btfsc STATUS,0");
    asm("incfsz _pic16_mscratch+11,w");
    asm("addwf _pic16_mscratch+7,f");
    asm("_m16_skip:");
    /* tmp <<= 1 (32-bit left shift through carry, LSB <- 0) */
    asm("bcf   STATUS,0");
    asm("rlf   _pic16_mscratch+8,f");
    asm("rlf   _pic16_mscratch+9,f");
    asm("rlf   _pic16_mscratch+10,f");
    asm("rlf   _pic16_mscratch+11,f");
    /* bk >>= 1 (test the next multiplier bit) */
    asm("bcf   STATUS,0");
    asm("rrf   _pic16_mscratch+3,f");
    asm("rrf   _pic16_mscratch+2,f");
    asm("decfsz _pic16_mscratch+0,f");
    asm("goto  _m16_loop");

    return (uint32_t)pic16_mscratch[4] | ((uint32_t)pic16_mscratch[5] << 8)
         | ((uint32_t)pic16_mscratch[6] << 16) | ((uint32_t)pic16_mscratch[7] << 24);
}

/**
 * @brief  16x16 -> 32 signed multiply on top of the asm unsigned path
 *         (PIC16).
 * @param  a  multiplicand, -32768..32767
 * @param  b  multiplier,   -32768..32767
 * @return (int32_t)a*b.
 *
 * abs the operands (unsigned, so INT16_MIN abs = 0x8000 with no
 * 16-bit-int overflow), call mul_u16, and negate the 32-bit result if
 * the signs differed. */
int32_t epic_math_mul_s16(int16_t a, int16_t b)
{
    int neg = ((a < 0) != 0) ^ ((b < 0) != 0);
    uint16_t ua = (a < 0) ? (uint16_t)(0u - (uint16_t)a) : (uint16_t)a;
    uint16_t ub = (b < 0) ? (uint16_t)(0u - (uint16_t)b) : (uint16_t)b;
    uint32_t ur = epic_math_mul_u16(ua, ub);
    if (neg) {
        ur = (uint32_t)epic_math_negate_s32((int32_t)ur);
    }
    return (int32_t)ur;
}
