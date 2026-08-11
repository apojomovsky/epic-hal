/*
 * Real-target implementation of EPIC_WDT_Refresh / EPIC_Sleep_Enter
 * (linked by the XC8 Makefile; the host counterpart is
 * `pic18fxx5x_wdt_sleep_sim.c`). Native `clrwdt`/`sleep` instructions.
 */

#include "core/pic18fxx5x_wdt_sleep.h"

/**
 * @brief  Refresh the Watchdog Timer (real-target build) by executing the
 *         `clrwdt` instruction. MUST be called more often than the WDT
 *         period (DS39632E §14.x).
 */
void EPIC_WDT_Refresh(void)
{
    asm("clrwdt");
}

/**
 * @brief  Enter Power-down (Sleep) mode (real-target build) via the
 *         `sleep` instruction; the CPU halts until any enabled interrupt
 *         wakes it (DS39632E §3.0).
 */
void EPIC_Sleep_Enter(void)
{
    asm("sleep");
}
