/**
 * @file    target/pic16f87xa_platform.h
 * @brief   Real-target platform: how SFRs are accessed and how the weak
 *          attribute is spelled, for the XC8 build.
 *
 * @details
 *   Target half of the SFR mapping layer (paired with
 *   host/pic16f87xa_platform.h); the build's include path picks which
 *   resolves, so pic16f87xa.h includes this name unconditionally with no
 *   `#ifdef`. SFR access is a direct volatile dereference of the literal
 *   address; XC8 has no weak symbols, so PIC8_WEAK is empty.
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

/* PIE1/PIE2 (Bank 1) enable/disable via inline asm, not a plain C RMW:
 * while Bank 1 is selected, XC8 v4.00 can misdirect an ordinary C local
 * assumed to live in Bank 0 (pic16f87xa-hal/docs/ARCHITECTURE.md,
 * Finding 1). Loads the operand into W before the bank switch, then
 * does the whole RMW as one `iorwf`/`andwf` against W and the SFR only;
 * lives here (not pic16_irq.c) because inline asm is XC8-only syntax,
 * and that file is shared with the host build. */

/* File-scope symbol the asm above needs (inline asm can only address
 * file-scope symbols, see pic8-math/docs/ARCHITECTURE.md's "Inline-asm
 * binding"); `__at`-pinned into bank-independent common RAM (0x70) in
 * pic16_isr_vector.c rather than left to the linker's default
 * placement, which scatters unpinned statics by best-fit, not
 * declaration order (AGENTS.md). */
extern volatile uint8_t pic8_irq_pie_scratch __at(0x70);

/* Same fix shape as PIE1/PIE2 above, for plain Bank 1 SFR writes whose
 * source is a C-level local or parameter (ARCHITECTURE.md Finding 9):
 * load into W through a bank-independent scratch byte before switching
 * banks, then a single `movwf <SFR>` while banked touches nothing else
 * C-level. Separate scratch byte from PIE's own; unrelated subsystems. */
extern volatile uint8_t pic8_bank1_scratch __at(0x71);

#define PIC8_BANK1_WRITE8(sfr_name, value)                              \
    do {                                                                \
        pic8_bank1_scratch = (uint8_t)(value);                         \
        asm("movf _pic8_bank1_scratch,w");                             \
        asm("bsf STATUS,5");                                           \
        asm("movwf " #sfr_name);                                       \
        asm("bcf STATUS,5");                                           \
    } while (0)

/* Read side of the same fix: bank in, read the SFR into W, bank out,
 * then hand the value to the caller through the same scratch byte.
 * Statement macro with an output parameter, matching
 * PIC8_BANK1_WRITE8's own shape. */
#define PIC8_BANK1_READ8(sfr_name, out_var)                             \
    do {                                                                \
        asm("bsf STATUS,5");                                           \
        asm("movf " #sfr_name ",w");                                   \
        asm("bcf STATUS,5");                                           \
        asm("movwf _pic8_bank1_scratch");                              \
        (out_var) = pic8_bank1_scratch;                                \
    } while (0)

/* Same fix, Banks 2/3 (pic16f87xa_eeprom.c's EEDATA/EEADR/EECON1/
 * EECON2). Unlike PIC8_BANK1_*, these set/clear *both* RP1:RP0 bits
 * explicitly since EEPROM interleaves Bank 2 and Bank 3 back to back,
 * so the incoming bank can't be assumed. Both exit to Bank 0 rather
 * than the caller's original bank by design: every access in this
 * codebase selects its own bank before touching an SFR. */
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
