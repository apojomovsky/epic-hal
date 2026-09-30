/* WDT / Sleep / BOR / POR status smoke test: after sim_reset the BOR
 * and POR flags read set (fresh-POR PCON), the ClearFlag helpers clear
 * them, the software WDT enable and prescaler program WDTCON (Bank 1
 * on this family), and the WDT/Sleep no-ops do not crash. */

#include "pic16f63x_67x_68x_hal.h"
#include "pic16f63x_67x_68x_sim.h"
#include "pic16f63x_67x_68x_sfr.h"
#include "core/pic14_wdt_sleep.h"
#include <stdio.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } \
} while (0)

/**
 * @brief Smoke-test the WDT/Sleep/BOR/POR helpers on the sim backend.
 */
int main(void)
{
    pic16f63x_67x_68x_sim_reset();

    /* After POR, both flags should read set. */
    CHECK(EPIC_POR_GetStatus() == 1U, "POR not set after reset");
    CHECK(EPIC_BOR_GetStatus() == 1U, "BOR not set after reset");

    /* Clear them. */
    EPIC_POR_ClearFlag();
    CHECK(EPIC_POR_GetStatus() == 0U, "POR not cleared");
    EPIC_BOR_ClearFlag();
    CHECK(EPIC_BOR_GetStatus() == 0U, "BOR not cleared");

    /* Software WDT enable programs WDTCON<SWDTEN> (Bank 1). */
    EPIC_WDT_SetSoftwareEnable(1U);
    CHECK((EPIC_REG8(PIC_REG_WDTCON) & PIC_WDTCON_SWDTEN) != 0U,
          "SWDTEN not set");
    EPIC_WDT_SetSoftwareEnable(0U);
    CHECK((EPIC_REG8(PIC_REG_WDTCON) & PIC_WDTCON_SWDTEN) == 0U,
          "SWDTEN not cleared");

    /* Prescaler programs WDTCON<WDTPS>. */
    EPIC_WDT_SetPrescaler(5U);
    CHECK((EPIC_REG8(PIC_REG_WDTCON) & PIC_WDTCON_WDTPS_MASK) ==
          (uint8_t)(5U << PIC_WDTCON_WDTPS_POS), "WDTPS not programmed");

    /* WDT refresh and Sleep are no-ops on sim but must not crash. */
    EPIC_WDT_Refresh();
    EPIC_Sleep_Enter();

    printf("OK: WDT/Sleep/BOR/POR helpers, flags and no-op instructions.\n");
    return 0;
}
