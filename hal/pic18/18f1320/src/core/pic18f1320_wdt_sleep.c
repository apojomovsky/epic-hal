/*
 * BOR / POR status helpers, shared by both builds. `EPIC_WDT_Refresh` /
 * `EPIC_Sleep_Enter` are link-time-selected (`*_sim.c` host, `*_target.c`
 * XC8); these helpers are identical on both. RCON: POR = bit 1, BOR =
 * bit 0 (DS39605F Register 4-1). Both are active low: hardware clears
 * the bit on the event and software sets it back.
 */

#include "core/pic18f1320_wdt_sleep.h"

/**
 * @brief  Return 1 if the last reset was a Brown-out Reset (RCON<BOR>,
 *         active low). Re-arm after reading via @ref EPIC_BOR_ClearFlag.
 * @return 1 if the last reset was a Brown-out Reset, else 0.
 */
uint8_t EPIC_BOR_GetStatus(void)
{
    return (EPIC_REG8(PIC_REG_RCON) & PIC_RCON_BOR) ? 0U : 1U;
}

/**
 * @brief  Re-arm the BOR flag by setting RCON<BOR>.
 */
void EPIC_BOR_ClearFlag(void)
{
    EPIC_BIT_SET(EPIC_REG8(PIC_REG_RCON), PIC_RCON_BOR);
}

/**
 * @brief  Return 1 if the device just powered on (RCON<POR>, active
 *         low: cleared by hardware on a Power-on Reset, DS39605F
 *         Register 4-1).
 * @return 1 if the device just powered on, else 0.
 */
uint8_t EPIC_POR_GetStatus(void)
{
    return (EPIC_REG8(PIC_REG_RCON) & PIC_RCON_POR) ? 0U : 1U;
}

/**
 * @brief  Re-arm the POR flag by setting RCON<POR>.
 */
void EPIC_POR_ClearFlag(void)
{
    EPIC_BIT_SET(EPIC_REG8(PIC_REG_RCON), PIC_RCON_POR);
}
