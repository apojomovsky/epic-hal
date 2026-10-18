/**
 * WDT control, Idle entry and BOR/POR status, shared by both builds: they
 * only touch WDTCON0, CPUDOZE and PCON0 through the platform SFR macros
 * (WDTCON0 writes go through epic_sfr_write8 for the simulator's pinned
 * period).
 * EPIC_WDT_Refresh and EPIC_Sleep_Enter live in pic16f188xx_wdt_sleep_sim.c
 * (host) and pic16f188xx_wdt_sleep_target.c (XC8), selected at link time.
 */
#include "core/pic16f188xx_wdt_sleep.h"

#define WDT_PERIOD_MAX ((unsigned int)WDT_PERIOD_256S)

/**
 * @brief Program the WDT period and set SWDTEN.
 * @details One WDTCON0 write sets both fields. The readback tells whether
 * the period took: with CONFIG3 WDTCPS fixing it the WDTPS bits ignore
 * the write.
 * @param period Period to program.
 * @return EPIC_OK when WDTPS reads back as programmed, EPIC_INVALID for an
 *         out-of-range period, EPIC_ERROR when the period is fixed.
 */
EPIC_StatusTypeDef EPIC_WDT_Enable(WDT_PeriodTypeDef period)
{
    uint8_t back;

    if ((unsigned int)period > WDT_PERIOD_MAX)
    {
        return EPIC_INVALID;
    }
    epic_sfr_write8(PIC_REG_WDTCON0,
        (uint8_t)(((uint8_t)period << PIC_WDTCON0_WDTPS_SHIFT) | PIC_WDTCON0_SWDTEN));
    back = (uint8_t)((EPIC_REG8(PIC_REG_WDTCON0) & PIC_WDTCON0_WDTPS_MASK) >>
        PIC_WDTCON0_WDTPS_SHIFT);
    return (back == (uint8_t)period) ? EPIC_OK : EPIC_ERROR;
}

/**
 * @brief Clear WDTCON0<SWDTEN>, keeping the period bits.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_WDT_Disable(void)
{
    epic_sfr_write8(PIC_REG_WDTCON0,
        (uint8_t)(EPIC_REG8(PIC_REG_WDTCON0) & (uint8_t)~PIC_WDTCON0_SWDTEN));
    return EPIC_OK;
}

/**
 * @brief Enter Idle: IDLEN set around one EPIC_Sleep_Enter.
 * @details IDLEN is restored to its value on entry, so a later
 * EPIC_Sleep_Enter is still a full Sleep unless the caller set IDLEN.
 */
void EPIC_Idle_Enter(void)
{
    uint8_t was_idle = (EPIC_REG8(PIC_REG_CPUDOZE) & PIC_CPUDOZE_IDLEN) ? 1U : 0U;

    EPIC_BIT_SET(EPIC_REG8(PIC_REG_CPUDOZE), PIC_CPUDOZE_IDLEN);
    EPIC_Sleep_Enter();
    if (was_idle == 0U)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_CPUDOZE), PIC_CPUDOZE_IDLEN);
    }
}

/**
 * @brief Report a Brown-out Reset: PCON0<BOR> is active low.
 * @return 1 when BOR reads clear, 0 otherwise.
 */
uint8_t EPIC_BOR_GetStatus(void)
{
    return (EPIC_REG8(PIC_REG_PCON0) & PIC_PCON0_nBOR) ? 0U : 1U;
}

/**
 * @brief Set PCON0<BOR>, re-arming the flag.
 */
void EPIC_BOR_ClearFlag(void)
{
    EPIC_BIT_SET(EPIC_REG8(PIC_REG_PCON0), PIC_PCON0_nBOR);
}

/**
 * @brief Report a Power-on Reset: PCON0<POR> is active low.
 * @return 1 when POR reads clear, 0 otherwise.
 */
uint8_t EPIC_POR_GetStatus(void)
{
    return (EPIC_REG8(PIC_REG_PCON0) & PIC_PCON0_nPOR) ? 0U : 1U;
}

/**
 * @brief Set PCON0<POR>, re-arming the flag.
 */
void EPIC_POR_ClearFlag(void)
{
    EPIC_BIT_SET(EPIC_REG8(PIC_REG_PCON0), PIC_PCON0_nPOR);
}
