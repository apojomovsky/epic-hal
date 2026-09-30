/* Host-simulation half of the SFR mapping layer (paired with
 * target/pic16f628a_platform.h); the include path picks which resolves,
 * so pic16f628a.h includes this name unconditionally with no #ifdef.
 * SFR access indexes the 512-byte memory-backed pic16f628a_sim_sfr[]
 * (src/sim/pic16f628a_sim.c), so tests can poke registers directly. */

#ifndef PIC16F628A_PLATFORM_H
#define PIC16F628A_PLATFORM_H

#include <stdint.h>

/* 512-byte memory-backed register file, defined in
 * src/sim/pic16f628a_sim.c. */
extern uint8_t pic16f628a_sim_sfr[0x200];

/* GCC/Clang weak attribute, lets user code override a peripheral's
 * IRQHandler if it ever needs to. */
#define EPIC_WEAK   __attribute__((weak))

/* Placement pins are an XC8 extension (__at); the host has no concept
 * of absolute GPR placement, so the pin is a no-op here. */
#define EPIC_PLACE(addr)

/* SFR access resolves to an index into the simulated register file. */
#define EPIC_SFR_PTR(addr)       (&pic16f628a_sim_sfr[(uint16_t)(addr)])
#define epic_sfr_read8(addr)     (pic16f628a_sim_sfr[(uint16_t)(addr)])
#define epic_sfr_write8(addr, v) \
    do { pic16f628a_sim_sfr[(uint16_t)(addr)] = (uint8_t)(v); } while (0)

/* Address of a register as a uint8_t lvalue (read/write/RMW). */
#define EPIC_REG8(addr)          (pic16f628a_sim_sfr[(uint16_t)(addr)])

/* PIE1 enable/disable, direct read-modify-write: the simulated register
 * file is a plain array, so none of target/pic16f628a_platform.h's
 * inline-asm banking path applies here. Single PIR/PIE pair on this
 * part; the is_pir2 parameter is unused. */
#define EPIC_PIE_ENABLE_BIT(is_pir2, mask) \
    do { \
        (void)(is_pir2); \
        pic16f628a_sim_sfr[0x8CU] |= (uint8_t)(mask); \
    } while (0)

#define EPIC_PIE_DISABLE_BIT(is_pir2, mask) \
    do { \
        (void)(is_pir2); \
        pic16f628a_sim_sfr[0x8CU] &= (uint8_t)~(mask); \
    } while (0)

/* Host twin of the target header's EPIC_PIE1_READ_TMR1IE. */
#define EPIC_PIE1_READ_TMR1IE(out_var) ((out_var) = pic16f628a_sim_sfr[0x8CU])

/* Host twin of the target header's EPIC_PIE1_READ_TXIE. */
#define EPIC_PIE1_READ_TXIE(out_var)   ((out_var) = pic16f628a_sim_sfr[0x8CU])

/* Host twin of the target header's EPIC_PIE1_READ_EEIE (PIE1 bit 7). */
#define EPIC_PIE1_READ_EEIE(out_var)   ((out_var) = pic16f628a_sim_sfr[0x8CU])

#endif /* PIC16F628A_PLATFORM_H */
