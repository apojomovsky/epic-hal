/**
 * @file    pic_math_mul.c (PIC16 inline-asm backend)
 * @brief   Multiply primitives via AN526's shift-and-add (PIC16F87XA has
 *          no hardware multiply): accumulate a<<i for each set bit i of
 *          the multiplier. All operands live in the shared scratch buffer
 *          (pic_math_scratch.h), one `banksel` per routine. STATUS bits
 *          by number (C=0, Z=2).
 */

#include <xc.h>
#include "pic_math.h"
#include "pic_math_scratch.h"

/* ─── pic_math_mul_u8 ────────────────────────────────────────────
 * 8x8 -> 16 shift-add. tmp = a (16-bit, shifted left each step -> a<<i); for
 * each set bit i of b, r += tmp. Offsets a@0,b@1,bk@2,cnt@3,r@4-5,t@6-7.
 * Hand-trace a=0x0C b=0x14 (12*20=240=0x00F0): b=0b00010100, bits 2,4 set.
 *   t=0x000C. bit0,1 clear (t->0x30). bit2 set: r+=0x30; t->0x60. bit3 clear
 *   (t->0xC0). bit4 set: r+=0xC0 -> 0x30+0xC0=0xF0 -> r=0x00F0.            */

/* Straight-line (speed) form: one inlined step per multiplier bit. */
#define MUL8_STEP(bit) do {                                   \
    asm("btfss _pic16_mscratch+1," #bit);                      \
    asm("goto  _m8_skip_" #bit);                               \
    asm("movf  _pic16_mscratch+6,w");   /* tLO */               \
    asm("addwf _pic16_mscratch+4,f");   /* rLO += tLO, C */      \
    asm("movf  _pic16_mscratch+7,w");   /* tHI */               \
    asm("btfsc STATUS,0");                                     \
    asm("incf  _pic16_mscratch+7,w");   /* tHI + carry */        \
    asm("addwf _pic16_mscratch+5,f");   /* rHI += tHI + carry */  \
    asm("_m8_skip_" #bit ":");                                \
    asm("bcf   STATUS,0");                                     \
    asm("rlf   _pic16_mscratch+6,f");   /* tmp <<= 1 */          \
    asm("rlf   _pic16_mscratch+7,f");                          \
} while (0)

uint16_t pic_math_mul_u8(uint8_t a, uint8_t b)
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

#if PIC_MATH_OPTIMIZE_FOR_SIZE
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
    asm("incf  _pic16_mscratch+7,w");
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

/* ─── pic_math_mul_u16 ───────────────────────────────────────────
 * 16x16 -> 32 shift-add, 16 iterations. tmp = a in a 32-bit register
 * (shifted left -> a<<i); for each set bit of b, r += tmp (32-bit add, carry
 * idiom across 4 bytes). Offsets a@0-1,b@2-3,bk@4-5,r@6-9,t@10-13,cnt@14.
 * Hand-trace a=0x0102 b=0x0103 (258*259=66822=0x00010506): b bits 0,1,8 set.
 *   r += a<<0 (0x102) + a<<1 (0x204) + a<<8 (0x10200) = 0x10506.           */
uint32_t pic_math_mul_u16(uint16_t a, uint16_t b)
{
    pic16_mscratch[0] = (uint8_t)a;           pic16_mscratch[1] = (uint8_t)(a >> 8);
    pic16_mscratch[2] = (uint8_t)b;           pic16_mscratch[3] = (uint8_t)(b >> 8);
    asm("banksel _pic16_mscratch");
    asm("clrf  _pic16_mscratch+6");           /* r = 0                              */
    asm("clrf  _pic16_mscratch+7");
    asm("clrf  _pic16_mscratch+8");
    asm("clrf  _pic16_mscratch+9");
    asm("clrf  _pic16_mscratch+10");          /* t = 0                              */
    asm("clrf  _pic16_mscratch+11");
    asm("clrf  _pic16_mscratch+12");
    asm("clrf  _pic16_mscratch+13");
    asm("movf  _pic16_mscratch+0,w");
    asm("movwf _pic16_mscratch+10");
    asm("movf  _pic16_mscratch+1,w");
    asm("movwf _pic16_mscratch+11");          /* t = a (32-bit, low 16)              */
    asm("movf  _pic16_mscratch+2,w");
    asm("movwf _pic16_mscratch+4");
    asm("movf  _pic16_mscratch+3,w");
    asm("movwf _pic16_mscratch+5");           /* bk = b                             */
    asm("movlw 16");
    asm("movwf _pic16_mscratch+14");          /* cnt = 16                           */
    asm("_m16_loop:");
    /* if bk LSB set, r += tmp (32-bit add, carry idiom across 4 bytes) */
    asm("btfss _pic16_mscratch+4,0");
    asm("goto  _m16_skip");
    asm("movf  _pic16_mscratch+10,w");
    asm("addwf _pic16_mscratch+6,f");
    asm("movf  _pic16_mscratch+11,w");
    asm("btfsc STATUS,0");
    asm("incf  _pic16_mscratch+11,w");
    asm("addwf _pic16_mscratch+7,f");
    asm("movf  _pic16_mscratch+12,w");
    asm("btfsc STATUS,0");
    asm("incf  _pic16_mscratch+12,w");
    asm("addwf _pic16_mscratch+8,f");
    asm("movf  _pic16_mscratch+13,w");
    asm("btfsc STATUS,0");
    asm("incf  _pic16_mscratch+13,w");
    asm("addwf _pic16_mscratch+9,f");
    asm("_m16_skip:");
    /* tmp <<= 1 (32-bit left shift through carry, LSB <- 0) */
    asm("bcf   STATUS,0");
    asm("rlf   _pic16_mscratch+10,f");
    asm("rlf   _pic16_mscratch+11,f");
    asm("rlf   _pic16_mscratch+12,f");
    asm("rlf   _pic16_mscratch+13,f");
    /* bk >>= 1 (test the next multiplier bit) */
    asm("bcf   STATUS,0");
    asm("rrf   _pic16_mscratch+4,f");
    asm("rrf   _pic16_mscratch+5,f");
    asm("decfsz _pic16_mscratch+14,f");
    asm("goto  _m16_loop");

    return (uint32_t)pic16_mscratch[6] | ((uint32_t)pic16_mscratch[7] << 8)
         | ((uint32_t)pic16_mscratch[8] << 16) | ((uint32_t)pic16_mscratch[9] << 24);
}

/* ─── pic_math_mul_s16 ──────────────────────────────────────────
 * Signed 16x16 on top of the asm unsigned path: abs the operands (unsigned,
 * so INT16_MIN abs = 0x8000 with no 16-bit-int overflow), call mul_u16, and
 * negate the 32-bit result if the signs differed. Plain C calling this
 * backend's asm primitives (mul_u16 + negate_s32). */
int32_t pic_math_mul_s16(int16_t a, int16_t b)
{
    int neg = ((a < 0) != 0) ^ ((b < 0) != 0);
    uint16_t ua = (a < 0) ? (uint16_t)(0u - (uint16_t)a) : (uint16_t)a;
    uint16_t ub = (b < 0) ? (uint16_t)(0u - (uint16_t)b) : (uint16_t)b;
    uint32_t ur = pic_math_mul_u16(ua, ub);
    if (neg) {
        ur = (uint32_t)pic_math_negate_s32((int32_t)ur);
    }
    return (int32_t)ur;
}
