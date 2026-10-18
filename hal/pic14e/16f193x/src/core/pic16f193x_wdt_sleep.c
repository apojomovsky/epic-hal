/**
 * BOR / POR status helpers, shared by both builds: they only read and
 * clear bits in PCON (DS41364B §3.0, Register 3-3) through the platform
 * SFR macro, so they stay here as one shared translation unit. The
 * build-mode-specific helpers EPIC_WDT_Refresh and EPIC_Sleep_Enter
 * live in pic16f193x_wdt_sleep_sim.c (host) and
 * pic16f193x_wdt_sleep_target.c (XC8), selected at link time.
 */

#include "core/pic16f193x_wdt_sleep.h"

/**
 * @brief  Returns 1 if the last reset was a Brown-out Reset (PCON<BOR>,
 *         DS41364B §3.0; active low: hardware clears it on the event).
 *         Re-arm after reading via @ref EPIC_BOR_ClearFlag.
 * @return 1 if the last reset was a Brown-out Reset, 0 otherwise.
 */
uint8_t EPIC_BOR_GetStatus(void)
{
    return (EPIC_REG8(PIC_REG_PCON) & PIC_PCON_BOR) ? 0U : 1U;
}

/**
 * @brief Re-arm the BOR flag by setting PCON<BOR>.
 */
void EPIC_BOR_ClearFlag(void)
{
    EPIC_BIT_SET(EPIC_REG8(PIC_REG_PCON), PIC_PCON_BOR);
}

/**
 * @brief  Returns 1 if the device just powered on (PCON<POR>,
 *         DS41364B §3.0; active low: hardware clears it on the event).
 * @return 1 if a Power-on Reset just occurred, 0 otherwise.
 */
uint8_t EPIC_POR_GetStatus(void)
{
    return (EPIC_REG8(PIC_REG_PCON) & PIC_PCON_POR) ? 0U : 1U;
}

/**
 * @brief Re-arm the POR flag by setting PCON<POR>.
 */
void EPIC_POR_ClearFlag(void)
{
    EPIC_BIT_SET(EPIC_REG8(PIC_REG_PCON), PIC_PCON_POR);
}
