/* WDT / Sleep smoke test: OPTION_REG must come out of reset at its
 * silicon POR (all ones, prescaler on the WDT), and the WDT/Sleep
 * no-ops must not crash. The 7x host build carries no PCON-tied POR/BOR
 * helpers, so this test covers the OPTION reset image only. */

#include "pic16f7x.h"
#include "pic16f7x_sim.h"
#include "pic16f7x_sfr.h"
#include "core/pic14_wdt_sleep.h"
#include <stdio.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } \
} while (0)

/**
 * @brief Smoke-test the OPTION POR image and WDT/Sleep no-ops on sim.
 */
int main(void)
{
    pic16f7x_sim_reset();

    /* OPTION_REG resets to all ones, so PSA=1 parks the prescaler on
     * the WDT (silicon POR, not Timer0). */
    CHECK(EPIC_REG8(PIC_REG_OPTION) == PIC_OPTION_POR_VALUE, "OPTION != POR after reset");
    CHECK((EPIC_REG8(PIC_REG_OPTION) & PIC_OPTION_PSA) != 0U, "prescaler not on WDT after reset");

    /* WDT refresh and Sleep are no-ops on sim but must not crash. */
    EPIC_WDT_Refresh();
    EPIC_Sleep_Enter();

    printf("OK: OPTION POR image and WDT/Sleep no-op instructions.\n");
    return 0;
}
