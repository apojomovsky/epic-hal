/* epic-cc variant of the SFR mapping layer (paired with
 * target/pic16f88x_platform.h and host/pic16f88x_platform.h); the build's
 * include path picks which resolves, so pic16f88x.h includes
 * "pic16f88x_platform.h" unconditionally with no #ifdef.
 *
 * Under epic-cc the SFR layer is the same volatile dereference shape as
 * the XC8 target; banking is inserted by the compiler's banking pass, so
 * the XC8-specific inline-asm pie/bank fixups are replaced by plain C.
 * Placement uses EPIC_AT from <epic-cc.h>. */

#ifndef PIC16F88X_PLATFORM_H
#define PIC16F88X_PLATFORM_H

#include <stdint.h>

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

#define EPIC_WEAK
/* Placement pins are an XC8 bank-placement concern; the
 * whole-program overlay places globals (unique addresses, one
 * bank) itself, so a pin only fragments the layout (epic-hal#86). */
#define EPIC_PLACE(addr)

#define EPIC_SFR_PTR(addr)       ((volatile uint8_t *)(uintptr_t)(addr))
#define epic_sfr_read8(addr)     (*(volatile uint8_t *)(uintptr_t)(addr))
#define epic_sfr_write8(addr, v) \
    do { *(volatile uint8_t *)(uintptr_t)(addr) = (uint8_t)(v); } while (0)

#define EPIC_REG8(addr)          (*(volatile uint8_t *)(uintptr_t)(addr))

/* Bare-name alias for the single legacy SFR that does not match its
 * PIC_REG_* spelling. Peripherals pass OPTION_REG to EPIC_BANK1_*,
 * which token-pastes to PIC_REG_OPTION_REG. */
#define PIC_REG_OPTION_REG PIC_REG_OPTION
#ifndef OPTION_REG
#define OPTION_REG PIC_REG_OPTION
#endif

/* Banked SFR access under epic-cc is plain C; banking is handled by the
 * compiler. No inline asm. */
#define EPIC_BANK1_WRITE8(sfr_name, value) \
    do { EPIC_REG8(PIC_REG_##sfr_name) = (uint8_t)(value); } while (0)
#define EPIC_BANK1_READ8(sfr_name, out_var) \
    do { (out_var) = EPIC_REG8(PIC_REG_##sfr_name); } while (0)
#define EPIC_BANK2_WRITE8(sfr_name, value) \
    do { EPIC_REG8(PIC_REG_##sfr_name) = (uint8_t)(value); } while (0)
#define EPIC_BANK2_READ8(sfr_name, out_var) \
    do { (out_var) = EPIC_REG8(PIC_REG_##sfr_name); } while (0)
#define EPIC_BANK3_WRITE8(sfr_name, value) \
    do { EPIC_REG8(PIC_REG_##sfr_name) = (uint8_t)(value); } while (0)
#define EPIC_BANK3_READ8(sfr_name, out_var) \
    do { (out_var) = EPIC_REG8(PIC_REG_##sfr_name); } while (0)

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

#define EPIC_PIE1_READ_TMR1IE(out_var) ((out_var) = EPIC_REG8(0x8CU))
#define EPIC_PIE1_READ_TXIE(out_var)   ((out_var) = EPIC_REG8(0x8CU))
#define EPIC_PIE2_READ_EEIE(out_var)   ((out_var) = EPIC_REG8(0x8DU))

#endif /* PIC16F88X_PLATFORM_H */
