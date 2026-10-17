/**
 * Host simulation platform: SFRs live in a memory array owned by the
 * simulator. Selected at build time by putting include/host first.
 * EPIC_REG8 is the raw array cell; epic_sfr_read8/write8 go through the
 * simulator's access hooks, which model the side effects of reading or
 * writing a few registers (RC1REG, TX1REG, SSP1BUF and the read-only
 * status bits). Drivers use the hooked pair only where a register has
 * such effects on silicon.
 */
#ifndef PIC16F188XX_PLATFORM_H
#define PIC16F188XX_PLATFORM_H

#include <stdint.h>

extern uint8_t pic16f188xx_sim_sfr[0x2000];

/**
 * @brief Read an SFR through the simulator, applying read side effects.
 * @param addr SFR address.
 * @return The register value.
 */
uint8_t pic16f188xx_sim_sfr_read8(uint16_t addr);

/**
 * @brief Write an SFR through the simulator, applying write side effects.
 * @param addr SFR address.
 * @param value Value written.
 */
void pic16f188xx_sim_sfr_write8(uint16_t addr, uint8_t value);

#define EPIC_WEAK __attribute__((weak))
#define EPIC_PLACE(addr)
#define EPIC_SFR_PTR(addr) (&pic16f188xx_sim_sfr[(uint16_t)(addr)])
#define EPIC_REG8(addr) (pic16f188xx_sim_sfr[(uint16_t)(addr)])
#define epic_sfr_read8(addr) pic16f188xx_sim_sfr_read8((uint16_t)(addr))
#define epic_sfr_write8(addr, value) \
    pic16f188xx_sim_sfr_write8((uint16_t)(addr), (uint8_t)(value))
/* PPSLOCK key sequences, each a 0x55 write, a 0xAA write and a bit write
 * (PPSLOCKED clear to unlock, set to lock). The simulator checks the order
 * in its write hook. */
#define epic_pps_unlock_sequence() \
    do { \
        pic16f188xx_sim_sfr_write8(PIC_REG_PPSLOCK, 0x55U); \
        pic16f188xx_sim_sfr_write8(PIC_REG_PPSLOCK, 0xAAU); \
        pic16f188xx_sim_sfr_write8(PIC_REG_PPSLOCK, 0x00U); \
    } while (0)
#define epic_pps_lock_sequence() \
    do { \
        pic16f188xx_sim_sfr_write8(PIC_REG_PPSLOCK, 0x55U); \
        pic16f188xx_sim_sfr_write8(PIC_REG_PPSLOCK, 0xAAU); \
        pic16f188xx_sim_sfr_write8(PIC_REG_PPSLOCK, 0x01U); \
    } while (0)

#endif
