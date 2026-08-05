/**
 * @file    pic16f87xa_wdt_sleep_target.c
 * @brief   Real-target implementation of EPIC_WDT_Refresh / EPIC_Sleep_Enter.
 *
 * @details
 *   Linked by the XC8 Makefile. The companion host implementation is
 *   pic16f87xa_wdt_sleep_sim.c; the build picks one, so neither this file
 *   nor the examples need `#ifdef`. On a real PIC these are the native
 *   `clrwdt` / `sleep` instructions. The shared BOR/POR status helpers
 *   live in pic16f87xa_wdt_sleep.c.
 */

#include "core/pic16f87xa_wdt_sleep.h"

void EPIC_WDT_Refresh(void)
{
    asm("clrwdt");
}

void EPIC_Sleep_Enter(void)
{
    asm("sleep");
}
