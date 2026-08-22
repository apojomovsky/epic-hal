/*
 * epic-cc platform half of the SFR mapping layer (sibling to
 * host/pic18_platform.h and target/pic18_platform.h); the build's
 * include path picks one, so pic18fxx5x.h includes "pic18_platform.h"
 * unconditionally. SFR access stays a direct volatile dereference,
 * EPIC_PLACE maps onto epic-cc's EPIC_AT and EPIC_WEAK onto weak.
 */

#ifndef PIC18_PLATFORM_H
#define PIC18_PLATFORM_H

#include <stdint.h>
#include <epic-cc.h>

/* Weak attribute, lets user code override a peripheral's IRQHandler. */
#define EPIC_WEAK   __attribute__((weak))

/* Placement pins map onto epic-cc's spelling. */
#define EPIC_PLACE(addr)         EPIC_AT(addr)

/* Bridge FOSC_HZ to EPIC_FOSC_HZ for shared code. */
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

#endif /* PIC18_PLATFORM_H */
