/* epic-cc variant of the SFR mapping layer (paired with
 * target/pic16f87xa_platform.h and host/pic16f87xa_platform.h); the build's
 * include path picks which resolves, so pic16f87xa.h includes
 * "pic16f87xa_platform.h" unconditionally with no #ifdef.
 *
 * Under epic-cc the SFR layer is the same volatile dereference shape as
 * the XC8 target (and as epic-cc's own fixtures); banking is inserted by
 * the compiler's banking pass, so the XC8-specific inline-asm
 * pie/bank fixups are replaced by plain C. Placement uses EPIC_AT from
 * <epic-cc.h>. */

#ifndef PIC16F87XA_PLATFORM_H
#define PIC16F87XA_PLATFORM_H

#include <stdint.h>

/* Prefer the toolchain header when building with epic-cc; fall back to a
 * local definition for host-gcc smoke checks. */
#ifdef __has_include
#if __has_include(<epic-cc.h>)
#include <epic-cc.h>
#else
#define EPIC_AT(addr) __attribute__((section(".epicat." #addr)))
#define EPIC_CONFIG(spec) \
    static const char __epic_config[] __attribute__((used, section(".epiccfg." spec))) = spec
#ifndef EPIC_FOSC_HZ
#define EPIC_FOSC_HZ 0
#endif
#endif
#else
#include <epic-cc.h>
#endif

/* epic-cc has no weak symbols, same as XC8 target. */
#define EPIC_WEAK

/* Placement pins map to epic-cc's EPIC_AT. */
#define EPIC_PLACE(addr) EPIC_AT(addr)
/* Legacy bare name OPTION_REG (XC8 SFR symbol) maps to its address. The
 * peripherals pass OPTION_REG to EPIC_BANK1_*; token pasting then needs
 * PIC_REG_OPTION_REG to exist. */
#define PIC_REG_OPTION_REG PIC_REG_OPTION
/* Address of a register as a uint8_t lvalue (read/write/RMW). */
#define EPIC_REG8(addr)          (*(volatile uint8_t *)(uintptr_t)(addr))

/* Alias for the legacy bare name used in BANK macro call sites. */
#ifndef OPTION_REG
#define OPTION_REG PIC_REG_OPTION
#endif

/* Banked SFR access under epic-cc is a plain C access; the compiler's
 * banking pass inserts the required BANKSEL. No inline asm. */
#define EPIC_BANK1_WRITE8(sfr_name, value) \
    do { EPIC_REG8(PIC_REG_##sfr_name) = (uint8_t)(value); } while (0)

#define EPIC_BANK1_READ8(sfr_name, out_var) \
    do { (out_var) = EPIC_REG8(PIC_REG_##sfr_name); } while (0)

/* PIE1 (0x8C) / PIE2 (0x8D) enable/disable, plain RMW. */
#define EPIC_PIE_ENABLE_BIT(is_pir2, mask) \
    do { \
        if (is_pir2) { EPIC_REG8(0x8DU) |= (uint8_t)(mask); } \
        else         { EPIC_REG8(0x8CU) |= (uint8_t)(mask); } \
    } while (0)

#define EPIC_PIE_DISABLE_BIT(is_pir2, mask) \
    do { \
        if (is_pir2) { EPIC_REG8(0x8DU) &= (uint8_t)~(mask); } \
        else         { EPIC_REG8(0x8CU) &= (uint8_t)~(mask); } \
    } while (0)

/* TMR1IE / TXIE / EEIE reads, plain. */
#define EPIC_PIE1_READ_TMR1IE(out_var) ((out_var) = EPIC_REG8(0x8CU))
#define EPIC_PIE1_READ_TXIE(out_var)   ((out_var) = EPIC_REG8(0x8CU))
#define EPIC_PIE2_READ_EEIE(out_var)   ((out_var) = EPIC_REG8(0x8DU))

#endif /* PIC16F87XA_PLATFORM_H */
