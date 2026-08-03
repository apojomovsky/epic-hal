/**
 * @file    pic16f193x_wdt_sleep.c
 * @brief   BOR / POR status helpers, shared by both builds.
 *
 * @details
 *   The build-mode-specific helpers HAL_WDT_Refresh and HAL_Sleep_Enter
 *   live in pic16f193x_wdt_sleep_sim.c (host) and
 *   pic16f193x_wdt_sleep_target.c (XC8), selected at link time. The BOR/POR
 *   status helpers below are identical on both builds, they just read and
 *   clear bits in PCON (DS41364B §3.0, Register 3-3) through the platform
 *   SFR macro, so they stay here as one shared translation unit.
 */

#include "core/pic16f193x_wdt_sleep.h"

uint8_t HAL_BOR_GetStatus(void)
{
    return (PIC8_REG8(PIC_REG_PCON) & PIC_PCON_BOR) ? 1U : 0U;
}

void HAL_BOR_ClearFlag(void)
{
    PIC8_BIT_CLR(PIC8_REG8(PIC_REG_PCON), PIC_PCON_BOR);
}

uint8_t HAL_POR_GetStatus(void)
{
    return (PIC8_REG8(PIC_REG_PCON) & PIC_PCON_POR) ? 1U : 0U;
}

void HAL_POR_ClearFlag(void)
{
    PIC8_BIT_CLR(PIC8_REG8(PIC_REG_PCON), PIC_PCON_POR);
}
