/* Real-target half of the SFR mapping layer (paired with
 * host/pic16f5x_platform.h and epiccc/pic16f5x_platform.h); the include
 * path picks which resolves, so pic16f5x_hal.h includes this name
 * unconditionally with no #ifdef. XC8 build: SFR access is a direct
 * volatile deref of the literal address; XC8 has no weak symbols, so
 * EPIC_WEAK is empty. TRIS/OPTION are control-space instructions on
 * this core: XC8 lowers a write through the `__control` externs below
 * to the native `tris <f>` / `option` instruction (probed: `TRISA = v`
 * -> `movf v,w; tris 5`), so no inline asm is needed here. */

#ifndef PIC16F5X_PLATFORM_H
#define PIC16F5X_PLATFORM_H
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

/* Control-space SFRs (TRISA/TRISB/OPTION): on the 12-bit core these
 * are written with dedicated instructions, there is no file-register
 * address for them (DS41213D §12.0, Table 12-1). XC8 declares them
 * `extern volatile __control` and lowers a C assignment to the native
 * instruction; the `__at` values are the register select operands
 * (TRIS takes the port's file address; OPTION uses 0). Names are
 * family-internal so they never collide with xc.h's own SFR macros. */
extern volatile __control unsigned char pic16f5x_trisa __at(0x005);
extern volatile __control unsigned char pic16f5x_trisb __at(0x006);
extern volatile __control unsigned char pic16f5x_option __at(0x000);

/**
 * @brief Write a port's TRIS control register.
 * @param sel the port select ('A' or 'B'); only A/B exist on this die.
 * @param val the TRIS byte (1 = input bit, 0 = output bit).
 */
#define EPIC_TRIS_WRITE(sel, val)                                          \
    do {                                                                   \
        if ((sel) == 'A')                                                  \
        {                                                                  \
            pic16f5x_trisa = (uint8_t)(val);                               \
        }                                                                  \
        else                                                               \
        {                                                                  \
            pic16f5x_trisb = (uint8_t)(val);                               \
        }                                                                  \
    } while (0)

/**
 * @brief Write the OPTION control register.
 * @param val the OPTION byte (PS<2:0>, PSA, T0SE, T0CS).
 */
#define EPIC_OPTION_WRITE(val) pic16f5x_option = (uint8_t)(val)

#endif /* PIC16F5X_PLATFORM_H */
