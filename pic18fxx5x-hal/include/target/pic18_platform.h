/**
 * @file    target/pic18_platform.h
 * @brief   Real-target platform: how SFRs are accessed and how the weak
 *          attribute is spelled, for the XC8 build.
 *
 * @details
 *   Target half of the SFR mapping layer (the host half is
 *   `host/pic18_platform.h`); the build's include path picks one, so
 *   `pic18fxx5x.h` includes `"pic18_platform.h"` unconditionally with no
 *   `#ifdef`. Every SFR access is a direct volatile dereference of the
 *   literal address, cast through `uintptr_t` to silence XC8's
 *   integer-to-pointer warning. XC8 has no weak symbols, so `PIC8_WEAK`
 *   is empty.
 */

#ifndef PIC18_PLATFORM_H
#define PIC18_PLATFORM_H

#include <stdint.h>

/* XC8 has no concept of weak symbols. */
#define PIC8_WEAK

/* SFR access resolves to a direct volatile dereference of the address. */
#define PIC8_SFR_PTR(addr)       ((volatile uint8_t *)(uintptr_t)(addr))
#define epic_sfr_read8(addr)     (*(volatile uint8_t *)(uintptr_t)(addr))
#define epic_sfr_write8(addr, v) \
    do { *(volatile uint8_t *)(uintptr_t)(addr) = (uint8_t)(v); } while (0)

/* Address of a register as a uint8_t lvalue (read/write/RMW). */
#define PIC8_REG8(addr)          (*(volatile uint8_t *)(uintptr_t)(addr))

#endif /* PIC18_PLATFORM_H */
