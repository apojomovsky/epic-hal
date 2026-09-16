/* epic-cc half of the SFR mapping layer (paired with target/ and
 * host/); the include path picks which resolves. The baseline asm pass
 * emits inline-asm templates verbatim and resolves only literal and
 * equ symbols (probed: `movf _scratch,w` panics the assembler), so
 * TRIS/OPTION load W from a scratch byte pinned to a literal address;
 * the XC8 target uses plain `__control` externs instead. */

#ifndef PIC16F5X_PLATFORM_H
#define PIC16F5X_PLATFORM_H

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

/* Weak attribute: lets user code override a peripheral handler (none
 * exist on this core; kept for contract parity). */
#define EPIC_WEAK   __attribute__((weak))

/* Placement pins are an XC8 bank-placement concern; the
 * whole-program overlay places globals itself. */
#define EPIC_PLACE(addr)

/* Bridge the historic FOSC_HZ name to the epic-cc spelling so shared
 * harness code sees the right frequency without its own #ifdef. */
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

/* Control-space TRIS/OPTION: loaded through a scratch byte pinned to a
 * literal file address (0x0C, in the 16F54's shared GPR window
 * 0x07-0x0F) because the baseline asm pass resolves only literal
 * operands (and EPIC_AT stringizes its argument, so the macro name
 * cannot be indirection: irparse rejects a non-literal section suffix).
 * `movf` loads W, `tris <f>` / `option` consume it. */
#define EPIC_SCRATCH_ADDR 0x0CU
static volatile uint8_t pic16f5x_scratch EPIC_AT(0x0C);

#define EPIC_TRIS_WRITE(sel, val)                                          \
    do {                                                                   \
        pic16f5x_scratch = (uint8_t)(val);                                 \
        asm volatile("movf 12, w");                                        \
        if ((sel) == 'A')                                                  \
        {                                                                  \
            asm volatile("tris 5");                                        \
        }                                                                  \
        else if ((sel) == 'B')                                             \
        {                                                                  \
            asm volatile("tris 6");                                        \
        }                                                                  \
        else                                                               \
        {                                                                  \
            _EPIC_TRIS_WRITE_OTHER((sel));                                 \
        }                                                                  \
    } while (0)

#if PIC16F5X_FAMILY_HAS_PORTC || PIC16F5X_FAMILY_HAS_PORTD \
    || PIC16F5X_FAMILY_HAS_PORTE
/**
 * @brief TRIS select for the wider ports: `tris <f>` takes the port's
 *        file address (7/8/9), kept a literal so the asm pass inlines.
 * @param sel the port select ('C', 'D' or 'E').
 */
static inline void _EPIC_TRIS_WRITE_OTHER(char sel)
{
#if PIC16F5X_FAMILY_HAS_PORTC
    if (sel == 'C')
    {
        asm volatile("tris 7");
        return;
    }
#endif
#if PIC16F5X_FAMILY_HAS_PORTD
    if (sel == 'D')
    {
        asm volatile("tris 8");
        return;
    }
#endif
#if PIC16F5X_FAMILY_HAS_PORTE
    if (sel == 'E')
    {
        asm volatile("tris 9");
        return;
    }
#endif
    (void)sel;
}
#else
#define _EPIC_TRIS_WRITE_OTHER(sel) do { (void)(sel); } while (0)
#endif

#define EPIC_OPTION_WRITE(val)                                             \
    do {                                                                   \
        pic16f5x_scratch = (uint8_t)(val);                                 \
        asm volatile("movf 12, w");                                        \
        asm volatile("option");                                            \
    } while (0)

#endif /* PIC16F5X_PLATFORM_H */
