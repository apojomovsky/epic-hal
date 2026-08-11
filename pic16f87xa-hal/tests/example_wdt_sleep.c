/* WDT / Sleep / BOR / POR status smoke test: after sim_reset the BOR
 * and POR flags are set (PCON = 0x0F), the ClearFlag helpers clear
 * them, and the WDT/Sleep no-ops do not crash. */

#include "pic16f87xa.h"
#include "pic16f87xa_sim.h"
#include "pic16f87xa_sfr.h"
#include "core/pic16f87xa_wdt_sleep.h"
#include <stdio.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } \
} while (0)

/**
 * @brief Smoke-test the WDT/Sleep/BOR/POR helpers on the sim backend.
 */
int main(void)
{
    pic16f87xa_sim_reset();

    /* After POR, both flags should be set. */
    CHECK(EPIC_POR_GetStatus() == 1U, "POR not set after reset");
    CHECK(EPIC_BOR_GetStatus() == 1U, "BOR not set after reset");

    /* Clear them. */
    EPIC_POR_ClearFlag();
    CHECK(EPIC_POR_GetStatus() == 0U, "POR not cleared");
    EPIC_BOR_ClearFlag();
    CHECK(EPIC_BOR_GetStatus() == 0U, "BOR not cleared");

    /* WDT refresh and Sleep are no-ops on sim but must not crash. */
    EPIC_WDT_Refresh();
    EPIC_Sleep_Enter();

    printf("OK: WDT/Sleep/BOR/POR helpers, flags and no-op instructions.\n");
    return 0;
}
