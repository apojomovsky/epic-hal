/**
 * @file    pic16f87xa_wdt_sleep.c
 * @brief   BOR / POR status helpers, shared by both builds.
 *
 * @details
 *   The build-mode-specific helpers EPIC_WDT_Refresh and EPIC_Sleep_Enter
 *   live in pic16f87xa_wdt_sleep_sim.c (host) and
 *   pic16f87xa_wdt_sleep_target.c (XC8), selected at link time. The BOR/POR
 *   status helpers below are identical on both builds, they just read and
 *   clear bits in PCON through the platform SFR macro, so they stay here
 *   as one shared translation unit.
 */

#include "core/pic16f87xa_wdt_sleep.h"

/* PCON bits (DS39582B §14.10, Register 14-2). */
#define PIC_PCON_BOR   EPIC_BIT(0)
#define PIC_PCON_POR   EPIC_BIT(1)

/* PCON lives in Bank 1 (address 0x8E). Plain EPIC_REG8 accesses
 * silently misdirect to the Bank-0 alias (PIR1) under XC8 v4.00 (same
 * class as the TXSTA/OPTION_REG sites probed 2026-08-09 by
 * pic16f87xa-hal/tests/sim_bank_probe.c), so reads and read-modify-
 * writes go through the safe Bank-1 macros where they exist. */

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
