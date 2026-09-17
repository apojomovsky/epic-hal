/* Real-target half of the SFR mapping layer (paired with
 * host/pic16f818_819_platform.h); the include path picks which
 * resolves, so pic16f818_819_hal.h includes this name unconditionally
 * with no #ifdef. XC8 build: SFR access is a direct volatile deref of
 * the literal address; XC8 has no weak symbols, so EPIC_WEAK is empty.
 * The die has four GPR bank windows and the EEPROM pair sits in Banks
 * 2/3 (DS39598F Figure 2-3), so every bank pair exists here. */

#ifndef PIC16F818_819_PLATFORM_H
#define PIC16F818_819_PLATFORM_H

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

/* File-scope symbols the asm needs (inline asm can only address
 * file-scope symbols, see epic-math/docs/ARCHITECTURE.md's "Inline-asm
 * binding"); __at-pinned to bank-independent common RAM (0x70, mirrored
 * into Banks 1/2/3, DS39598F Figure 2-3) in pic16_isr_vector.c, not
 * left to the linker's best-fit scatter. The literals must match
 * PIC14MIDRANGE_COMMON_RAM_BASE in pic14_midrange.h. */
extern volatile uint8_t epic_irq_pie_scratch __at(0x70);
extern volatile uint8_t epic_bank1_scratch __at(0x71);

/* Same fix shape as the PIE macros below, for plain Bank 0 SFR writes
 * whose source is a C-level local or parameter (see README.md, XC8
 * codegen gotchas): load into W through the bank-independent scratch
 * byte, then a single movwf with RP1:RP0 cleared. The access itself
 * leaves the part in Bank 0, so no exit sequence is needed. */
#define EPIC_BANK0_WRITE8(sfr_name, value)                              \
    do {                                                                \
        epic_bank1_scratch = (uint8_t)(value);                         \
        asm("movf _epic_bank1_scratch,w");                             \
        asm("bcf STATUS,6");                                           \
        asm("bcf STATUS,5");                                           \
        asm("movwf " #sfr_name);                                       \
    } while (0)

#define EPIC_BANK0_READ8(sfr_name, out_var)                             \
    do {                                                                \
        asm("bcf STATUS,6");                                           \
        asm("bcf STATUS,5");                                           \
        asm("movf " #sfr_name ",w");                                   \
        asm("movwf _epic_bank1_scratch");                              \
        (out_var) = epic_bank1_scratch;                                \
    } while (0)

/* Bank 1 pair: serves OPTION_REG (Timer0, GPIO pull-ups), PR2 (Timer2),
 * SSPADD/SSPSTAT, ADCON1/ADRESL and PCON. Same fix shape, with both
 * select bits set explicitly: an incoming RP1=1 state (observed after
 * the sim harness init, see tests/sim_bank_probe.c) would otherwise
 * route the access to Bank 3 GPR instead of the SFR. */
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

/* Bank 2 pair: the EEPROM data registers EEDATA/EEADR (0x10C/0x10D,
 * DS39598F Table 3-1). Both select bits are set explicitly since the
 * EEPROM driver interleaves Banks 2 and 3 back to back, so the
 * incoming bank cannot be assumed. Both exit to Bank 0 by design:
 * every access in this codebase selects its own bank first. */
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

/* Bank 3 pair: the EEPROM control registers EECON1/EECON2 (0x18C/0x18D,
 * DS39598F Register 3-1). */
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

/* PIE1/PIE2 (Bank 1: 0x8C/0x8D) enable/disable via inline asm, not a
 * plain C RMW: while a bank is selected, XC8 v4.00 can misdirect an
 * ordinary C local assumed to live in Bank 0 (see README.md, XC8
 * codegen gotchas). Bank 2's matching offset (0x10D) is EEADR, NOT
 * PIE2, so a Bank-2 select would OR the mask into EEADR and never arm
 * the PIR2 source. Inline asm is XC8-only, so this lives here, not in
 * pic14_irq.c (shared with the host build). */
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

/* Read the TMR1IE bit (PIE1 bit 0, Bank 1) through the same
 * bank-in/read/bank-out scratch mechanism as EPIC_BANK1_READ8. The
 * dispatcher skips TIMER1_IRQHandler when TMR1IE is off: Timer1
 * free-runs with its overflow interrupt disabled (epic-swuart needs
 * the counter but never the overflow), so TMR1IF latches at every
 * 65536-cycle wrap and would make every CCP event pay the full
 * handler cost before its own dispatch. */
#define EPIC_PIE1_READ_TMR1IE(out_var) EPIC_BANK1_READ8(PIE1, (out_var))

/* Same shape, for TMR2IE (PIE1 bit 1): the dispatcher gates
 * TIMER2_IRQHandler on the enable bit so a stale TMR2IF does not
 * dispatch with its source off. */
#define EPIC_PIE1_READ_TMR2IE(out_var) EPIC_BANK1_READ8(PIE1, (out_var))

/* Same shape, for CCP1IE (PIE1 bit 2). */
#define EPIC_PIE1_READ_CCP1IE(out_var) EPIC_BANK1_READ8(PIE1, (out_var))

/* Same shape, for SSPIE (PIE1 bit 3). */
#define EPIC_PIE1_READ_SSPIE(out_var) EPIC_BANK1_READ8(PIE1, (out_var))

/* Same shape, for ADIE (PIE1 bit 6): the USART pair this die lacks
 * occupies PIE1 bits 4 and 5, so there is no TXIE reader here. */
#define EPIC_PIE1_READ_ADIE(out_var) EPIC_BANK1_READ8(PIE1, (out_var))

/* Same shape, for the EEIE bit (PIE2 bit 4, Bank 1). The dispatcher
 * skips EEPROM_IRQHandler when EEIE is off: EEPROM completion is often
 * polled with EEIE disabled, and an unconditional dispatch would clear
 * the polled flag from a live ISR. */
#define EPIC_PIE2_READ_EEIE(out_var) EPIC_BANK1_READ8(PIE2, (out_var))

#endif /* PIC16F818_819_PLATFORM_H */
