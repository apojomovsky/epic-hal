/* BOR / POR status helpers and WDT software control, shared by both
 * builds. The build-specific EPIC_WDT_Refresh / EPIC_Sleep_Enter live
 * in the _sim / _target twins, selected at link time; the helpers here
 * only read and clear PCON bits and set WDTCON through the platform
 * SFR macro. */

#include "core/pic16f88x_wdt_sleep.h"

/* PCON lives in Bank 1 (0x8E); WDTCON in Bank 2 (0x105). Plain
 * EPIC_REG8 accesses silently misdirect to the Bank-0 alias under XC8
 * v4.00 (same class as the TXSTA/OPTION_REG sites in
 * tests/sim_bank_probe.c), so reads and RMWs go through the safe
 * banked macros where they exist. */

/**
 * @brief Return whether the last reset was a Brown-out Reset (PCON<BOR>).
 * @return 1 if BOR was the reset cause, 0 otherwise.
 */
uint8_t EPIC_BOR_GetStatus(void)
{
#ifdef EPIC_BANK1_READ8
    uint8_t pcon = 0u;
    EPIC_BANK1_READ8(PCON, pcon);
    return (pcon & PIC_PCON_BOR) ? 1U : 0U;
#else
    return (EPIC_REG8(PIC_REG_PCON) & PIC_PCON_BOR) ? 1U : 0U;
#endif
}

/**
 * @brief Clear PCON<BOR>.
 */
void EPIC_BOR_ClearFlag(void)
{
#ifdef EPIC_BANK1_READ8
    uint8_t pcon = 0u;
    EPIC_BANK1_READ8(PCON, pcon);
    pcon &= (uint8_t)~PIC_PCON_BOR;
    EPIC_BANK1_WRITE8(PCON, pcon);
#else
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PCON), PIC_PCON_BOR);
#endif
}

/**
 * @brief Return whether the device powered on via a Power-on Reset
 *        (PCON<POR>).
 * @return 1 if POR was the reset cause, 0 otherwise.
 */
uint8_t EPIC_POR_GetStatus(void)
{
#ifdef EPIC_BANK1_READ8
    uint8_t pcon = 0u;
    EPIC_BANK1_READ8(PCON, pcon);
    return (pcon & PIC_PCON_POR) ? 1U : 0U;
#else
    return (EPIC_REG8(PIC_REG_PCON) & PIC_PCON_POR) ? 1U : 0U;
#endif
}

/**
 * @brief Clear PCON<POR>.
 */
void EPIC_POR_ClearFlag(void)
{
#ifdef EPIC_BANK1_READ8
    uint8_t pcon = 0u;
    EPIC_BANK1_READ8(PCON, pcon);
    pcon &= (uint8_t)~PIC_PCON_POR;
    EPIC_BANK1_WRITE8(PCON, pcon);
#else
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PCON), PIC_PCON_POR);
#endif
}

/**
 * @brief Enable or disable the software Watchdog Timer (WDTCON<SWDTEN>).
 * @param enable 1 to turn the WDT on, 0 to turn it off.
 */
void EPIC_WDT_SetSoftwareEnable(uint8_t enable)
{
#ifdef EPIC_BANK2_READ8
    uint8_t wdtcon = 0u;
    EPIC_BANK2_READ8(WDTCON, wdtcon);
    if (enable) wdtcon |= PIC_WDTCON_SWDTEN;
    else        wdtcon &= (uint8_t)~PIC_WDTCON_SWDTEN;
    EPIC_BANK2_WRITE8(WDTCON, wdtcon);
#else
    uint8_t wdtcon = EPIC_REG8(PIC_REG_WDTCON);
    if (enable) wdtcon |= PIC_WDTCON_SWDTEN;
    else        wdtcon &= (uint8_t)~PIC_WDTCON_SWDTEN;
    EPIC_REG8(PIC_REG_WDTCON) = wdtcon;
#endif
}

/**
 * @brief Set the WDT period prescaler (WDTCON<WDTPS3:WDTPS0>).
 * @param wdtps the 4-bit WDTPS value (0..11; 12..15 reserved).
 */
void EPIC_WDT_SetPrescaler(uint8_t wdtps)
{
#ifdef EPIC_BANK2_READ8
    uint8_t wdtcon = 0u;
    EPIC_BANK2_READ8(WDTCON, wdtcon);
    wdtcon &= (uint8_t)~PIC_WDTCON_WDTPS_MASK;
    wdtcon |= (uint8_t)((wdtps << PIC_WDTCON_WDTPS_POS) & PIC_WDTCON_WDTPS_MASK);
    EPIC_BANK2_WRITE8(WDTCON, wdtcon);
#else
    uint8_t wdtcon = EPIC_REG8(PIC_REG_WDTCON);
    wdtcon &= (uint8_t)~PIC_WDTCON_WDTPS_MASK;
    wdtcon |= (uint8_t)((wdtps << PIC_WDTCON_WDTPS_POS) & PIC_WDTCON_WDTPS_MASK);
    EPIC_REG8(PIC_REG_WDTCON) = wdtcon;
#endif
}
