/* epic-cc variant of the SFR mapping layer (paired with
 * target/pic16f628a_platform.h and host/pic16f628a_platform.h);
 * the build's include path picks which resolves, so
 * pic16f628a_hal.h includes "pic16f628a_platform.h" unconditionally
 * with no #ifdef. Banking is inserted by the compiler's banking pass,
 * so the XC8-specific inline-asm pie/bank fixups are plain C here;
 * placement pins are dropped (EPIC_PLACE expands to nothing) because
 * the overlay places globals itself. */

#ifndef PIC16F628A_PLATFORM_H
#define PIC16F628A_PLATFORM_H

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
 * banking pass inserts the required BANKSEL. No inline asm, and none of
 * the XC8 scratch bytes (the target header's epic_irq_pie_scratch /
 * epic_bank1_scratch pins exist only for its asm sequences). Bank 1
 * serves OPTION_REG/PCON/PIE1 on this die; there is no Bank 2/3 SFR. */
#define EPIC_BANK0_WRITE8(sfr_name, value) \
    do { EPIC_REG8(PIC_REG_##sfr_name) = (uint8_t)(value); } while (0)
#define EPIC_BANK0_READ8(sfr_name, out_var) \
    do { (out_var) = EPIC_REG8(PIC_REG_##sfr_name); } while (0)
#define EPIC_BANK1_WRITE8(sfr_name, value) \
    do { EPIC_REG8(PIC_REG_##sfr_name) = (uint8_t)(value); } while (0)
#define EPIC_BANK1_READ8(sfr_name, out_var) \
    do { (out_var) = EPIC_REG8(PIC_REG_##sfr_name); } while (0)

/* Single PIR/PIE pair on this part (no PIR2/PIE2): the enable/disable
 * macros take the PIE1 (0x8C) path unconditionally, plain RMW. */
#define EPIC_PIE_ENABLE_BIT(is_pir2, mask) \
    do { \
        (void)(is_pir2); \
        EPIC_REG8(0x8CU) |= (uint8_t)(mask); \
    } while (0)

#define EPIC_PIE_DISABLE_BIT(is_pir2, mask) \
    do { \
        (void)(is_pir2); \
        EPIC_REG8(0x8CU) &= (uint8_t)~(mask); \
    } while (0)

/* PIE1 reader for the dispatch's USART row: TMR1IE, TMR2IE, CCP1IE,
 * TXIE and EEIE (PIE1 bit 7 here, unlike the 87XA's PIE2) all sit in
 * PIE1. No SSP/ADIE reader: this die has neither SSP nor ADC. */
#define EPIC_PIE1_READ_TMR1IE(out_var) ((out_var) = EPIC_REG8(0x8CU))
#define EPIC_PIE1_READ_TMR2IE(out_var) ((out_var) = EPIC_REG8(0x8CU))
#define EPIC_PIE1_READ_CCP1IE(out_var) ((out_var) = EPIC_REG8(0x8CU))
#define EPIC_PIE1_READ_TXIE(out_var)   ((out_var) = EPIC_REG8(0x8CU))
#define EPIC_PIE1_READ_EEIE(out_var)   ((out_var) = EPIC_REG8(0x8CU))

#endif /* PIC16F628A_PLATFORM_H */
