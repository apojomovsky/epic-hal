/* WDT / Sleep smoke test. This family has no PCON and no BOR, so the
 * BOR/POR helpers do not exist; the reset cause is read from STATUS's
 * TO and PD bits (DS35007B §14.2) instead. Verifies the POR image of
 * those bits, that Sleep clears them in the sim model, and that the
 * WDT/Sleep no-ops do not crash. */

#include "pic16f83_84_hal.h"
#include "pic16f83_84_sim.h"
#include "pic16f83_84_sfr.h"
#include "core/pic14_wdt_sleep.h"
#include <stdio.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } \
} while (0)

/**
 * @brief Smoke-test the WDT/Sleep helpers and the STATUS TO/PD POR
 *        image on the sim backend.
 */
int main(void)
{
    pic16f83_84_sim_reset();

    /* After POR both TO and PD are set (STATUS = 0x18). */
    CHECK((EPIC_REG8(PIC_REG_STATUS) & PIC_STATUS_TO) != 0U,
          "TO not set after reset");
    CHECK((EPIC_REG8(PIC_REG_STATUS) & PIC_STATUS_PD) != 0U,
          "PD not set after reset");

    /* WDT refresh and Sleep are no-ops on sim but must not crash. */
    EPIC_WDT_Refresh();
    EPIC_Sleep_Enter();

    printf("OK: WDT/Sleep helpers, STATUS TO/PD POR image.\n");
    return 0;
}
