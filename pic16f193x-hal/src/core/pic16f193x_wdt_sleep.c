/**
 * @file    pic16f193x_wdt_sleep.c
 * @brief   BOR / POR status helpers, shared by both builds.
 *
 * @details
 *   The build-mode-specific helpers EPIC_WDT_Refresh and EPIC_Sleep_Enter
 *   live in pic16f193x_wdt_sleep_sim.c (host) and
 *   pic16f193x_wdt_sleep_target.c (XC8), selected at link time. The BOR/POR
 *   status helpers below are identical on both builds, they just read and
 *   clear bits in PCON (DS41364B §3.0, Register 3-3) through the platform
 *   SFR macro, so they stay here as one shared translation unit.
 */

#include "core/pic16f193x_wdt_sleep.h"

uint8_t EPIC_BOR_GetStatus(void)
{
    return (EPIC_REG8(PIC_REG_PCON) & PIC_PCON_BOR) ? 1U : 0U;
}

void EPIC_BOR_ClearFlag(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PCON), PIC_PCON_BOR);
}

uint8_t EPIC_POR_GetStatus(void)
{
    return (EPIC_REG8(PIC_REG_PCON) & PIC_PCON_POR) ? 1U : 0U;
}

void EPIC_POR_ClearFlag(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PCON), PIC_PCON_POR);
}
