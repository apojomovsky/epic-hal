/* Host-simulation half of the SFR mapping layer (paired with
 * target/pic16f818_819_platform.h); the include path picks which
 * resolves, so pic16f818_819_hal.h includes this name unconditionally
 * with no #ifdef. SFR access indexes the 512-byte memory-backed
 * pic16f818_819_sim_sfr[] (src/sim/pic16f818_819_sim.c), so tests can
 * poke registers directly. The four bank windows land at 0x00, 0x80,
 * 0x100 and 0x180, so one 0x200-byte array holds the whole file. */

#ifndef PIC16F818_819_PLATFORM_H
#define PIC16F818_819_PLATFORM_H

#include <stdint.h>

/* 512-byte memory-backed register file (DS39598F Figures 2-3/2-4
 * layout), defined in src/sim/pic16f818_819_sim.c. */
extern uint8_t pic16f818_819_sim_sfr[0x200];

/* GCC/Clang weak attribute, lets user code override a peripheral's
 * IRQHandler if it ever needs to. */
#define EPIC_WEAK   __attribute__((weak))

/* Placement pins are an XC8 extension (__at); the host has no concept
 * of absolute GPR placement, so the pin is a no-op here. The target
 * header maps it to XC8's __at(addr). */
#define EPIC_PLACE(addr)

/* SFR access resolves to an index into the simulated register file. */
#define EPIC_SFR_PTR(addr)       (&pic16f818_819_sim_sfr[(uint16_t)(addr)])
#define epic_sfr_read8(addr)     (pic16f818_819_sim_sfr[(uint16_t)(addr)])
#define epic_sfr_write8(addr, v) \
    do { pic16f818_819_sim_sfr[(uint16_t)(addr)] = (uint8_t)(v); } while (0)

/* Address of a register as a uint8_t lvalue (read/write/RMW). */
#define EPIC_REG8(addr)          (pic16f818_819_sim_sfr[(uint16_t)(addr)])

/* The shared core passes OPTION_REG to the Bank-1 macros, whose host
 * forms index PIC_REG_##sfr_name; the family SFR map owns that alias
 * (pic16f818_819_sfr.h), so the name resolves here through it. */

/* Banked-literal accessors, host forms (plain array indexing). Bank 1
 * serves OPTION_REG/PR2/SSPADD/SSPSTAT/ADCON1/ADRESL/PCON, Bank 2 the
 * EEPROM data pair and Bank 3 the EEPROM control pair (nothing on this
 * die keeps an EEPROM register in Bank 0 or Bank 1). */
#define EPIC_BANK0_READ8(sfr_name, out_var) \
    do { (out_var) = pic16f818_819_sim_sfr[PIC_REG_##sfr_name]; } while (0)
#define EPIC_BANK0_WRITE8(sfr_name, value) \
    do { pic16f818_819_sim_sfr[PIC_REG_##sfr_name] = (uint8_t)(value); } while (0)
#define EPIC_BANK1_READ8(sfr_name, out_var) \
    do { (out_var) = pic16f818_819_sim_sfr[PIC_REG_##sfr_name]; } while (0)
#define EPIC_BANK1_WRITE8(sfr_name, value) \
    do { pic16f818_819_sim_sfr[PIC_REG_##sfr_name] = (uint8_t)(value); } while (0)
#define EPIC_BANK2_READ8(sfr_name, out_var) \
    do { (out_var) = pic16f818_819_sim_sfr[PIC_REG_##sfr_name]; } while (0)
#define EPIC_BANK2_WRITE8(sfr_name, value) \
    do { pic16f818_819_sim_sfr[PIC_REG_##sfr_name] = (uint8_t)(value); } while (0)
#define EPIC_BANK3_READ8(sfr_name, out_var) \
    do { (out_var) = pic16f818_819_sim_sfr[PIC_REG_##sfr_name]; } while (0)
#define EPIC_BANK3_WRITE8(sfr_name, value) \
    do { pic16f818_819_sim_sfr[PIC_REG_##sfr_name] = (uint8_t)(value); } while (0)

/* PIE1 (0x8C) / PIE2 (0x8D) enable/disable, direct read-modify-write:
 * the simulated register file is a plain array, so none of
 * target/pic16f818_819_platform.h's inline-asm banking path applies
 * here. */
#define EPIC_PIE_ENABLE_BIT(is_pir2, mask) \
    do { \
        if (is_pir2) { pic16f818_819_sim_sfr[0x8DU] |= (uint8_t)(mask); } \
        else         { pic16f818_819_sim_sfr[0x8CU] |= (uint8_t)(mask); } \
    } while (0)

#define EPIC_PIE_DISABLE_BIT(is_pir2, mask) \
    do { \
        if (is_pir2) { pic16f818_819_sim_sfr[0x8DU] &= (uint8_t)~(mask); } \
        else         { pic16f818_819_sim_sfr[0x8CU] &= (uint8_t)~(mask); } \
    } while (0)

/* PIE1/PIE2 enable-bit reads for the dispatch tiers. Host twins of the
 * target header's macros: the sim register file is a plain array, so
 * no banking path is needed. The dispatcher skips TIMER1_IRQHandler
 * when TMR1IE is off (a free-running Timer1 with the overflow
 * interrupt disabled latches TMR1IF at every wrap; see the target
 * header's comment for why that must not dispatch the handler). */
#define EPIC_PIE1_READ_TMR1IE(out_var) ((out_var) = pic16f818_819_sim_sfr[0x8CU])
#define EPIC_PIE1_READ_TMR2IE(out_var) ((out_var) = pic16f818_819_sim_sfr[0x8CU])
#define EPIC_PIE1_READ_CCP1IE(out_var) ((out_var) = pic16f818_819_sim_sfr[0x8CU])
#define EPIC_PIE1_READ_SSPIE(out_var)  ((out_var) = pic16f818_819_sim_sfr[0x8CU])
#define EPIC_PIE1_READ_ADIE(out_var)   ((out_var) = pic16f818_819_sim_sfr[0x8CU])
#define EPIC_PIE2_READ_EEIE(out_var)   ((out_var) = pic16f818_819_sim_sfr[0x8DU])

#endif /* PIC16F818_819_PLATFORM_H */
