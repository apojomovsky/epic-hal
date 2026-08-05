/**
 * @file    example_comp.c
 * @brief   Comparator driver smoke test on the PIC18 host sim.
 *
 * @details
 *   Verifies init CMCON programming (DS39632E §22.0), sim-driven C1/C2
 *   output + CMIF, flag tracking, and deinit. The change IRQ handler
 *   clears CMIF, so polling checks disable the sim IRQ callback.
 */

#include "epic_hal.h"
#include "core/epic_harness.h"
#include "pic18fxx5x_sim.h"

#define CHECK(cond, msg) do { \
    if (!(cond)) { epic_harness_log("FAIL: %s\n", msg); return epic_harness_report(0); } \
} while (0)

int main(void)
{
    epic_harness_init(16U);
    /* Disable the sim IRQ callback so the COMP handler does not clear CMIF
     * before the polling checks read it. */
    pic18_sim_set_irq_callback(NULL);

    /* 1. Two independent comparators, both inverted, input switch on.
     *    CMCON = mode 010 | CIS(bit3) | C1INV(bit4) | C2INV(bit5)
     *          = 0x02 | 0x08 | 0x10 | 0x20 = 0x3A. */
    COMP_HandleTypeDef h = COMP_HANDLE_DEFAULT;
    h.Mode       = COMP_MODE_TWO_INDEP;
    h.C1Inverted = true;
    h.C2Inverted = true;
    h.CIS        = true;
    EPIC_COMP_Init(&h);

    CHECK(epic_sfr_read8(PIC_REG_CMCON) == 0x3AU,
          "CMCON not 0x3A for two-indep, both inverted, CIS");

    /* 2. Drive the comparator outputs. C1OUT is bit 6 (0x40), C2OUT bit 7. */
    pic18_sim_drive_comp(1U, 0U);
    CHECK(EPIC_COMP_C1Out() == 1U, "C1Out not 1 after drive_comp(1,0)");
    CHECK(EPIC_COMP_C2Out() == 0U, "C2Out not 0 after drive_comp(1,0)");
    CHECK(EPIC_COMP_IsChangeFlag() == 1U, "CMIF not set after drive_comp");

    pic18_sim_drive_comp(1U, 1U);
    CHECK(EPIC_COMP_C2Out() == 1U, "C2Out not 1 after drive_comp(1,1)");

    /* 3. Clear the change flag. */
    EPIC_COMP_ClearChangeFlag();
    CHECK(EPIC_COMP_IsChangeFlag() == 0U, "CMIF not cleared after ClearChangeFlag");

    /* 4. DeInit restores the POR default (comparators off). */
    EPIC_COMP_DeInit();
    CHECK(epic_sfr_read8(PIC_REG_CMCON) == 0x07U,
          "CMCON not 0x07 (off) after DeInit");

    epic_harness_log("OK: Comparator driver, mode/inv/CIS, outputs, change flag all pass.\n");
    return epic_harness_report(1);
}
