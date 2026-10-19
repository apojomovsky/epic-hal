#include "pic16f63x_67x_68x_hal.h"
#include "pic16f63x_67x_68x_sim.h"
#include "pic16f63x_67x_68x_sfr.h"
#include <stdio.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } \
} while (0)

/**
 * @brief Verify Timer0 PSA bypass and prescaler reset behavior.
 */
int main(void)
{
    pic16f63x_67x_68x_sim_reset();
    EPIC_REG8(PIC_REG_OPTION) = 0x08U;
    pic16f63x_67x_68x_sim_step(4U);
    CHECK(EPIC_REG8(PIC_REG_TMR0) == 4U, "PSA=1 did not bypass prescaler");

    pic16f63x_67x_68x_sim_reset();
    EPIC_REG8(PIC_REG_OPTION) = 0x01U;
    CHECK(EPIC_REG8(PIC_REG_TMR0) == 0U, "TMR0 != 0 after reset");
    pic16f63x_67x_68x_sim_step(8U);
    CHECK(EPIC_REG8(PIC_REG_TMR0) == 2U, "1:4 prescaler ratio wrong");

    pic16f63x_67x_68x_sim_reset();
    EPIC_REG8(PIC_REG_OPTION) = 0x00U;
    pic16f63x_67x_68x_sim_step(1U);
    pic16f63x_67x_68x_sim_reset();
    EPIC_REG8(PIC_REG_OPTION) = 0x00U;
    pic16f63x_67x_68x_sim_step(1U);
    CHECK(EPIC_REG8(PIC_REG_TMR0) == 0U, "reset did not clear prescaler");

    printf("OK: Timer0 PSA bypass and prescaler reset.\n");
    return 0;
}
