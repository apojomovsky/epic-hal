/**
 * Real-target implementation of EPIC_WDT_Refresh / EPIC_Sleep_Enter,
 * linked by the XC8 Makefile (the companion host implementation is
 * pic16f193x_wdt_sleep_sim.c; the build picks one, no `#ifdef`). On a
 * real PIC these are the native `clrwdt` / `sleep` instructions (both
 * present on the Enhanced Mid-range core, DS41364B §24.0 / §24.2). The
 * shared BOR/POR status helpers live in pic16f193x_wdt_sleep.c.
 */

#include "core/pic16f193x_wdt_sleep.h"

/**
 * @brief Refresh the Watchdog Timer: native `clrwdt` instruction.
 */
void EPIC_WDT_Refresh(void)
{
    asm("clrwdt");
}

/**
 * @brief Enter Sleep: native `sleep` instruction; the part halts until
 *        any enabled interrupt wakes it (DS41364B §24.2).
 */
void EPIC_Sleep_Enter(void)
{
    asm("sleep");
}
