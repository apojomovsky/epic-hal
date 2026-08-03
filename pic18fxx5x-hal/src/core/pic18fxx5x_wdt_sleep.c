/**
 * @file    pic18fxx5x_wdt_sleep.c
 * @brief   BOR / POR status helpers, shared by both builds.
 *
 * @details
 *   `HAL_WDT_Refresh`/`HAL_Sleep_Enter` are link-time-selected
 *   (`*_sim.c` host, `*_target.c` XC8); these BOR/POR helpers are
 *   identical on both builds, so they stay shared. RCON: POR = bit 1,
 *   BOR = bit 0 (DS39632E Register 4-1).
 */

#include "core/pic18fxx5x_wdt_sleep.h"

uint8_t HAL_BOR_GetStatus(void)
{
    return (PIC8_REG8(PIC_REG_RCON) & PIC_RCON_BOR) ? 1U : 0U;
}

void HAL_BOR_ClearFlag(void)
{
    PIC8_BIT_CLR(PIC8_REG8(PIC_REG_RCON), PIC_RCON_BOR);
}

uint8_t HAL_POR_GetStatus(void)
{
    return (PIC8_REG8(PIC_REG_RCON) & PIC_RCON_POR) ? 1U : 0U;
}

void HAL_POR_ClearFlag(void)
{
    PIC8_BIT_CLR(PIC8_REG8(PIC_REG_RCON), PIC_RCON_POR);
}
