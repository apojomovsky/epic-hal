/**
 * @file    target/pic16f87xa_platform.h
 * @brief   Real-target platform: how SFRs are accessed and how the weak
 *          attribute is spelled, for the XC8 build.
 *
 * @details
 *   This is the target half of the SFR mapping layer. The companion
 *   host/pic16f87xa_platform.h is used by the CMake host build. Which one
 *   is included is decided by the build's include path (the XC8 Makefile
 *   puts include/target first; CMake puts include/host first), so
 *   pic16f87xa.h includes "pic16f87xa_platform.h" unconditionally and
 *   there is no `#ifdef` around code anywhere in the HAL.
 *
 *   On a real PIC every SFR access is a direct volatile dereference of
 *   the literal address, exactly what the XC8 linker maps to the SFR.
 *   The address is cast through uintptr_t so XC8 does not warn about
 *   converting an integer to a pointer. XC8 has no weak symbols, so
 *   PIC8_WEAK is empty.
 */

#ifndef PIC16F87XA_PLATFORM_H
#define PIC16F87XA_PLATFORM_H

#include <stdint.h>

/* XC8 has no concept of weak symbols. */
#define PIC8_WEAK

/* SFR access resolves to a direct volatile dereference of the address. */
#define PIC8_SFR_PTR(addr)       ((volatile uint8_t *)(uintptr_t)(addr))
#define pic8_sfr_read8(addr)     (*(volatile uint8_t *)(uintptr_t)(addr))
#define pic8_sfr_write8(addr, v) \
    do { *(volatile uint8_t *)(uintptr_t)(addr) = (uint8_t)(v); } while (0)

/* Address of a register as a uint8_t lvalue (read/write/RMW). */
#define PIC8_REG8(addr)          (*(volatile uint8_t *)(uintptr_t)(addr))

/* PIE1/PIE2 (Bank 1) enable/disable, hand-written inline asm, not a
 * plain C read-modify-write through PIC8_REG8. Empirically probed under
 * MPLAB SIM (docs/ci-plan.md Phase 4): XC8 v4.00 places ordinary C
 * locals assuming Bank 0 and reaches them with plain direct addressing
 * regardless of the CPU's actual current bank, so a local read/written
 * while banked into Bank 1 (pic_select_bank(1) in effect) gets
 * misdirected. This macro instead loads the operand into W *before* the
 * bank switch and does the whole read-modify-write as one `iorwf`/
 * `andwf <SFR>,f` against the named SFR and W only, so nothing
 * Bank-0-assumed is ever touched while banked. This approach is
 * confirmed correct per the XC8 v4.00 User's Guide, not just
 * empirically: see pic16f87xa-hal/docs/ARCHITECTURE.md's Finding 1
 * (in-line asm resets the compiler's bank tracking, §5.12.2).
 *
 * Lives here (the target platform header), not in pic16_irq.c: that
 * file is shared with the host build (this repo's rule: no `#ifdef`
 * between host/target anywhere in the HAL, the split happens via
 * per-platform headers selected by include path), and `asm()`/`__at()`
 * are XC8-only syntax the host's gcc/clang cannot parse at all. The
 * host counterpart is host/pic16f87xa_platform.h's own definition of
 * these same two macro names, a plain array write, no banking concept
 * needed there at all.
 *
 * pic8_irq_pie_scratch (the file-scope symbol the asm needs; inline asm
 * can only address file-scope symbols, never function params or locals,
 * see pic8-math/docs/ARCHITECTURE.md's "Inline-asm binding" section)
 * is declared extern here and defined, `__at`-pinned into PIC16 mid-
 * range's bank-independent common RAM (0x70, DS39582B Figure 2-3), in
 * pic16_isr_vector.c (also target-only, so the pin doesn't need its own
 * new file). Pinned rather than left to default placement: the linker
 * scatters unpinned `static` storage by best-fit, not declaration order
 * (a documented, previously-hit gotcha, see AGENTS.md), and it picked a
 * Bank 1 address once, which defeats the whole point of loading this
 * into W before the bank switch (confirmed: a real "fixup overflow
 * referencing psect bssBANK1" link error before this was pinned). */
extern volatile uint8_t pic8_irq_pie_scratch __at(0x70);

/* Same root cause and same fix shape as PIC8_PIE_ENABLE_BIT above, for
 * plain (non-read-modify-write) Bank 1 SFR writes whose source value is
 * a function parameter or other C-level local: `HAL_TIMER2_WritePeriod`
 * (writing `period` into PR2) and `HAL_USART_Init` (writing `h->SPBRG`
 * into SPBRG) both landed the wrong byte in the register, traced via
 * `mdb` instruction-stepping to the exact point of divergence (the
 * parameter's own value was already correct right up until this write;
 * see pic16f87xa-hal/docs/ARCHITECTURE.md Finding 9). Same fix: load the
 * value into W through the bank-independent scratch byte *before*
 * switching banks, then a single `movwf <SFR>` while banked touches
 * nothing else. A separate scratch byte from PIE's own
 * (`pic8_bank1_scratch`, not `pic8_irq_pie_scratch`): unrelated
 * subsystems, no reason to couple them, and this repo has a full 16
 * bytes of common RAM (0x70-0x7F, DS39582B Figure 2-3) to work with. */
extern volatile uint8_t pic8_bank1_scratch __at(0x71);

