/*
 * Comparator smoke: program two-independent mode, verify CMCON.
 * Host sim verifies programming; mdb gate proves CMCON on hardware.
 */

#include "pic18f2520_hal.h"
#include "pic18f2520_sfr.h"
#include "peripherals/pic18f2520_comp.h"
#include "core/epic_harness.h"

/** Simulated run length (host only). */
#define SIM_CYCLES  1000UL

/**
 * @brief  Program comparators + verify CMCON.
 *
 *          PASS if CMCON programs to two-independent mode (CM=010).
 */
int main(void)
{
    epic_harness_init(SIM_CYCLES);
    uint8_t ok = 1U;

    COMP_HandleTypeDef h = COMP_HANDLE_DEFAULT;
    if (EPIC_COMP_Init(&h) != EPIC_OK) ok = 0U;

    uint8_t cmcon = epic_sfr_read8(PIC_REG_CMCON);
    if ((cmcon & PIC_CMCON_CM_MASK) != COMP_MODE_TWO_INDEP) ok = 0U;

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
    }

    epic_harness_log("Comparators programmed (CMCON=0x%02X).\n", (unsigned)cmcon);
    return epic_harness_report(ok);
}
