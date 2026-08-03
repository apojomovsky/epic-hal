/**
 * @file    pic18fxx5x_wdt_sleep_target.c
 * @brief   Real-target implementation of HAL_WDT_Refresh / HAL_Sleep_Enter.
 *
 * @details
 *   Linked by the XC8 Makefile (the host counterpart is
 *   `pic18fxx5x_wdt_sleep_sim.c`). Native `clrwdt`/`sleep` instructions.
 */

#include "core/pic18fxx5x_wdt_sleep.h"

void HAL_WDT_Refresh(void)
{
    asm("clrwdt");
}

void HAL_Sleep_Enter(void)
{
    asm("sleep");
}
