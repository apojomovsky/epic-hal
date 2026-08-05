/**
 * @file    host/pic18_platform.h
 * @brief   Host-simulation platform: how SFRs are stored and how the weak
 *          attribute is spelled, for the CMake host build.
 *
 * @details
 *   Host half of the SFR mapping layer (the target half is
 *   `target/pic18_platform.h`); the build's include path picks one, so
 *   `pic18fxx5x.h` includes `"pic18_platform.h"` unconditionally with no
 *   `#ifdef`. Every SFR access indexes a memory-backed register file
 *   `pic18_sim_sfr[]` (`src/sim/pic18_sim.c`), sized to the full 12-bit
 *   data-memory address space so tests can poke any register directly.
 */

#ifndef PIC18_PLATFORM_H
#define PIC18_PLATFORM_H

#include <stdint.h>

/* 4096-byte memory-backed register file (DS39632E Figure 5-5 data-memory
 * map footprint), defined in src/sim/pic18_sim.c. */
extern uint8_t pic18_sim_sfr[0x1000];

/* GCC/Clang weak attribute, lets user code override a peripheral's
 * IRQHandler if it ever needs to. */
#define PIC8_WEAK   __attribute__((weak))

/* SFR access resolves to an index into the simulated register file. */
#define PIC8_SFR_PTR(addr)       (&pic18_sim_sfr[(uint16_t)(addr)])
#define epic_sfr_read8(addr)     (pic18_sim_sfr[(uint16_t)(addr)])
#define epic_sfr_write8(addr, v) \
    do { pic18_sim_sfr[(uint16_t)(addr)] = (uint8_t)(v); } while (0)

/* Address of a register as a uint8_t lvalue (read/write/RMW). */
#define PIC8_REG8(addr)          (pic18_sim_sfr[(uint16_t)(addr)])

#endif /* PIC18_PLATFORM_H */
