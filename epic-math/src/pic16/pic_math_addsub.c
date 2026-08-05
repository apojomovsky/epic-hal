/**
 * @file    pic_math_addsub.c (PIC16 inline-asm backend)
 * @brief   Add/sub/negate primitives. Mid-range PIC16 has no `addwfc`/
 *          `subwfb`, so carry propagation uses AN526's idiom: `btfsc
 *          STATUS,0` + `incf` to fold carry into the high byte's addend.
 *          STATUS bits by number (C=0, Z=2). Operands live in the shared
 *          scratch buffer (pic_math_scratch.h), one `banksel` per routine.
 */

#include <xc.h>
#include "pic_math.h"
#include "pic_math_scratch.h"

/* ─── pic_math_add_u16 ───────────────────────────────────────────
 * 16-bit add with the carry idiom. Offsets: a@0, b@2, r@4, co@6.
 * Hand-trace a=0xFFFF b=0x0002:
 *   movf b+0,w     ; w=0x02
 *   addwf a+0,w    ; w=0xFF+0x02=0x01, C=1
 *   movwf r+0      ; r_lo=0x01
 *   movf b+1,w     ; w=0x00 (movf keeps C)            ; C=1
 *   btfsc STATUS,0 ; C=1 -> don't skip incf
 *   incf b+1,w     ; w=0x01
 *   addwf a+1,w    ; w=0xFF+0x01=0x00, C=1            ; C=1
 *   movwf r+1      ; r_hi=0x00 -> r=0x0001
 *   clrf co; btfsc STATUS,0 (C=1); incf co ; co=1
 * Matches host: 0xFFFF+0x0002=0x10001 -> 0x0001, carry=1.                 */
uint16_t pic_math_add_u16(uint16_t a, uint16_t b, bool *carry_out)
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
    asm("incf  _pic16_mscratch+3,w");
    asm("addwf _pic16_mscratch+1,w");
    asm("movwf _pic16_mscratch+5");
    asm("clrf  _pic16_mscratch+6");
    asm("btfsc STATUS,0");
    asm("incf  _pic16_mscratch+6,f");
    if (carry_out) *carry_out = (bool)pic16_mscratch[6];
    return (uint16_t)pic16_mscratch[4] | ((uint16_t)pic16_mscratch[5] << 8);
}

/* ─── pic_math_sub_u16 ───────────────────────────────────────────
 * a - b, borrow_out = (a < b). Offsets: a@0, b@2, r@4, bo@6.
 * Hand-trace a=0x0002 b=0xFFFF:
 *   movf b+0,w    ; w=0xFF
 *   subwf a+0,w   ; w=0x02-0xFF=0x03, C=0 (borrow)        ; C=0
 *   movwf r+0     ; r_lo=0x03
 *   movf b+1,w    ; w=0x00 (C preserved)                  ; C=0
 *   btfss STATUS,0 ; C=0 -> don't skip incf
 *   incf b+1,w    ; w=0x01 (borrow into high)
 *   subwf a+1,w   ; w=0x00-0x01=0xFF, C=0                ; C=0
 *   movwf r+1     ; r_hi=0xFF -> r=0xFF03 (wraps to 0x0003)
 *   clrf bo; btfss STATUS,0 (C=0, no skip); incf bo ; bo=1
 * Matches host: 0x0002-0xFFFF=0x0003, borrow=1.                          */
uint16_t pic_math_sub_u16(uint16_t a, uint16_t b, bool *borrow_out)
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
    asm("incf  _pic16_mscratch+3,w");
    asm("subwf _pic16_mscratch+1,w");
    asm("movwf _pic16_mscratch+5");
    asm("clrf  _pic16_mscratch+6");
    asm("btfss STATUS,0");
    asm("incf  _pic16_mscratch+6,f");
    if (borrow_out) *borrow_out = (bool)pic16_mscratch[6];
    return (uint16_t)pic16_mscratch[4] | ((uint16_t)pic16_mscratch[5] << 8);
}

/* ─── pic_math_negate_s16 ────────────────────────────────────────
 * -v = ~v + 1. Offsets: v@0, r@2. inc low; if it wrapped (Z), inc high.
 * Hand-trace v=5 (0x0005) -> -5 (0xFFFB): comf v+0->0xFA, comf v+1->0xFF
 * (r=0xFFFA); incf r+0->0xFB, Z=0 -> r+1 stays 0xFF -> r=0xFFFB.
 * v=INT16_MIN (0x8000): comf->0x7FFF, inc low wraps Z=1 -> inc high
 * 0x7F->0x80 -> 0x8000 (negate of INT16_MIN is itself).               */
int16_t pic_math_negate_s16(int16_t v)
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

/* ─── pic_math_negate_s32 ────────────────────────────────────────
 * Same ~v + 1, carry cascade across 4 bytes. Offsets: v@0-3, r@4-7. */
int32_t pic_math_negate_s32(int32_t v)
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
