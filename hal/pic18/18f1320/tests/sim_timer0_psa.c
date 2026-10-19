#include <stdint.h>
#include <stdio.h>

#include "pic18f1320.h"
#include "pic18f1320_sfr.h"
#include "pic18f1320_sim.h"

/**
 * @brief Timer0 PSA bypass and prescaler reset behavior.
 * @return 0 on OK, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;

    /* PSA bypass: each cycle ticks TMR0L. */
    pic18_sim_reset();
    EPIC_REG8(PIC_REG_T0CON) =
        (uint8_t)(PIC_T0CON_TMR0ON | PIC_T0CON_T08BIT | PIC_T0CON_PSA);
    pic18_sim_step(3U);
    ok &= (EPIC_REG8(PIC_REG_TMR0L) == 3U);

    /* 1:4 prescaler: 8 cycles tick twice. */
    pic18_sim_reset();
    EPIC_REG8(PIC_REG_T0CON) =
        (uint8_t)(PIC_T0CON_TMR0ON | PIC_T0CON_T08BIT | 0x01U);
    EPIC_REG8(PIC_REG_TMR0L) = 0U;
    pic18_sim_step(8U);
    ok &= (EPIC_REG8(PIC_REG_TMR0L) == 2U);

    /* Reset clears the prescaler phase: one cycle stays at zero twice. */
    pic18_sim_reset();
    EPIC_REG8(PIC_REG_T0CON) =
        (uint8_t)(PIC_T0CON_TMR0ON | PIC_T0CON_T08BIT);
    pic18_sim_step(1U);
    pic18_sim_reset();
    EPIC_REG8(PIC_REG_T0CON) =
        (uint8_t)(PIC_T0CON_TMR0ON | PIC_T0CON_T08BIT);
    pic18_sim_step(1U);
    ok &= (EPIC_REG8(PIC_REG_TMR0L) == 0U);

    printf("timer0_psa: %s\n", ok ? "OK" : "FAIL");
    return ok ? 0 : 1;
}
