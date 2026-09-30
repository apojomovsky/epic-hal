/* epic-cc variant of the SFR mapping layer (paired with target/ and
 * host/); the include path picks which resolves, so pic16f193x.h
 * includes this name unconditionally with no `#ifdef`.
 *
 * Banking is the compiler's banking pass's job, so the XC8 inline-asm
 * PIE/bank fixups are plain C here, and placement pins are dropped:
 * the whole-program overlay places globals, and a bank-straddling
 * object goes through the linear region, which __at() cannot express. */

#ifndef PIC16F193X_PLATFORM_H
#define PIC16F193X_PLATFORM_H

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

/* Placement pins are an XC8 bank-placement concern; the overlay places
 * globals itself (see above). */
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

/* PIE1/PIE2/PIE3 enable/disable, plain C RMW (bank 1: 0x91/0x92/0x93,
 * DS41364B Tables 2-4). The compiler's banking pass inserts the MOVLB,
 * which is the whole point of the XC8 inline-asm version existing: XC8
 * misdirects this same plain-C shape to the linear region, epic-cc
 * banks it correctly. Same pir_index spelling as the target header
 * (0 for PIE1, 1 for PIE2, 2 for PIE3). */
#define EPIC_PIE_ENABLE_BIT(pir_index, mask)                              \
    do {                                                                  \
        volatile uint8_t *pie =                                           \
            ((pir_index) == 0U) ? &EPIC_REG8(0x91U) :                     \
            ((pir_index) == 1U) ? &EPIC_REG8(0x92U) :                     \
                                  &EPIC_REG8(0x93U);                      \
        *pie |= (uint8_t)(mask);                                          \
    } while (0)

#define EPIC_PIE_DISABLE_BIT(pir_index, mask)                            \
    do {                                                                  \
        volatile uint8_t *pie =                                           \
            ((pir_index) == 0U) ? &EPIC_REG8(0x91U) :                     \
            ((pir_index) == 1U) ? &EPIC_REG8(0x92U) :                     \
                                  &EPIC_REG8(0x93U);                      \
        *pie &= (uint8_t)~(uint8_t)(mask);                                \
    } while (0)

/* Whole-PIE reads for the dispatch tiers: TMR1IE, TXIE (both PIE1 bit
 * 0/4) and EEIE (PIE2 bit 4), DS41364B Tables 2-4. Plain C, same reason
 * as the enable/disable macros above. */
#define EPIC_PIE1_READ_TMR1IE(out_var)  ((out_var) = EPIC_REG8(0x91U))
#define EPIC_PIE1_READ_TXIE(out_var)    ((out_var) = EPIC_REG8(0x91U))
#define EPIC_PIE2_READ_EEIE(out_var)    ((out_var) = EPIC_REG8(0x92U))

#endif /* PIC16F193X_PLATFORM_H */
