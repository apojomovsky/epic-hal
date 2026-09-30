/* Host-simulation half of the SFR mapping layer (paired with
 * target/pic16f83_84_platform.h); the include path picks which
 * resolves, so pic16f83_84_hal.h includes this name unconditionally
 * with no #ifdef. SFR access indexes the memory-backed
 * pic16f83_84_sim_sfr[] (src/sim/pic16f83_84_sim.c), so tests can poke
 * registers directly. */

#ifndef PIC16F83_84_PLATFORM_H
#define PIC16F83_84_PLATFORM_H

#include <stdint.h>

/* Memory-backed register file, defined in src/sim/pic16f83_84_sim.c
 * (0x200 covers the 2-bank register file with room to spare). */
extern uint8_t pic16f83_84_sim_sfr[0x200];

/* GCC/Clang weak attribute, lets user code override a peripheral's
 * IRQHandler if it ever needs to. */
#define EPIC_WEAK   __attribute__((weak))

/* Placement pins are an XC8 extension (__at); the host has no concept
 * of absolute GPR placement, so the pin is a no-op here. */
#define EPIC_PLACE(addr)

/* SFR access resolves to an index into the simulated register file. */
#define EPIC_SFR_PTR(addr)       (&pic16f83_84_sim_sfr[(uint16_t)(addr)])
#define epic_sfr_read8(addr)     (pic16f83_84_sim_sfr[(uint16_t)(addr)])
#define epic_sfr_write8(addr, v) \
    do { pic16f83_84_sim_sfr[(uint16_t)(addr)] = (uint8_t)(v); } while (0)

/* Address of a register as a uint8_t lvalue (read/write/RMW). */
#define EPIC_REG8(addr)          (pic16f83_84_sim_sfr[(uint16_t)(addr)])

/* Banked-literal accessors, host forms (plain array indexing). The
 * Bank-0 pair serves EEDATA/EEADR, the Bank-1 pair EECON1/EECON2 and
 * OPTION_REG; the shared core's PIR-less paths require both to exist. */
#define EPIC_BANK0_READ8(sfr_name, out_var) \
    do { (out_var) = pic16f83_84_sim_sfr[PIC_REG_##sfr_name]; } while (0)
#define EPIC_BANK0_WRITE8(sfr_name, value)  \
    do { pic16f83_84_sim_sfr[PIC_REG_##sfr_name] = (uint8_t)(value); } while (0)
#define EPIC_BANK1_READ8(sfr_name, out_var) \
    do { (out_var) = pic16f83_84_sim_sfr[PIC_REG_##sfr_name]; } while (0)
#define EPIC_BANK1_WRITE8(sfr_name, value)  \
    do { pic16f83_84_sim_sfr[PIC_REG_##sfr_name] = (uint8_t)(value); } while (0)

#endif /* PIC16F83_84_PLATFORM_H */
