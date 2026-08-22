/* epic-cc implementation of EPIC_WDT_Refresh / EPIC_Sleep_Enter
 * (sibling to src/target/pic16f87xa_wdt_sleep_target.c); uses the
 * epic-cc intrinsics from epic-cc.h. Linked by the epic-cc build. */

#include "core/pic16f87xa_wdt_sleep.h"
#include <epic-cc.h>

/**
 * @brief Refresh the Watchdog Timer with the native clrwdt instruction.
 */
void EPIC_WDT_Refresh(void)
{
    __epic_clrwdt();
}

/**
 * @brief Enter Sleep with the native sleep instruction. The device
 *        halts until any enabled interrupt wakes it.
 */
void EPIC_Sleep_Enter(void)
{
    __epic_sleep();
}
