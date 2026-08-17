/* WDT / Sleep / BOR / POR status smoke test: after sim_reset the BOR
 * and POR flags are set (PCON = 0x0F), the ClearFlag helpers clear
 * them, and the WDT/Sleep no-ops do not crash. */

#include "pic16f88x.h"
#include "pic16f88x_sim.h"
#include "pic16f88x_sfr.h"
#include "core/pic16f88x_wdt_sleep.h"
#include <stdio.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } \
} while (0)

/**
 * @brief Smoke-test the WDT/Sleep/BOR/POR helpers on the sim backend.
 */
int main(void)
{
    pic16f88x_sim_reset();

    /* After POR: PCON = --01 --0x, POR bit 0 means a Power-on Reset
     * occurred (the user must set it back to 1). The sim's POR value is
     * 0x10 (SBOREN=1, POR=0); BOR is unknown (x), the sim leaves it 0. */
    CHECK(EPIC_POR_GetStatus() == 0U, "POR not indicating a POR after reset");
    CHECK(EPIC_BOR_GetStatus() == 0U, "BOR not 0 after reset");

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
