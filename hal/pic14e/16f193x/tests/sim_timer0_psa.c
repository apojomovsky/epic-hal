/* Timer0 PSA bypass and prescaler reset probe. */

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "pic16f193x_sim.h"
#include <stdio.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } \
} while (0)

/**
 * @brief Check the Timer0 PSA bypass and prescaler reset behavior.
 * @return 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    pic16f193x_sim_reset();
    EPIC_REG8(PIC_REG_OPTION) = 0x08U;
    EPIC_REG8(PIC_REG_TMR0) = 0x00U;
    pic16f193x_sim_step(4U);
    CHECK(EPIC_REG8(PIC_REG_TMR0) == 4U, "PSA=1 did not count 1:1");

    pic16f193x_sim_reset();
    EPIC_REG8(PIC_REG_OPTION) = 0x01U;
    EPIC_REG8(PIC_REG_TMR0) = 0x00U;
    pic16f193x_sim_step(8U);
    CHECK(EPIC_REG8(PIC_REG_TMR0) == 2U, "1:4 prescaler did not divide");

    pic16f193x_sim_reset();
    EPIC_REG8(PIC_REG_OPTION) = 0x00U;
    pic16f193x_sim_step(1U);
    pic16f193x_sim_reset();
    EPIC_REG8(PIC_REG_OPTION) = 0x00U;
    pic16f193x_sim_step(1U);
    CHECK(EPIC_REG8(PIC_REG_TMR0) == 0U, "reset did not clear prescaler");

    printf("OK: Timer0 PSA bypass and prescaler reset.\n");
    return 0;
}
