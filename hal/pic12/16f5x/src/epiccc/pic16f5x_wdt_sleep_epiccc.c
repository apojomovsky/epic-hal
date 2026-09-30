/*
 * PIC16F5x epic-cc implementation of EPIC_WDT_Refresh /
 * EPIC_Sleep_Enter, using the intrinsics from <epic-cc.h> (which lower
 * to asm blocks the baseline backend understands). The XC8 target uses
 * raw asm() clrwdt/sleep; the host sim no-ops.
 */

#include "core/pic16f5x_wdt_sleep.h"
#ifdef __has_include
#if __has_include(<epic-cc.h>)
#include <epic-cc.h>
#endif
#endif

#ifndef __epic_clrwdt
#define __epic_clrwdt() asm volatile("clrwdt")
#endif
#ifndef __epic_sleep
#define __epic_sleep() asm volatile("sleep")
#endif

/**
 * @brief  Feed the watchdog timer with the epic-cc clrwdt intrinsic.
 */
void EPIC_WDT_Refresh(void)
{
    __epic_clrwdt();
}

/**
 * @brief  Enter sleep with the epic-cc sleep intrinsic.
 */
void EPIC_Sleep_Enter(void)
{
    __epic_sleep();
}
