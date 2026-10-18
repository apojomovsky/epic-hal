/**
 * Host model of Timer4, stepped once per tick by pic16f188xx_sim.c. Basic
 * free-running mode only: internal clock sources count (FOSC counts four
 * clocks per tick, the oscillator rows one, like Timer2), a PR4 match
 * clears TMR4 and steps the postscaler, and its terminal count sets
 * PIR4.TMR4IF. T4HLT and T4RST are stored but not modelled: a nonzero
 * T4HLT MODE holds the counter, and the pin, CLKR, NCO, ZCD and CLC
 * clock rows never tick.
 */
#include <stdint.h>
#include "pic16f188xx.h"
#include "pic16f188xx_sim.h"
#include "pic16f188xx_sim_priv.h"
#include "peripherals/pic16f188xx_timer4.h"

static uint32_t sim_t4_prescale_acc;
static uint8_t sim_t4_post_acc;
static uint8_t sim_t4_wrap;

/**
 * @brief Clear the model state and load the POR value of PR4.
 * @details PR4 powers up 0xFF; the other Timer4 SFRs power up 0x00 (EDC por).
 */
void pic16f188xx_sim_timer4_reset(void)
{
    sim_t4_prescale_acc = 0U;
    sim_t4_post_acc = 0U;
    sim_t4_wrap = 0U;
    pic16f188xx_sim_sfr[PIC_REG_PR4] = 0xFFU;
}

/**
 * @brief Advance Timer4 by one instruction tick.
 * @details Halting (ON clear) clears the dividers and keeps TMR4, like
 * Timer2. The match tick clears TMR4 to 0 and raises the wrap view for the
 * PWM model; the postscaled terminal count sets TMR4IF.
 */
void pic16f188xx_sim_timer4_step(void)
{
    uint8_t con = pic16f188xx_sim_sfr[PIC_REG_T4CON];
    uint8_t cs = (uint8_t)(pic16f188xx_sim_sfr[PIC_REG_T4CLKCON] & PIC_T4CLKCON_CS_MASK);
    uint8_t outps = (uint8_t)(con & PIC_T4CON_OUTPS_MASK);
    uint32_t ratio = 1UL << ((con & PIC_T4CON_CKPS_MASK) >> PIC_T4CON_CKPS_SHIFT);
    uint8_t clocks = 1U;

    sim_t4_wrap = 0U;
    if ((con & PIC_T4CON_ON) == 0U)
    {
        sim_t4_prescale_acc = 0U;
        sim_t4_post_acc = 0U;
        return;
    }
    if ((pic16f188xx_sim_sfr[PIC_REG_T4HLT] & PIC_T4HLT_MODE_MASK) != 0U)
    {
        return;
    }
    if (cs == (uint8_t)TIMER4_CS_FOSC)
    {
        clocks = 4U;
    }
    else if ((cs != (uint8_t)TIMER4_CS_FOSC4) && (cs != (uint8_t)TIMER4_CS_HFINTOSC) &&
             (cs != (uint8_t)TIMER4_CS_LFINTOSC) && (cs != (uint8_t)TIMER4_CS_MFINTOSC16) &&
             (cs != (uint8_t)TIMER4_CS_SOSC))
    {
        return;
    }
    while (clocks != 0U)
    {
        clocks--;
        sim_t4_prescale_acc++;
        if (sim_t4_prescale_acc < ratio)
        {
            continue;
        }
        sim_t4_prescale_acc = 0U;
        if (pic16f188xx_sim_sfr[PIC_REG_TMR4] != pic16f188xx_sim_sfr[PIC_REG_PR4])
        {
            pic16f188xx_sim_sfr[PIC_REG_TMR4]++;
            continue;
        }
        pic16f188xx_sim_sfr[PIC_REG_TMR4] = 0U;
        sim_t4_wrap = 1U;
        sim_t4_post_acc++;
        if (sim_t4_post_acc > outps)
        {
            sim_t4_post_acc = 0U;
            pic16f188xx_sim_sfr[PIC_REG_PIR4] |= PIC_PIR4_TMR4IF;
        }
    }
}

/**
 * @brief Timer4 view for the CCP and PWM model.
 * @details Live means ON with FOSC/4 as the clock, the only clock PWM
 * accepts. The position is TMR4 x 4 plus the prescaler fraction in quarter
 * steps, the 10-bit time base the duty compares against.
 * @param base Out: wrap, live and position of this tick.
 */
void pic16f188xx_sim_timer4_base(pic16f188xx_sim_pwm_base_t *base)
{
    uint8_t con = pic16f188xx_sim_sfr[PIC_REG_T4CON];
    uint32_t ratio = 1UL << ((con & PIC_T4CON_CKPS_MASK) >> PIC_T4CON_CKPS_SHIFT);

    base->wrap = sim_t4_wrap;
    base->live = (uint8_t)(((con & PIC_T4CON_ON) != 0U) &&
        ((pic16f188xx_sim_sfr[PIC_REG_T4CLKCON] & PIC_T4CLKCON_CS_MASK) ==
         (uint8_t)TIMER4_CS_FOSC4));
    base->pos = (uint16_t)(((uint32_t)pic16f188xx_sim_sfr[PIC_REG_TMR4] * 4UL) +
        ((sim_t4_prescale_acc * 4UL) / ratio));
}
