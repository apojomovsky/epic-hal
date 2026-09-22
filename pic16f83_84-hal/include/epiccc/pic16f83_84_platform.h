/* epic-cc variant of the SFR mapping layer (paired with
 * target/pic16f83_84_platform.h and host/pic16f83_84_platform.h);
 * the build's include path picks which resolves, so
 * pic16f83_84_hal.h includes "pic16f83_84_platform.h" unconditionally
 * with no #ifdef. Banking is inserted by the compiler's banking pass,
 * so the XC8-specific inline-asm pie/bank fixups are plain C here;
 * placement pins are dropped (EPIC_PLACE expands to nothing) because
 * the overlay places globals itself. */

#ifndef PIC16F83_84_PLATFORM_H
#define PIC16F83_84_PLATFORM_H

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

/* Weak attribute: lets user code override a peripheral's IRQHandler. */
#define EPIC_WEAK   __attribute__((weak))

/* Placement pins are an XC8 bank-placement concern; the
 * whole-program overlay places globals (unique addresses, one
 * bank) itself, so a pin only fragments the layout (epic-hal#86). */
#define EPIC_PLACE(addr)

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

/* Legacy bare name OPTION_REG (an XC8 SFR symbol on the target path)
 * maps to its address here. */
#ifndef OPTION_REG
#define OPTION_REG PIC_REG_OPTION
#endif

/* Banked SFR access under epic-cc is a plain C access; the compiler's
 * banking pass inserts the required BANKSEL. No inline asm. Bank 1
 * serves OPTION_REG/TRISA/TRISB/EECON1/EEADR/EEDATA on this die; there
 * is no Bank 2/3 SFR. */
#define EPIC_BANK0_WRITE8(sfr_name, value) \
    do { EPIC_REG8(PIC_REG_##sfr_name) = (uint8_t)(value); } while (0)
#define EPIC_BANK0_READ8(sfr_name, out_var) \
    do { (out_var) = EPIC_REG8(PIC_REG_##sfr_name); } while (0)
#define EPIC_BANK1_WRITE8(sfr_name, value) \
    do { EPIC_REG8(PIC_REG_##sfr_name) = (uint8_t)(value); } while (0)
#define EPIC_BANK1_READ8(sfr_name, out_var) \
    do { (out_var) = EPIC_REG8(PIC_REG_##sfr_name); } while (0)

/* Single INTCON-level interrupt source on this part: no PIR1/PIR2 and
 * no PIE1/PIE2, so there are no PIE enable/disable macros and no PIE
 * readers. The dispatch's PIR-less path clears the EECON1-resident EEIF
 * (EECON1<4>, Bank 1) through EPIC_BANK1_* above; its 628A-style
 * INTCON.TMR0IF/RBIF rows read PIC_REG_INTCON directly. */

#endif /* PIC16F83_84_PLATFORM_H */
