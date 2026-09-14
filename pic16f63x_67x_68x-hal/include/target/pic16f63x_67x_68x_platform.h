/* Real-target half of the SFR mapping layer (paired with
 * host/pic16f63x_67x_68x_platform.h); the include path picks which
 * resolves, so pic16f63x_67x_68x_hal.h includes this name
 * unconditionally with no #ifdef. XC8 build: SFR access is a direct
 * volatile deref of the literal address; XC8 has no weak symbols, so
 * EPIC_WEAK is empty. */

#ifndef PIC16F63X_67X_68X_PLATFORM_H
#define PIC16F63X_67X_68X_PLATFORM_H

#include <stdint.h>

/* XC8 has no concept of weak symbols. */
#define EPIC_WEAK

/* Placement pins map to XC8's __at(addr) extension; the host header
 * defines EPIC_PLACE as a no-op. */
#define EPIC_PLACE(addr)         __at(addr)

/* SFR access resolves to a direct volatile dereference of the address. */
#define EPIC_SFR_PTR(addr)       ((volatile uint8_t *)(uintptr_t)(addr))
#define epic_sfr_read8(addr)     (*(volatile uint8_t *)(uintptr_t)(addr))
#define epic_sfr_write8(addr, v) \
    do { *(volatile uint8_t *)(uintptr_t)(addr) = (uint8_t)(v); } while (0)

/* Address of a register as a uint8_t lvalue (read/write/RMW). */
#define EPIC_REG8(addr)          (*(volatile uint8_t *)(uintptr_t)(addr))

/* PIE1/PIE2 (Bank 1: 0x8C/0x8D) enable/disable via inline asm, not a
 * plain C RMW: while a bank is selected, XC8 v4.00 can misdirect an
 * ordinary C local assumed to live in Bank 0 (see README.md, XC8
 * codegen gotchas). Loads the operand into W before the bank switch,
 * does the whole RMW as one iorwf/andwf, selects Bank 1 absolutely and
 * exits to Bank 0. Inline asm is XC8-only, so this lives here, not in
 * pic16_irq.c (shared with the host build). */

/* The banked-access macros below need bank-independent common RAM
 * for their scratch byte, selected by FAMILY_HAS_COMMON_RAM. Parts
 * without it (630/676, whose GPR is bank 0 only) have no banked
 * path: a plain literal hits the bank-0 alias (measured TRISA
 * stuck at POR on the 630 blink), so the #else arm fails loudly
 * instead of misdirecting silently. */
#if PIC16F63X_67X_68X_FAMILY_HAS_COMMON_RAM
/* File-scope symbol the asm needs (inline asm can only address
 * file-scope symbols, see epic-math/docs/ARCHITECTURE.md's "Inline-asm
 * binding"); __at-pinned to bank-independent common RAM
 * (PIC14MIDRANGE_COMMON_RAM_BASE, 0x70 here) in pic16_isr_vector.c,
 * not left to the linker's best-fit scatter. The literals here must
 * match PIC14MIDRANGE_COMMON_RAM_BASE in pic14_midrange.h. */
extern volatile uint8_t epic_irq_pie_scratch __at(0x70);

/* Same fix shape as PIE1/PIE2 above, for plain Bank 1 SFR writes whose
 * source is a C-level local or parameter (see README.md, XC8 codegen
 * gotchas): load into W through a bank-independent scratch byte, then a
 * single movwf while banked. Separate scratch from PIE's own. */
extern volatile uint8_t epic_bank1_scratch __at(0x71);

