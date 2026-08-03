/**
 * @file    pic_math_addsub.c (PIC18 inline-asm backend)
 * @brief   Add/sub/negate primitives using PIC18's single-instruction
 *          carry ops (`addwfc`/`subwfb`), unlike PIC16's skip-and-increment
 *          idiom. Operands are file-scope `static volatile` scratch, one
 *          `banksel` per routine (see ARCHITECTURE.md "Inline-asm binding").
 */

#include <xc.h>
#include "pic_math.h"

/* ─── pic_math_add_u16 ───────────────────────────────────────────
 * Hand-trace (PIC18), a=0xFFFF b=0x0002 -> sum 0x10001, r=0x0001, carry=1:
 *   movf  b+0,w   ; w=0x02
 *   addwf a+0,w   ; w=0xFF+0x02=0x01, C=1 (overflow)   ; C=1
 *   movwf r+0     ; r_lo=0x01
 *   movf  b+1,w   ; w=0x00 (movf keeps C)              ; C=1
 *   addwfc a+1,w  ; w=0xFF+0x00+1=0x00, C=1            ; C=1
 *   movwf r+1     ; r_hi=0x00  -> r=0x0001
 *   clrf co; btfsc STATUS,0 (C=1, don't skip); setf co  ; co=1
 * Matches the host oracle: (0xFFFF+0x0002)=0x10001 -> 0x0001, carry=1.       */
static volatile uint16_t m_add_a, m_add_b, m_add_r;
static volatile uint8_t  m_add_co;

uint16_t pic_math_add_u16(uint16_t a, uint16_t b, bool *carry_out)
{
    m_add_a = a; m_add_b = b; m_add_co = 0;
    asm("banksel _m_add_a");
    asm("movf   _m_add_b+0,w");        /* w = bLO                              */
    asm("addwf  _m_add_a+0,w");        /* w = aLO + bLO, C = carry-out        */
    asm("movwf  _m_add_r+0");          /* rLO                                 */
    asm("movf   _m_add_b+1,w");        /* w = bHI (movf preserves C)          */
    asm("addwfc _m_add_a+1,w");        /* w = aHI + bHI + C, C = final carry  */
    asm("movwf  _m_add_r+1");          /* rHI                                 */
    asm("clrf   _m_add_co");
    asm("btfsc  STATUS,0");            /* if C=1 (carry out)                  */
    asm("setf   _m_add_co");
    if (carry_out) *carry_out = (bool)m_add_co;
    return m_add_r;
}

/* ─── pic_math_sub_u16 ───────────────────────────────────────────
 * a - b, borrow_out = (a < b). Hand-trace a=0x0002 b=0xFFFF -> r=0x0003,
 * borrow=1:
 *   movf  b+0,w   ; w=0xFF
 *   subwf a+0,w   ; w=0x02-0xFF=0x03, C=0 (borrow)         ; C=0
 *   movwf r+0     ; r_lo=0x03
 *   movf  b+1,w   ; w=0x00 (C preserved)                   ; C=0
 *   subwfb a+1,w  ; w=0x00-0x00-1(borrow)=0xFF, C=0        ; C=0
 *   movwf r+1     ; r_hi=0xFF -> r=0xFF03 (0x0002-0xFFFF wraps to 0x0003)
 *   clrf bo; btfss STATUS,0 (C=0, don't skip); setf bo     ; bo=1
 * Matches host: 0x0002-0xFFFF = 0x0003 (mod 2^16), borrow=1.               */
static volatile uint16_t m_sub_a, m_sub_b, m_sub_r;
static volatile uint8_t  m_sub_bo;

