/*
 * epic-cc implementation of EPIC_WDT_Refresh / EPIC_Sleep_Enter
 * (sibling to src/target/pic18fxx5x_wdt_sleep_target.c).
 */

#include "core/pic18fxx5x_wdt_sleep.h"
#include <epic-cc.h>

/**
 * @brief Refresh the Watchdog Timer via the native clrwdt instruction.
 */
void EPIC_WDT_Refresh(void)
{
    __epic_clrwdt();
}

/**
 * @brief Enter Sleep via the native sleep instruction.
 */
void EPIC_Sleep_Enter(void)
{
    __epic_sleep();
}
