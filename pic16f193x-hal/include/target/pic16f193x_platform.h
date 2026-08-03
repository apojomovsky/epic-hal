/**
 * @file    target/pic16f193x_platform.h
 * @brief   Real-target platform: how SFRs are accessed and how the weak
 *          attribute is spelled, for the XC8 build.
 *
 * @details
 *   Target half of the SFR mapping layer (paired with
 *   host/pic16f193x_platform.h); the build's include path picks which
 *   resolves, so pic16f193x.h includes this name unconditionally with no
 *   `#ifdef`. SFR access is a direct volatile dereference of the literal
 *   address; XC8 has no weak symbols, so PIC8_WEAK is empty.
 *
 *   On the Enhanced Mid-range core XC8 auto-banks every literal SFR
 *   access (it knows each SFR's bank from the DFP and emits the BSR
 *   select), so the PIE1/2/3 read-modify-writes below are plain C against
 *   `PIC_REG_*` tokens, no inline-asm bank switching. Whether this plain
 *   form is safe on this core is NOT assumed from the classic-PIC16
 *   result (where the same shape failed under XC8 v4.00,
 *   pic16f87xa-hal/docs/ARCHITECTURE.md Finding 1): it is verified by the
 *   §4 codegen probe (docs/adding-a-device.md) the moment the
 *   Microchip.PIC12-16F1xxx_DFP is available, and replaced with an
 *   inline-asm banking sequence if it misdirects. Until the probe runs,
 *   every SFR access stays a compile-time-constant token and runtime
 *   dispatch branches before touching any SFR, the same proven pattern
 *   used by pic18_irq.c / pic18fxx5x_ccp.c.
 */

#ifndef PIC16F193X_PLATFORM_H
#define PIC16F193X_PLATFORM_H

#include <stdint.h>

/* XC8 has no concept of weak symbols. */
#define PIC8_WEAK

/* SFR access resolves to a direct volatile dereference of the address. */
#define PIC8_SFR_PTR(addr)       ((volatile uint8_t *)(uintptr_t)(addr))
#define pic8_sfr_read8(addr)     (*(volatile uint8_t *)(uintptr_t)(addr))
#define pic8_sfr_write8(addr, v) \
    do { *(volatile uint8_t *)(uintptr_t)(addr) = (uint8_t)(v); } while (0)

/* Address of a register as a uint8_t lvalue (read/write/RMW). */
#define PIC8_REG8(addr)          (*(volatile uint8_t *)(uintptr_t)(addr))

/* PIE1/PIE2/PIE3 enable/disable. Plain C RMW against the literal PIE
 * register token; XC8 auto-banks the bank-1 access. See the file header
 * for the verification status of this form on the Enhanced Mid-range core.
 * `pir_index` is 0 for PIE1, 1 for PIE2, 2 for PIE3 (DS41364B §4.5). */
#define PIC8_PIE_REG_ADDR(pir_index) \
    ((pir_index) == 0 ? 0x91U : ((pir_index) == 1 ? 0x92U : 0x93U))

#define PIC8_PIE_ENABLE_BIT(pir_index, mask)                              \
    do {                                                                  \
        uint8_t _pa = PIC8_PIE_REG_ADDR(pir_index);                       \
        uint8_t _v = pic8_sfr_read8(_pa);                                 \
        _v |= (uint8_t)(mask);                                            \
        pic8_sfr_write8(_pa, _v);                                         \
    } while (0)

#define PIC8_PIE_DISABLE_BIT(pir_index, mask)                             \
    do {                                                                  \
        uint8_t _pa = PIC8_PIE_REG_ADDR(pir_index);                       \
        uint8_t _v = pic8_sfr_read8(_pa);                                 \
        _v &= (uint8_t)~(uint8_t)(mask);                                  \
        pic8_sfr_write8(_pa, _v);                                         \
    } while (0)

#endif /* PIC16F193X_PLATFORM_H */