uint16_t pic_math_sub_u16(uint16_t a, uint16_t b, bool *borrow_out)
{
    m_sub_a = a; m_sub_b = b; m_sub_bo = 0;
    asm("banksel _m_sub_a");
    asm("movf    _m_sub_b+0,w");       /* w = bLO                            */
    asm("subwf   _m_sub_a+0,w");       /* w = aLO - bLO, C=1 if no borrow    */
    asm("movwf   _m_sub_r+0");         /* rLO                                */
    asm("movf    _m_sub_b+1,w");       /* w = bHI (C preserved)              */
    asm("subwfb  _m_sub_a+1,w");       /* w = aHI - bHI - borrow, C=1 if ok  */
    asm("movwf   _m_sub_r+1");         /* rHI                                */
    asm("clrf    _m_sub_bo");
    asm("btfss   STATUS,0");           /* if C=0 (borrowed)                  */
    asm("setf    _m_sub_bo");
    if (borrow_out) *borrow_out = (bool)m_sub_bo;
    return m_sub_r;
}

/* ─── pic_math_negate_s16 ────────────────────────────────────────
 * -v = ~v + 1 (two's complement). The +1 is a 16-bit increment with carry:
 * inc the low byte; if it wrapped to 0 (Z set), inc the high byte. Label-free
 * cascade (btfsc skips incf when Z=0).
 * Hand-trace v=5 (0x0005) -> -5 (0xFFFB):
 *   comf v+0,w -> 0xFA; movwf r+0
 *   comf v+1,w -> 0xFF; movwf r+1        ; r=0xFFFA
 *   incf r+0,f   -> 0xFB, Z=0           ; no wrap, Z=0
 *   btfsc STATUS,2 (Z=0, skip) -> r+1 stays 0xFF
 *   -> r=0xFFFB = -5. Correct.
 * v=INT16_MIN (0x8000) -> ~0x7FFF +1 = 0x8000 (negate of INT16_MIN is itself):
 *   comf 0x00->0xFF, comf 0x80->0x7F ; r=0x7FFF
 *   incf r+0 0xFF->0x00, Z=1        ; wrapped
 *   btfsc STATUS,2 (Z=1, no skip) -> incf r+1 0x7F->0x80
 *   -> r=0x8000. Correct.                                            */
static volatile int16_t m_neg_v;
static volatile int16_t m_neg_r;

int16_t pic_math_negate_s16(int16_t v)
{
    m_neg_v = v;
    asm("banksel _m_neg_v");
    asm("comf  _m_neg_v+0,w");         /* w = ~vLO                            */
    asm("movwf _m_neg_r+0");
    asm("comf  _m_neg_v+1,w");         /* w = ~vHI                            */
    asm("movwf _m_neg_r+1");
    asm("incf  _m_neg_r+0,f");         /* rLO++ ; Z if wrapped to 0           */
    asm("btfsc STATUS,2");             /* if Z (rLO wrapped), inc rHI         */
    asm("incf  _m_neg_r+1,f");
    return m_neg_r;
}

/* ─── pic_math_negate_s32 ────────────────────────────────────────
 * Same ~v + 1, with the carry cascade extended across 4 bytes. Each btfsc
 * reads the Z from the preceding incf (or, if that incf was skipped, the Z
 * is still 0 from the earlier non-wrapping incf, so the cascade stops). */
static volatile int32_t m_neg32_v;
static volatile int32_t m_neg32_r;

int32_t pic_math_negate_s32(int32_t v)
{
    m_neg32_v = v;
    asm("banksel _m_neg32_v");
    asm("comf  _m_neg32_v+0,w");  asm("movwf _m_neg32_r+0");
    asm("comf  _m_neg32_v+1,w");  asm("movwf _m_neg32_r+1");
    asm("comf  _m_neg32_v+2,w");  asm("movwf _m_neg32_r+2");
    asm("comf  _m_neg32_v+3,w");  asm("movwf _m_neg32_r+3");
    asm("incf  _m_neg32_r+0,f");         /* byte0++ ; Z if wrapped          */
    asm("btfsc STATUS,2");               /* cascade carry while Z set       */
    asm("incf  _m_neg32_r+1,f");
    asm("btfsc STATUS,2");
    asm("incf  _m_neg32_r+2,f");
    asm("btfsc STATUS,2");
    asm("incf  _m_neg32_r+3,f");
    return m_neg32_r;
}
