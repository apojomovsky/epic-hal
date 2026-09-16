/* Host-simulation half of the SFR mapping layer (paired with
 * target/pic16f5x_platform.h and epiccc/pic16f5x_platform.h); the
 * include path picks which resolves, so pic16f5x_hal.h includes this
 * name unconditionally with no #ifdef. SFR access indexes the
 * memory-backed pic16f5x_sim_sfr[] (src/sim/pic16f5x_sim.c), so tests
 * can poke registers directly. TRIS/OPTION route into the sim's
 * control shadow registers. */

#ifndef PIC16F5X_PLATFORM_H
#define PIC16F5X_PLATFORM_H

#include <stdint.h>

/* Host-build marker: the sim backend's control shadow registers
 * (below) exist only on this build; tests guard shadow reads with this
 * macro (the target controls are write-only, DS41213D Table 12-1). */
#define PIC16F5X_HOST 1

/* Memory-backed register file, defined in src/sim/pic16f5x_sim.c. */
extern uint8_t pic16f5x_sim_sfr[256];

/* GCC/Clang weak attribute, lets user code override a peripheral
 * handler (none exist on this core; kept for contract parity). */
#define EPIC_WEAK   __attribute__((weak))

/* Placement pins are an XC8 extension (__at); the host has no concept
 * of absolute GPR placement, so the pin is a no-op here. */
#define EPIC_PLACE(addr)

/* SFR access resolves to an index into the simulated register file. */
#define EPIC_SFR_PTR(addr)       (&pic16f5x_sim_sfr[(uint16_t)(addr)])
#define epic_sfr_read8(addr)     (pic16f5x_sim_sfr[(uint16_t)(addr)])
#define epic_sfr_write8(addr, v) \
    do { pic16f5x_sim_sfr[(uint16_t)(addr)] = (uint8_t)(v); } while (0)

/* Address of a register as a uint8_t lvalue (read/write/RMW). */
#define EPIC_REG8(addr)          (pic16f5x_sim_sfr[(uint16_t)(addr)])

/* Control-space TRIS/OPTION shadow registers, defined in
 * src/sim/pic16f5x_sim.c. */
extern uint8_t pic16f5x_sim_trisa;
extern uint8_t pic16f5x_sim_trisb;
#if PIC16F5X_FAMILY_HAS_PORTC
extern uint8_t pic16f5x_sim_trisc;
#endif
#if PIC16F5X_FAMILY_HAS_PORTD
extern uint8_t pic16f5x_sim_trisd;
#endif
#if PIC16F5X_FAMILY_HAS_PORTE
extern uint8_t pic16f5x_sim_trise;
#endif
extern uint8_t pic16f5x_sim_option;

#define EPIC_TRIS_WRITE(sel, val)                                           \
    do {                                                                    \
        if ((sel) == 'A')                                                   \
        {                                                                   \
            pic16f5x_sim_trisa = (uint8_t)(val);                            \
        }                                                                   \
        else if ((sel) == 'B')                                              \
        {                                                                   \
            pic16f5x_sim_trisb = (uint8_t)(val);                            \
        }                                                                   \
        else                                                                \
        {                                                                   \
            _EPIC_TRIS_WRITE_OTHER((sel), (uint8_t)(val));                  \
        }                                                                   \
    } while (0)

#if PIC16F5X_FAMILY_HAS_PORTC || PIC16F5X_FAMILY_HAS_PORTD \
    || PIC16F5X_FAMILY_HAS_PORTE
static inline void _EPIC_TRIS_WRITE_OTHER(char sel, uint8_t val)
{
#if PIC16F5X_FAMILY_HAS_PORTC
    if (sel == 'C')
    {
        pic16f5x_sim_trisc = val;
        return;
    }
#endif
#if PIC16F5X_FAMILY_HAS_PORTD
    if (sel == 'D')
    {
        pic16f5x_sim_trisd = val;
        return;
    }
#endif
#if PIC16F5X_FAMILY_HAS_PORTE
    if (sel == 'E')
    {
        pic16f5x_sim_trise = val;
        return;
    }
#endif
    /* Unknown select: write nothing. */
    (void)sel;
    (void)val;
}
#else
#define _EPIC_TRIS_WRITE_OTHER(sel, val) do { (void)(sel); (void)(val); } while (0)
#endif

#define EPIC_OPTION_WRITE(val) pic16f5x_sim_option = (uint8_t)(val)

#endif /* PIC16F5X_PLATFORM_H */
