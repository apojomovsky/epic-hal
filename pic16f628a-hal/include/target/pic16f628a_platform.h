/* Real-target half of the SFR mapping layer (paired with
 * host/pic16f628a_platform.h); the include path picks which resolves,
 * so pic16f628a.h includes this name unconditionally with no #ifdef.
 * XC8 build: SFR access is a direct volatile deref of the literal
 * address; XC8 has no weak symbols, so EPIC_WEAK is empty.
 * Single PIR/PIE pair on this part (no PIR2/PIE2, no Bank-2/3 SFRs):
 * the PIE macros take the PIR1 path unconditionally. */

#ifndef PIC16F628A_PLATFORM_H
#define PIC16F628A_PLATFORM_H

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

/* PIE1 (Bank 1, 0x8C) enable/disable via inline asm, not a plain C RMW:
 * while a bank is selected, XC8 v4.00 can misdirect an ordinary C local
 * assumed to live in Bank 0 (see README.md, XC8 codegen gotchas). Loads
 * the operand into W before the bank switch, does the whole RMW as one
 * iorwf/andwf, selects Bank 1 absolutely and exits to Bank 0. */

/* File-scope symbol the asm needs (inline asm can only address
 * file-scope symbols); __at-pinned to bank-independent common RAM
 * (0x70) in pic16_isr_vector.c, not left to the linker's best-fit
 * scatter. */
extern volatile uint8_t epic_irq_pie_scratch __at(0x70);

/* Same fix shape as PIE1 above, for plain Bank 1 SFR writes whose
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

/* PIE1-only enable/disable (no PIR2/PIE2 on this part; the shared IRQ
 * core never sets pir_is_pir2 here, so the parameter is unused). */
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

/* Read the TMR1IE bit (PIE1 bit 0, Bank 1) through the same
 * bank-in/read/bank-out scratch mechanism as EPIC_BANK1_READ8. */
#define EPIC_PIE1_READ_TMR1IE(out_var) EPIC_BANK1_READ8(PIE1, (out_var))

/* Same shape, for the TXIE bit (PIE1 bit 4). */
#define EPIC_PIE1_READ_TXIE(out_var) EPIC_BANK1_READ8(PIE1, (out_var))

/* Same shape, for the EEIE bit (PIE1 bit 7 on this part; the 87XA keeps
 * EEIE in PIE2). The shared dispatcher's PIR1 EEIF block uses this. */
#define EPIC_PIE1_READ_EEIE(out_var) EPIC_BANK1_READ8(PIE1, (out_var))

#endif /* PIC16F628A_PLATFORM_H */
