/* WDT / Sleep / BOR / POR status smoke test: the active-low PCON flags
 * are driven the way hardware does and the helpers must report and
 * re-arm them correctly, and the WDT/Sleep no-ops must not crash. */

#include "pic16f88x.h"
#include "pic16f88x_sim.h"
#include "pic16f88x_sfr.h"
#include "core/pic14_wdt_sleep.h"
#include <stdio.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } \
} while (0)

/* PCON<1:0> are nPOR and nBOR on this family. */
#define TEST_PCON_nBOR  0x01U
#define TEST_PCON_nPOR  0x02U
#define TEST_PCON_MASK  0x03U

/**
 * @brief Smoke-test the WDT/Sleep/BOR/POR helpers on the sim backend.
 */
int main(void)
{
    pic16f88x_sim_reset();
    /* OPTION_REG resets to all ones, so PSA=1 parks the prescaler on
     * the WDT (silicon POR, not Timer0). */
    CHECK(EPIC_REG8(PIC_REG_OPTION) == PIC_OPTION_POR_VALUE, "OPTION != POR after reset");
    CHECK((EPIC_REG8(PIC_REG_OPTION) & PIC_OPTION_PSA) != 0U, "prescaler not on WDT after reset");

    /* PCON<POR> and PCON<BOR> are active low: hardware clears the bit on
     * the event. Set the bits the way hardware does instead of relying on
     * the sim's reset image. */
    EPIC_REG8(PIC_REG_PCON) |= (uint8_t)(TEST_PCON_nPOR | TEST_PCON_nBOR);
    CHECK(EPIC_POR_GetStatus() == 0U, "POR reported with nPOR set");
    CHECK(EPIC_BOR_GetStatus() == 0U, "BOR reported with nBOR set");
    EPIC_REG8(PIC_REG_PCON) &= (uint8_t)~TEST_PCON_nPOR;
    CHECK(EPIC_POR_GetStatus() == 1U, "POR not reported with nPOR clear");
    CHECK(EPIC_BOR_GetStatus() == 0U, "BOR reported by a POR-only event");
    EPIC_REG8(PIC_REG_PCON) &= (uint8_t)~TEST_PCON_nBOR;
    CHECK(EPIC_BOR_GetStatus() == 1U, "BOR not reported with nBOR clear");

    /* ClearFlag re-arms the flag by setting the bit back. */
    EPIC_POR_ClearFlag();
    CHECK(EPIC_POR_GetStatus() == 0U, "POR still reported after ClearFlag");
    CHECK(EPIC_BOR_GetStatus() == 1U, "POR ClearFlag disturbed BOR");
    CHECK((EPIC_REG8(PIC_REG_PCON) & TEST_PCON_MASK) == TEST_PCON_nPOR,
          "PCON != nPOR after POR ClearFlag");
    EPIC_BOR_ClearFlag();
    CHECK(EPIC_BOR_GetStatus() == 0U, "BOR still reported after ClearFlag");
    CHECK((EPIC_REG8(PIC_REG_PCON) & TEST_PCON_MASK) == TEST_PCON_MASK,
          "PCON != nPOR | nBOR after both ClearFlags");

    /* WDT refresh and Sleep are no-ops on sim but must not crash. */
    EPIC_WDT_Refresh();
    EPIC_Sleep_Enter();

    printf("OK: WDT/Sleep/BOR/POR helpers, flags and no-op instructions.\n");
    return 0;
}
