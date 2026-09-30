/* PIC16F5x real-target implementation of EPIC_WDT_Refresh /
 * EPIC_Sleep_Enter: the native clrwdt / sleep instructions
 * (DS41213D sections 8.0, 7.0). Linked by the XC8 build; the host
 * twin is in src/sim/. */

#include "core/pic16f5x_wdt_sleep.h"

/**
 * @brief  Feed the watchdog timer with the native clrwdt instruction.
 */
void EPIC_WDT_Refresh(void)
{
    asm("clrwdt");
}

/**
 * @brief  Enter sleep with the native sleep instruction.
 */
void EPIC_Sleep_Enter(void)
{
    asm("sleep");
}
