/*
 * BOR / POR status helpers, shared by both builds. `EPIC_WDT_Refresh` /
 * `EPIC_Sleep_Enter` are link-time-selected (`*_sim.c` host, `*_target.c`
 * XC8); these helpers are identical on both. RCON: POR = bit 1, BOR =
 * bit 0 (DS39632E Register 4-1).
 */

#include "core/pic18fxx5x_wdt_sleep.h"

uint8_t EPIC_BOR_GetStatus(void)
{
    return (EPIC_REG8(PIC_REG_RCON) & PIC_RCON_BOR) ? 1U : 0U;
}

void EPIC_BOR_ClearFlag(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_RCON), PIC_RCON_BOR);
}

uint8_t EPIC_POR_GetStatus(void)
{
    return (EPIC_REG8(PIC_REG_RCON) & PIC_RCON_POR) ? 1U : 0U;
}

void EPIC_POR_ClearFlag(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_RCON), PIC_RCON_POR);
}
