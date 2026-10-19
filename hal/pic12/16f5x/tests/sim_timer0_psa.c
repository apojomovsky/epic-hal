/* Timer0 PSA bypass and prescaler reset probe: PSA=1 runs TMR0
 * 1:1, PSA=0 divides, and reset clears the divide phase. */

#include <stdio.h>

#include "pic16f5x_hal.h"
#include "pic16f5x_sfr.h"
#include "pic16f5x_sim.h"

/**
 * @brief Probe Timer0 PSA bypass, divide ratio, and reset phase.
 */
int main(void)
{
    /* (a) PSA=1 bypasses the divider: 4 cycles give TMR0 == 4. */
    pic16f5x_sim_reset();
    pic16f5x_sim_option = 0x08U;
    pic16f5x_sim_step(4U);
    if (EPIC_REG8(PIC_REG_TMR0) != 4U)
    {
        return 1;
    }

    /* (b) PSA=0 PS=001 divides 1:4: reset leaves TMR0 == 0, and
     * 8 cycles give TMR0 == 2. */
    pic16f5x_sim_reset();
    pic16f5x_sim_option = 0x01U;
    if (EPIC_REG8(PIC_REG_TMR0) != 0U)
    {
        return 1;
    }
    pic16f5x_sim_step(8U);
    if (EPIC_REG8(PIC_REG_TMR0) != 2U)
    {
        return 1;
    }

    /* (c) Reset clears the divide phase: 1 cycle at 1:2, reset,
     * then 1 cycle must not tick TMR0. */
    pic16f5x_sim_reset();
    pic16f5x_sim_option = 0x00U;
    pic16f5x_sim_step(1U);
    pic16f5x_sim_reset();
    pic16f5x_sim_option = 0x00U;
    pic16f5x_sim_step(1U);
    if (EPIC_REG8(PIC_REG_TMR0) != 0U)
    {
        return 1;
    }

    printf("timer0_psa OK\n");
    return 0;
}
