/* WDT / Sleep / PCON smoke test. PCON carries only nBOR (bit 0) and
 * nPOR (bit 1), with bits 7:2 unimplemented and reading as 0 and no
 * SBOREN, WDTCON or ULPWU anywhere on this die (DS39598F Register 2-8,
 * §12.9). The sim models both flags set after reset
 * (PIC_PCON_POR_VALUE = 0x03), the ClearFlag helpers clear them one at
 * a time, and the WDT refresh and Sleep helpers are no-ops on the host
 * that must not fault. */

#include "pic16f818_819_hal.h"
#include "pic16f818_819_sim.h"
#include "pic16f818_819_sfr.h"
#include "core/pic14_wdt_sleep.h"
#include <stdio.h>

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m) do { if (c) { g_pass++; } else { printf("FAIL: %s\n", m); g_fail++; } } while (0)

/**
 * @brief Smoke-test the WDT/Sleep helpers and the PCON nBOR/nPOR image
 *        on the sim backend.
 */
int main(void)
{
    pic16f818_819_sim_reset();

    /* After POR both flags are modelled set (PCON = 0x03). */
    CHECK(EPIC_REG8(PIC_REG_PCON) == PIC_PCON_POR_VALUE,
          "PCON (0x8E) != 0x03 (nBOR | nPOR) after reset");
    CHECK(EPIC_POR_GetStatus() == 1U, "POR not set after reset");
    CHECK(EPIC_BOR_GetStatus() == 1U, "BOR not set after reset");

    /* Clear them one at a time, the unimplemented bits stay 0. */
    EPIC_POR_ClearFlag();
    CHECK(EPIC_POR_GetStatus() == 0U, "POR not cleared");
    CHECK(EPIC_REG8(PIC_REG_PCON) == PIC_PCON_BOR, "PCON != nBOR after POR clear");
    EPIC_BOR_ClearFlag();
    CHECK(EPIC_BOR_GetStatus() == 0U, "BOR not cleared");
    CHECK(EPIC_REG8(PIC_REG_PCON) == 0x00U, "PCON != 0x00 after both clears");

    /* WDT refresh and Sleep are no-ops on sim but must not fault. */
    EPIC_WDT_Refresh();
    EPIC_Sleep_Enter();

    printf("example_wdt_sleep: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
