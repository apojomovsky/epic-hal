/* Real-target half of the SFR mapping layer (paired with
 * host/pic16f83_84_platform.h); the include path picks which resolves,
 * so pic16f83_84_hal.h includes this name unconditionally with no
 * #ifdef. XC8 build: SFR access is a direct volatile deref of the
 * literal address; XC8 has no weak symbols, so EPIC_WEAK is empty.
 * No PIR/PIE pair on this family, so there is no PIE enable path at
 * all: every interrupt gate is an INTCON bit (plain RMW, Bank 0). */

#ifndef PIC16F83_84_PLATFORM_H
#define PIC16F83_84_PLATFORM_H
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
 * file-scope symbols); __at-pinned into this family's bank-independent
 * common RAM (0x40, mirrored at 0xC0 in Bank 1) in pic16_isr_vector.c,
 * not left to the linker's best-fit scatter. The bigger 14-bit parts
 * pin theirs at 0x70; this family's GPR map ends at 0x4F. The
 * literals here must match PIC14MIDRANGE_COMMON_RAM_BASE in
 * pic14_midrange.h (not included here: the include cycles back into
 * this header through pic16f83_84_hal.h). */
extern volatile uint8_t epic_irq_pie_scratch __at(0x40);
extern volatile uint8_t epic_bank1_scratch __at(0x41);


/* Same fix shape as the 628A family's PIE macros, for plain Bank 1 SFR
 * writes whose source is a C-level local or parameter (see README.md,
 * XC8 codegen gotchas): load into W through a bank-independent scratch
 * byte, then a single movwf while banked; reads mirror it. Serves
 * EECON1/EECON2 (the shared EEPROM driver) and OPTION_REG (the GPIO
 * pull-up path). */
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

/* Bank 0 pair (EEDATA/EEADR on this family): explicit exit to Bank 0
 * before the access, so a caller's open bank window cannot misdirect
 * the deref. Same scratch mechanism as Bank 1. */
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

#endif /* PIC16F83_84_PLATFORM_H */
