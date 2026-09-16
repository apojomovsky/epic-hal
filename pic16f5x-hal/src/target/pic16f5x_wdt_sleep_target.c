/* PIC16F5x real-target implementation of EPIC_WDT_Refresh /
 * EPIC_Sleep_Enter: the native clrwdt / sleep instructions
 * (DS41213D §8.0, §7.0). Linked by the XC8 build; the host twin is in
 * src/sim/. */

#include "core/pic16f5x_wdt_sleep.h"

void EPIC_WDT_Refresh(void)
{
    asm("clrwdt");
}

void EPIC_Sleep_Enter(void)
{
    asm("sleep");
}
