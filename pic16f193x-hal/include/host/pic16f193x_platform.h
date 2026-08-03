/**
 * @file    host/pic16f193x_platform.h
 * @brief   Host-simulation platform: how SFRs are stored and how the weak
 *          attribute is spelled, for the CMake host build.
 *
 * @details
 *   Host half of the SFR mapping layer (paired with
 *   target/pic16f193x_platform.h for the XC8 build); the build's include
 *   path picks which one resolves, so pic16f193x.h includes this name
 *   unconditionally with no `#ifdef`. SFR access indexes the 4096-byte
 *   memory-backed pic16f193x_sim_sfr[] (src/sim/pic16f193x_sim.c), one
 *   byte per physical 12-bit data-memory address (DS41364B §2.2, up to
 *   32 banks x 128 bytes), so tests can poke registers directly.
 */

#ifndef PIC16F193X_PLATFORM_H
#define PIC16F193X_PLATFORM_H

#include <stdint.h>

/* 4096-byte memory-backed register file (one byte per 12-bit data-memory
 * address, DS41364B §2.2), defined in src/sim/pic16f193x_sim.c. */
extern uint8_t pic16f193x_sim_sfr[0x1000];

/* GCC/Clang weak attribute, lets user code override a peripheral's
 * IRQHandler if it ever needs to. */
#define PIC8_WEAK   __attribute__((weak))

/* SFR access resolves to an index into the simulated register file. */
#define PIC8_SFR_PTR(addr)       (&pic16f193x_sim_sfr[(uint16_t)(addr)])
#define pic8_sfr_read8(addr)     (pic16f193x_sim_sfr[(uint16_t)(addr)])
#define pic8_sfr_write8(addr, v) \
    do { pic16f193x_sim_sfr[(uint16_t)(addr)] = (uint8_t)(v); } while (0)

/* Address of a register as a uint8_t lvalue (read/write/RMW). */
#define PIC8_REG8(addr)          (pic16f193x_sim_sfr[(uint16_t)(addr)])

/* PIE1 (0x91) / PIE2 (0x92) / PIE3 (0x93) enable/disable, direct
 * read-modify-write: the simulated register file is a plain array, so
 * none of target/pic16f193x_platform.h's banking concerns apply here.
 * `pir_index` is 0 for PIE1, 1 for PIE2, 2 for PIE3 (DS41364B §4.5). */
#define PIC8_PIE_REG_ADDR(pir_index) \
    ((pir_index) == 0 ? 0x91U : ((pir_index) == 1 ? 0x92U : 0x93U))

#define PIC8_PIE_ENABLE_BIT(pir_index, mask) \
    do { pic16f193x_sim_sfr[PIC8_PIE_REG_ADDR(pir_index)] |= (uint8_t)(mask); } while (0)

#define PIC8_PIE_DISABLE_BIT(pir_index, mask) \
    do { pic16f193x_sim_sfr[PIC8_PIE_REG_ADDR(pir_index)] &= (uint8_t)~(mask); } while (0)

#endif /* PIC16F193X_PLATFORM_H */