#define PIC8_BANK1_WRITE8(sfr_name, value)                              \
    do {                                                                \
        pic8_bank1_scratch = (uint8_t)(value);                         \
        asm("movf _pic8_bank1_scratch,w");                             \
        asm("bsf STATUS,5");                                           \
        asm("movwf " #sfr_name);                                       \
        asm("bcf STATUS,5");                                           \
    } while (0)

/* Read side of the same fix: switch to Bank 1, read the SFR into W,
 * switch back to Bank 0 *before* touching any C-level storage (the
 * scratch byte itself is bank-independent common RAM either way, but
 * restoring the bank first keeps the sequence symmetric with the write
 * macro and avoids relying on that), then hand the value to the
 * caller's C-level `out_var` through the same scratch byte. Statement
 * macro with an output parameter (not an expression macro): simpler
 * and consistent with PIC8_BANK1_WRITE8's own shape. */
#define PIC8_BANK1_READ8(sfr_name, out_var)                             \
    do {                                                                \
        asm("bsf STATUS,5");                                           \
        asm("movf " #sfr_name ",w");                                   \
        asm("bcf STATUS,5");                                           \
        asm("movwf _pic8_bank1_scratch");                              \
        (out_var) = pic8_bank1_scratch;                                \
    } while (0)

/* Same fix, Banks 2 and 3 (`pic16f87xa_eeprom.c`'s EEDATA/EEADR/EECON1/
 * EECON2, confirmed corrupted the same way via a real-target `mdb`
 * probe, see pic16f87xa-hal/docs/ARCHITECTURE.md Finding 9's follow-up).
 * Unlike PIC8_BANK1_*, these set/clear *both* RP1:RP0 bits explicitly
 * rather than assuming RP1 already reads 0: EEPROM's own call sites
 * interleave Bank 2 and Bank 3 accesses back to back, so the incoming
 * bank state can't be assumed here the way it safely can for the
 * Bank-1-only PIE/Timer2/USART/ADC/VREF/COMP/PSP call sites. Both exit
 * to Bank 0 rather than restoring the caller's original bank (a
 * deliberate simplification, not a bug: every access in this codebase
 * explicitly selects the bank it needs before touching an SFR, none
 * rely on an inherited bank from a prior call, confirmed by this
 * session's own audit of every pic_select_bank call site). */
#define PIC8_BANK2_WRITE8(sfr_name, value)                              \
    do {                                                                \
        pic8_bank1_scratch = (uint8_t)(value);                         \
        asm("movf _pic8_bank1_scratch,w");                             \
        asm("bcf STATUS,5");                                           \
        asm("bsf STATUS,6");                                           \
        asm("movwf " #sfr_name);                                       \
        asm("bcf STATUS,5");                                           \
        asm("bcf STATUS,6");                                           \
    } while (0)

#define PIC8_BANK2_READ8(sfr_name, out_var)                             \
    do {                                                                \
        asm("bcf STATUS,5");                                           \
        asm("bsf STATUS,6");                                           \
        asm("movf " #sfr_name ",w");                                   \
        asm("bcf STATUS,5");                                           \
        asm("bcf STATUS,6");                                           \
        asm("movwf _pic8_bank1_scratch");                              \
        (out_var) = pic8_bank1_scratch;                                \
    } while (0)

#define PIC8_BANK3_WRITE8(sfr_name, value)                              \
    do {                                                                \
        pic8_bank1_scratch = (uint8_t)(value);                         \
        asm("movf _pic8_bank1_scratch,w");                             \
        asm("bsf STATUS,5");                                           \
        asm("bsf STATUS,6");                                           \
        asm("movwf " #sfr_name);                                       \
        asm("bcf STATUS,5");                                           \
        asm("bcf STATUS,6");                                           \
    } while (0)

#define PIC8_BANK3_READ8(sfr_name, out_var)                             \
    do {                                                                \
        asm("bsf STATUS,5");                                           \
        asm("bsf STATUS,6");                                           \
        asm("movf " #sfr_name ",w");                                   \
        asm("bcf STATUS,5");                                           \
        asm("bcf STATUS,6");                                           \
        asm("movwf _pic8_bank1_scratch");                              \
        (out_var) = pic8_bank1_scratch;                                \
    } while (0)

#define PIC8_PIE_ENABLE_BIT(is_pir2, mask)                              \
    do {                                                                \
        pic8_irq_pie_scratch = (uint8_t)(mask);                        \
        if (is_pir2) {                                                 \
            asm("movf _pic8_irq_pie_scratch,w");                       \
            asm("bsf STATUS,5");                                       \
            asm("iorwf PIE2,f");                                       \
            asm("bcf STATUS,5");                                       \
        } else {                                                       \
            asm("movf _pic8_irq_pie_scratch,w");                       \
            asm("bsf STATUS,5");                                       \
            asm("iorwf PIE1,f");                                       \
            asm("bcf STATUS,5");                                       \
        }                                                              \
    } while (0)

#define PIC8_PIE_DISABLE_BIT(is_pir2, mask)                             \
    do {                                                                \
        pic8_irq_pie_scratch = (uint8_t)~(mask);                       \
        if (is_pir2) {                                                 \
            asm("movf _pic8_irq_pie_scratch,w");                       \
            asm("bsf STATUS,5");                                       \
            asm("andwf PIE2,f");                                       \
            asm("bcf STATUS,5");                                       \
        } else {                                                       \
            asm("movf _pic8_irq_pie_scratch,w");                       \
            asm("bsf STATUS,5");                                       \
            asm("andwf PIE1,f");                                       \
            asm("bcf STATUS,5");                                       \
        }                                                              \
    } while (0)

#endif /* PIC16F87XA_PLATFORM_H */
