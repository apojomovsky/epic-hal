/**
 * Host simulation platform: SFRs live in a memory array owned by the
 * simulator. Selected at build time by putting include/host first.
 */
#ifndef PIC16F188XX_PLATFORM_H
#define PIC16F188XX_PLATFORM_H

#include <stdint.h>

extern uint8_t pic16f188xx_sim_sfr[0x2000];

#define EPIC_WEAK __attribute__((weak))
#define EPIC_PLACE(addr)
#define EPIC_SFR_PTR(addr) (&pic16f188xx_sim_sfr[(uint16_t)(addr)])
#define EPIC_REG8(addr) (pic16f188xx_sim_sfr[(uint16_t)(addr)])
#define epic_sfr_read8(addr) (*EPIC_SFR_PTR(addr))
#define epic_sfr_write8(addr, value) do { *EPIC_SFR_PTR(addr) = (uint8_t)(value); } while (0)

#endif
