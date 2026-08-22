/* epic-cc half of the SFR mapping layer (sibling to
 * host/pic16f87xa_platform.h and target/pic16f87xa_platform.h); the
 * build's include path picks which resolves, so pic16f87xa.h includes
 * this name unconditionally with no #ifdef. SFR access stays a direct
 * volatile dereference of the literal address (the same shape the
 * fixtures compile), EPIC_PLACE is mapped onto epic-cc's EPIC_AT and
 * EPIC_WEAK onto __attribute__((weak)). No banking inline asm here,
 * peripherals fall back to plain EPIC_REG8 and the compiler's banking
 * pass handles the rest. */

#ifndef PIC16F87XA_PLATFORM_H
#define PIC16F87XA_PLATFORM_H

#include <stdint.h>
#include <epic-cc.h>

/* Weak attribute: lets user code override a peripheral's IRQHandler. */
#define EPIC_WEAK   __attribute__((weak))

/* Placement pins map onto epic-cc's spelling. */
#define EPIC_PLACE(addr)         EPIC_AT(addr)

/* Bridge the historic FOSC_HZ name to the epic-cc spelling so shared
 * harness code (epic_harness_target.c) sees the right frequency without
 * its own #ifdef. The driver pre-defines EPIC_FOSC_HZ from the resolved
 * EPIC_CONFIG. */
#ifndef FOSC_HZ
#ifdef EPIC_FOSC_HZ
#define FOSC_HZ EPIC_FOSC_HZ
#endif
#endif

/* SFR access resolves to a direct volatile dereference of the address. */
#define EPIC_SFR_PTR(addr)       ((volatile uint8_t *)(uintptr_t)(addr))
#define epic_sfr_read8(addr)     (*(volatile uint8_t *)(uintptr_t)(addr))
#define epic_sfr_write8(addr, v) \
    do { *(volatile uint8_t *)(uintptr_t)(addr) = (uint8_t)(v); } while (0)

/* Address of a register as a uint8_t lvalue (read/write/RMW). */
#define EPIC_REG8(addr)          (*(volatile uint8_t *)(uintptr_t)(addr))

/* PIE1/PIE2 (Bank 1: 0x8C/0x8D) enable/disable. Plain volatile RMW; the
 * compiler's banking pass inserts BANKSELs, unlike the XC8 target which
 * needs inline asm. */
#define EPIC_PIE_ENABLE_BIT(is_pir2, mask) \
    do { \
        if (is_pir2) { EPIC_REG8(0x8D) |= (uint8_t)(mask); } \
        else         { EPIC_REG8(0x8C) |= (uint8_t)(mask); } \
    } while (0)

#define EPIC_PIE_DISABLE_BIT(is_pir2, mask) \
    do { \
        if (is_pir2) { EPIC_REG8(0x8D) &= (uint8_t)~(mask); } \
        else         { EPIC_REG8(0x8C) &= (uint8_t)~(mask); } \
    } while (0)

/* PIE reads for the dispatcher gate. No banking asm; plain volatile read. */
#define EPIC_PIE1_READ_TMR1IE(out_var) ((out_var) = EPIC_REG8(0x8C))
#define EPIC_PIE1_READ_TXIE(out_var)   ((out_var) = EPIC_REG8(0x8C))
#define EPIC_PIE2_READ_EEIE(out_var)   ((out_var) = EPIC_REG8(0x8D))

/* Banked SFR helpers: simple volatile accesses with literal addresses.
 * Peripherals use these only when EPIC_BANK1_* is defined, so defining
 * them selects the literal-SFR path (no runtime address). The SFR name
 * is pasted onto PIC_REG_*, with one alias for the OPTION_REG spelling. */
#define PIC_REG_OPTION_REG  PIC_REG_OPTION
#define EPIC_BANK1_WRITE8(sfr, val)  do { EPIC_REG8(PIC_REG_##sfr) = (uint8_t)(val); } while (0)
#define EPIC_BANK1_READ8(sfr, out)   do { (out) = EPIC_REG8(PIC_REG_##sfr); } while (0)
#define EPIC_BANK2_WRITE8(sfr, val)  do { EPIC_REG8(PIC_REG_##sfr) = (uint8_t)(val); } while (0)
#define EPIC_BANK2_READ8(sfr, out)   do { (out) = EPIC_REG8(PIC_REG_##sfr); } while (0)
#define EPIC_BANK3_WRITE8(sfr, val)  do { EPIC_REG8(PIC_REG_##sfr) = (uint8_t)(val); } while (0)
#define EPIC_BANK3_READ8(sfr, out)   do { (out) = EPIC_REG8(PIC_REG_##sfr); } while (0)

#endif /* PIC16F87XA_PLATFORM_H */