#define EPIC_BANK1_WRITE8(sfr_name, value)                              \
    do {                                                                \
        epic_bank1_scratch = (uint8_t)(value);                         \
        asm("movf _epic_bank1_scratch,w");                             \
        asm("bcf STATUS,6");                                           \
        asm("bsf STATUS,5");                                           \
        asm("movwf " #sfr_name);                                       \
        asm("bcf STATUS,5");                                           \
        asm("bcf STATUS,6");                                           \
    } while (0)

/* Read side of the same fix: bank in, read the SFR into W, bank out,
 * hand the value to the caller through the scratch byte. Both macros
 * select Bank 1 absolutely (clearing RP1 as well as setting RP0) and
 * exit to Bank 0: an incoming RP1=1 state would otherwise route the
 * access to Bank 3 GPR instead of the SFR. */
#define EPIC_BANK1_READ8(sfr_name, out_var)                             \
    do {                                                                \
        asm("bcf STATUS,6");                                           \
        asm("bsf STATUS,5");                                           \
        asm("movf " #sfr_name ",w");                                   \
        asm("bcf STATUS,5");                                           \
        asm("bcf STATUS,6");                                           \
        asm("movwf _epic_bank1_scratch");                              \
        (out_var) = epic_bank1_scratch;                                \
    } while (0)

/* PIE enable/disable with PIR2 support (PIE2 at 0x8D holds EEIE, C1IE,
 * C2IE, OSFIE on the 4-bank shapes). Same inline-asm shape as Bank 1.
 * The 2-bank shapes have no PIE2 SFR: their branch takes the PIE1-only
 * shape (the parameter is unused there, as on the 628A). */
#if PIC16F63X_67X_68X_FAMILY_HAS_PIR2
#define EPIC_PIE_ENABLE_BIT(is_pir2, mask)                              \
    do {                                                                \
        epic_irq_pie_scratch = (uint8_t)(mask);                        \
        if (is_pir2) {                                                 \
            asm("movf _epic_irq_pie_scratch,w");                       \
            asm("bcf STATUS,6");                                       \
            asm("bsf STATUS,5");                                       \
            asm("iorwf PIE2,f");                                       \
            asm("bcf STATUS,5");                                       \
            asm("bcf STATUS,6");                                       \
        } else {                                                       \
            asm("movf _epic_irq_pie_scratch,w");                       \
            asm("bcf STATUS,6");                                       \
            asm("bsf STATUS,5");                                       \
            asm("iorwf PIE1,f");                                       \
            asm("bcf STATUS,5");                                       \
            asm("bcf STATUS,6");                                       \
        }                                                              \
    } while (0)

#define EPIC_PIE_DISABLE_BIT(is_pir2, mask)                             \
    do {                                                                \
        epic_irq_pie_scratch = (uint8_t)~(mask);                       \
        if (is_pir2) {                                                 \
            asm("movf _epic_irq_pie_scratch,w");                       \
            asm("bcf STATUS,6");                                       \
            asm("bsf STATUS,5");                                       \
            asm("andwf PIE2,f");                                       \
            asm("bcf STATUS,5");                                       \
            asm("bcf STATUS,6");                                       \
        } else {                                                       \
            asm("movf _epic_irq_pie_scratch,w");                       \
            asm("bcf STATUS,6");                                       \
            asm("bsf STATUS,5");                                       \
            asm("andwf PIE1,f");                                       \
            asm("bcf STATUS,5");                                       \
            asm("bcf STATUS,6");                                       \
        }                                                              \
    } while (0)
#else
/* PIE1-only shapes (no PIE2 SFR): the parameter is unused. */
#define EPIC_PIE_ENABLE_BIT(is_pir2, mask)                              \
    do {                                                                \
        (void)(is_pir2);                                                \
        epic_irq_pie_scratch = (uint8_t)(mask);                        \
        asm("movf _epic_irq_pie_scratch,w");                           \
        asm("bcf STATUS,6");                                           \
        asm("bsf STATUS,5");                                           \
        asm("iorwf PIE1,f");                                           \
        asm("bcf STATUS,5");                                           \
        asm("bcf STATUS,6");                                           \
    } while (0)

#define EPIC_PIE_DISABLE_BIT(is_pir2, mask)                             \
    do {                                                                \
        (void)(is_pir2);                                                \
        epic_irq_pie_scratch = (uint8_t)~(mask);                       \
        asm("movf _epic_irq_pie_scratch,w");                           \
        asm("bcf STATUS,6");                                           \
        asm("bsf STATUS,5");                                           \
        asm("andwf PIE1,f");                                           \
        asm("bcf STATUS,5");                                           \
        asm("bcf STATUS,6");                                           \
    } while (0)
#endif

/* Read the TMR1IE bit (PIE1 bit 0, Bank 1) through the same
 * bank-in/read/bank-out scratch mechanism as EPIC_BANK1_READ8. The
 * dispatcher skips TIMER1_IRQHandler when TMR1IE is off: Timer1
 * free-runs with its overflow interrupt disabled, so TMR1IF latches at
 * every 65536-cycle wrap and would otherwise dispatch the handler on
 * every interrupt. */
#define EPIC_PIE1_READ_TMR1IE(out_var) EPIC_BANK1_READ8(PIE1, (out_var))

/* Same shape, for the EEIE bit (PIE2 bit 4, Bank 1). The dispatcher
 * skips EEPROM_IRQHandler when EEIE is off: EEPROM completion is often
 * polled with EEIE disabled, and an unconditional dispatch would clear
 * the polled flag from a live ISR. */
#define EPIC_PIE2_READ_EEIE(out_var) EPIC_BANK1_READ8(PIE2, (out_var))

/* Same shape, for the EEIE bit in PIE1 (2-bank shapes, EEPROM
 * completion in PIR1<7>). */
#define EPIC_PIE1_READ_EEIE(out_var) EPIC_BANK1_READ8(PIE1, (out_var))

/* Same fix, Bank 2. This family keeps WPUB/IOCB/VRCON/CM1CON0/CM2CON0/
 * CM2CON1/ANSEL and the EEPROM data/address pair in Bank 2. Sets RP1
 * and clears RP0 explicitly since the peripheral code interleaves
 * banks back to back, so the incoming bank can't be assumed. Exits to
 * Bank 0 by design: every access in this codebase selects its own bank
 * before touching an SFR. */
#define EPIC_BANK2_WRITE8(sfr_name, value)                              \
    do {                                                                \
        epic_bank1_scratch = (uint8_t)(value);                         \
        asm("movf _epic_bank1_scratch,w");                             \
        asm("bcf STATUS,5");                                           \
        asm("bsf STATUS,6");                                           \
        asm("movwf " #sfr_name);                                       \
        asm("bcf STATUS,5");                                           \
        asm("bcf STATUS,6");                                           \
    } while (0)

#define EPIC_BANK2_READ8(sfr_name, out_var)                             \
    do {                                                                \
        asm("bcf STATUS,5");                                           \
        asm("bsf STATUS,6");                                           \
        asm("movf " #sfr_name ",w");                                   \
        asm("bcf STATUS,5");                                           \
        asm("bcf STATUS,6");                                           \
        asm("movwf _epic_bank1_scratch");                              \
        (out_var) = epic_bank1_scratch;                                \
    } while (0)

/* Same fix, Bank 3 (EECON1/EECON2/SRCON). */
#define EPIC_BANK3_WRITE8(sfr_name, value)                              \
    do {                                                                \
        epic_bank1_scratch = (uint8_t)(value);                         \
        asm("movf _epic_bank1_scratch,w");                             \
        asm("bsf STATUS,5");                                           \
        asm("bsf STATUS,6");                                           \
        asm("movwf " #sfr_name);                                       \
        asm("bcf STATUS,5");                                           \
        asm("bcf STATUS,6");                                           \
    } while (0)

#define EPIC_BANK3_READ8(sfr_name, out_var)                             \
    do {                                                                \
        asm("bsf STATUS,5");                                           \
        asm("bsf STATUS,6");                                           \
        asm("movf " #sfr_name ",w");                                   \
        asm("bcf STATUS,5");                                           \
        asm("bcf STATUS,6");                                           \
        asm("movwf _epic_bank1_scratch");                              \
        (out_var) = epic_bank1_scratch;                                \
    } while (0)
#else
/* Parts without common RAM (16F630/16F676: GPR is bank 0 only) have
 * no banked-access path: a plain literal hits the bank-0 alias (XC8
 * encodes the access bank-implicitly, measured TRISA stuck at POR on
 * the 630 blink), and there is no scratch home for the asm path
 * above. Both parts are module-excluded (the 1058-word 676 blink
 * overflows the 1K-word flash with XC8 error 1347, measured; the
 * 942-word 630 blink leaves no headroom), so a target build for
 * them must fail loudly here instead of misdirecting silently.
 * Host builds never include this header. */
#error "63x/67x/68x target: banked SFR access needs common RAM (16F630/16F676 are module-excluded)"
#endif
#endif /* PIC16F63X_67X_68X_PLATFORM_H */
