/**
 * Host model of CCP1..CCP5 and PWM6/PWM7, stepped once per tick by
 * pic16f188xx_sim.c after the timers. Capture and compare model only the
 * Timer1/Timer2 couple (CCPTMRS value 01, TMR1). PWM runs on the
 * Timer1/Timer2 couple (TMR2) and the Timer3/Timer4 couple (value 10,
 * TMR4); the caller passes in each 10-bit time base. Other couples never
 * fire and hold the output low. Capture reads the pin CCPxPPS selects; outputs
 * are read back through pic16f188xx_sim_ccp_out and _pwm_out.
 */
#include <stdint.h>
#include <string.h>
#include "pic16f188xx.h"
#include "pic16f188xx_sim.h"
#include "pic16f188xx_sim_priv.h"
#include "peripherals/pic16f188xx_ccp.h"

#define SIM_CCP_N 5U
#define SIM_PWM_N 2U

static const uint16_t sim_ccp_l[SIM_CCP_N] = {
    PIC_REG_CCPR1L, PIC_REG_CCPR2L, PIC_REG_CCPR3L, PIC_REG_CCPR4L, PIC_REG_CCPR5L};
static const uint16_t sim_ccp_h[SIM_CCP_N] = {
    PIC_REG_CCPR1H, PIC_REG_CCPR2H, PIC_REG_CCPR3H, PIC_REG_CCPR4H, PIC_REG_CCPR5H};
static const uint16_t sim_ccp_con[SIM_CCP_N] = {
    PIC_REG_CCP1CON, PIC_REG_CCP2CON, PIC_REG_CCP3CON, PIC_REG_CCP4CON, PIC_REG_CCP5CON};
static const uint16_t sim_ccp_cap[SIM_CCP_N] = {
    PIC_REG_CCP1CAP, PIC_REG_CCP2CAP, PIC_REG_CCP3CAP, PIC_REG_CCP4CAP, PIC_REG_CCP5CAP};
static const uint16_t sim_ccp_pps[SIM_CCP_N] = {
    PIC_REG_CCP1PPS, PIC_REG_CCP2PPS, PIC_REG_CCP3PPS, PIC_REG_CCP4PPS, PIC_REG_CCP5PPS};
static const uint8_t sim_ccp_flag[SIM_CCP_N] = {
    PIC_PIR6_CCP1IF, PIC_PIR6_CCP2IF, PIC_PIR6_CCP3IF, PIC_PIR6_CCP4IF, PIC_PIR6_CCP5IF};
static const uint16_t sim_pwm_dcl[SIM_PWM_N] = {PIC_REG_PWM6DCL, PIC_REG_PWM7DCL};
static const uint16_t sim_pwm_dch[SIM_PWM_N] = {PIC_REG_PWM6DCH, PIC_REG_PWM7DCH};
static const uint16_t sim_pwm_con[SIM_PWM_N] = {PIC_REG_PWM6CON, PIC_REG_PWM7CON};

/* Per-instance model state. mode_img holds EN and MODE as last seen, so a
 * change resets the output latch and the capture prescaler like the
 * hardware reset a CCPxCON write gives (DS40001802D §30.2.1 note). */
static uint8_t sim_ccp_mode_img[SIM_CCP_N];
static uint8_t sim_ccp_out_lat[SIM_CCP_N];
static uint8_t sim_ccp_pulse[SIM_CCP_N];
static uint8_t sim_ccp_pin_prev[SIM_CCP_N];
static uint8_t sim_ccp_edges[SIM_CCP_N];
static uint16_t sim_ccp_duty[SIM_CCP_N];
static uint16_t sim_pwm_duty[SIM_PWM_N];
static uint8_t sim_pwm_hi[SIM_PWM_N];
static uint8_t sim_pwm_pin[SIM_PWM_N];
static uint16_t sim_t1_prev;

/**
 * @brief Clear the model state and load the POR values of its SFRs.
 * @details CCPTMRS0/1 power up 0x55/0x15 (Timer1/Timer2 couple) and the
 * CCPxPPS selects at RC2, RC1, RB5, RB0 and RA4 (EDC por, Table 13-1).
 */
void pic16f188xx_sim_ccp_reset(void)
{
    memset(sim_ccp_mode_img, 0, sizeof sim_ccp_mode_img);
    memset(sim_ccp_out_lat, 0, sizeof sim_ccp_out_lat);
    memset(sim_ccp_pulse, 0, sizeof sim_ccp_pulse);
    memset(sim_ccp_pin_prev, 0, sizeof sim_ccp_pin_prev);
    memset(sim_ccp_edges, 0, sizeof sim_ccp_edges);
    memset(sim_ccp_duty, 0, sizeof sim_ccp_duty);
    memset(sim_pwm_duty, 0, sizeof sim_pwm_duty);
    memset(sim_pwm_hi, 0, sizeof sim_pwm_hi);
    memset(sim_pwm_pin, 0, sizeof sim_pwm_pin);
    sim_t1_prev = 0U;
    pic16f188xx_sim_sfr[PIC_REG_CCPTMRS0] = PIC_CCPTMRS0_POR_VALUE;
    pic16f188xx_sim_sfr[PIC_REG_CCPTMRS1] = PIC_CCPTMRS1_POR_VALUE;
    pic16f188xx_sim_sfr[PIC_REG_CCP1PPS] = 0x12U;
    pic16f188xx_sim_sfr[PIC_REG_CCP2PPS] = 0x11U;
    pic16f188xx_sim_sfr[PIC_REG_CCP3PPS] = 0x0DU;
    pic16f188xx_sim_sfr[PIC_REG_CCP4PPS] = 0x08U;
    pic16f188xx_sim_sfr[PIC_REG_CCP5PPS] = 0x04U;
}

/**
 * @brief Timer couple an instance is tied to.
 * @param i Instance index 0..4.
 * @return CCPTMRS field, 1 for the Timer1/Timer2 couple.
 */
static uint8_t sim_ccp_tsel(unsigned int i)
{
    if (i == 4U)
    {
        return (uint8_t)(pic16f188xx_sim_sfr[PIC_REG_CCPTMRS1] & PIC_CCPTMRS_TSEL_MASK);
    }
    return (uint8_t)((pic16f188xx_sim_sfr[PIC_REG_CCPTMRS0] >> (2U * i)) &
        PIC_CCPTMRS_TSEL_MASK);
}

/**
 * @brief Level of the pin CCPxPPS selects.
 * @param i Instance index 0..4.
 * @return 0 or 1; 0 for a port the part lacks.
 */
static uint8_t sim_ccp_pin(unsigned int i)
{
    uint8_t sel = (uint8_t)(pic16f188xx_sim_sfr[sim_ccp_pps[i]] & 0x1FU);
    unsigned int port = (unsigned int)(sel >> 3);

    if ((port > 2U) && ((port != 3U) || (PIC16F188XX_FAMILY_HAS_PORTD == 0)))
    {
        return 0U;
    }
    return (uint8_t)((pic16f188xx_sim_sfr[PIC_REG_PORTA + port] >> (sel & 7U)) & 1U);
}

/**
 * @brief 10-bit PWM duty held in CCPRxH:L for the FMT alignment.
 * @param i Instance index 0..4.
 * @param con The instance's CCPxCON.
 * @return Duty, 0..1023.
 */
static uint16_t sim_ccp_pwm_duty(unsigned int i, uint8_t con)
{
    uint8_t h = pic16f188xx_sim_sfr[sim_ccp_h[i]];
    uint8_t l = pic16f188xx_sim_sfr[sim_ccp_l[i]];

    if ((con & PIC_CCP1CON_FMT) != 0U)
    {
        return (uint16_t)(((uint16_t)h << 2) | (uint16_t)(l >> 6));
    }
    return (uint16_t)((((uint16_t)h & 0x03U) << 8) | l);
}

/**
 * @brief Advance one CCP instance by a tick.
 * @param i Instance index 0..4.
 * @param tmr1 Current TMR1 value.
 * @param t1_changed Non-zero when TMR1 moved this tick (a match is an edge).
 * @param base PWM time bases: [0] Timer2, [1] Timer4.
 */
static void sim_step_ccp_one(unsigned int i, uint16_t tmr1, uint8_t t1_changed,
                             const pic16f188xx_sim_pwm_base_t base[2])
{
    uint8_t tsel = sim_ccp_tsel(i);
    uint8_t con = pic16f188xx_sim_sfr[sim_ccp_con[i]];
    uint8_t mode = (uint8_t)(con & PIC_CCP1CON_MODE_MASK);
    uint8_t img = (uint8_t)((con & PIC_CCP1CON_EN) | mode);
    uint8_t en = (uint8_t)(((con & PIC_CCP1CON_EN) != 0U) && (mode != 0U));
    uint8_t pin = (((pic16f188xx_sim_sfr[sim_ccp_cap[i]] & PIC_CCPCAP_CTS_MASK) == 0U)) ?
        sim_ccp_pin(i) : 0U;
    uint8_t rise = (uint8_t)((pin != 0U) && (sim_ccp_pin_prev[i] == 0U));
    uint8_t fall = (uint8_t)((pin == 0U) && (sim_ccp_pin_prev[i] != 0U));
    uint16_t ccpr = (uint16_t)(((uint16_t)pic16f188xx_sim_sfr[sim_ccp_h[i]] << 8) |
        pic16f188xx_sim_sfr[sim_ccp_l[i]]);
    uint8_t event = 0U;
    uint8_t need = 0U;

    sim_ccp_pin_prev[i] = pin;
    if (img != sim_ccp_mode_img[i])
    {
        sim_ccp_mode_img[i] = img;
        sim_ccp_out_lat[i] = 0U;
        sim_ccp_pulse[i] = 0U;
        sim_ccp_edges[i] = 0U;
    }
    if ((en != 0U) &&
        ((tsel == 1U) || ((tsel == 2U) && (mode == (uint8_t)CCP_MODE_PWM))))
    {
        switch (mode)
        {
        case CCP_MODE_CAPTURE_ANY_EDGE:
            event = (uint8_t)(rise | fall);
            break;
        case CCP_MODE_CAPTURE_FALLING:
            event = fall;
            break;
        case CCP_MODE_CAPTURE_RISING:
            event = rise;
            break;
        case CCP_MODE_CAPTURE_4TH:
        case CCP_MODE_CAPTURE_16TH:
            need = (mode == (uint8_t)CCP_MODE_CAPTURE_4TH) ? 4U : 16U;
            if (rise != 0U)
            {
                sim_ccp_edges[i]++;
                if (sim_ccp_edges[i] >= need)
                {
                    sim_ccp_edges[i] = 0U;
                    event = 1U;
                }
            }
            break;
        case CCP_MODE_PWM:
            if (base[tsel - 1U].live != 0U)
            {
                if (base[tsel - 1U].wrap != 0U)
                {
                    sim_ccp_duty[i] = sim_ccp_pwm_duty(i, con);
                    sim_ccp_out_lat[i] = (uint8_t)(sim_ccp_duty[i] != 0U);
                    pic16f188xx_sim_sfr[PIC_REG_PIR6] |= sim_ccp_flag[i];
                }
                if ((sim_ccp_out_lat[i] != 0U) && (base[tsel - 1U].pos >= sim_ccp_duty[i]))
                {
                    sim_ccp_out_lat[i] = 0U;
                }
            }
            break;
        case CCP_MODE_COMPARE_TOGGLE_CLR_TMR:
        case CCP_MODE_COMPARE_TOGGLE:
        case CCP_MODE_COMPARE_SET:
        case CCP_MODE_COMPARE_CLEAR:
        case CCP_MODE_COMPARE_PULSE:
        case CCP_MODE_COMPARE_PULSE_CLR_TMR:
            if (sim_ccp_pulse[i] != 0U)
            {
                sim_ccp_out_lat[i] = 0U;
                sim_ccp_pulse[i] = 0U;
            }
            if ((t1_changed != 0U) && (tmr1 == ccpr))
            {
                pic16f188xx_sim_sfr[PIC_REG_PIR6] |= sim_ccp_flag[i];
                if (mode == (uint8_t)CCP_MODE_COMPARE_SET)
                {
                    sim_ccp_out_lat[i] = 1U;
                }
                else if (mode == (uint8_t)CCP_MODE_COMPARE_CLEAR)
                {
                    sim_ccp_out_lat[i] = 0U;
                }
                else if ((mode == (uint8_t)CCP_MODE_COMPARE_PULSE) ||
                         (mode == (uint8_t)CCP_MODE_COMPARE_PULSE_CLR_TMR))
                {
                    sim_ccp_out_lat[i] = 1U;
                    sim_ccp_pulse[i] = 1U;
                }
                else
                {
                    sim_ccp_out_lat[i] ^= 1U;
                }
                if ((mode == (uint8_t)CCP_MODE_COMPARE_TOGGLE_CLR_TMR) ||
                    (mode == (uint8_t)CCP_MODE_COMPARE_PULSE_CLR_TMR))
                {
                    pic16f188xx_sim_sfr[PIC_REG_TMR1L] = 0U;
                    pic16f188xx_sim_sfr[PIC_REG_TMR1H] = 0U;
                    sim_t1_prev = 0U;
                }
            }
            break;
        default:
            break;
        }
        if (event != 0U)
        {
            pic16f188xx_sim_sfr[sim_ccp_l[i]] = (uint8_t)tmr1;
            pic16f188xx_sim_sfr[sim_ccp_h[i]] = (uint8_t)(tmr1 >> 8);
            pic16f188xx_sim_sfr[PIC_REG_PIR6] |= sim_ccp_flag[i];
        }
    }
    if (sim_ccp_out_lat[i] != 0U)
    {
        pic16f188xx_sim_sfr[sim_ccp_con[i]] |= PIC_CCP1CON_OUT;
    }
    else
    {
        pic16f188xx_sim_sfr[sim_ccp_con[i]] &= (uint8_t)~PIC_CCP1CON_OUT;
    }
}

/**
 * @brief Advance one dedicated PWM module by a tick.
 * @param k Module index 0 (PWM6) or 1 (PWM7).
 * @param base PWM time bases: [0] Timer2, [1] Timer4.
 */
static void sim_step_pwm_one(unsigned int k, const pic16f188xx_sim_pwm_base_t base[2])
{
    uint8_t con = pic16f188xx_sim_sfr[sim_pwm_con[k]];
    uint8_t tsel = (uint8_t)((pic16f188xx_sim_sfr[PIC_REG_CCPTMRS1] >> (2U + (2U * k))) &
        PIC_CCPTMRS_TSEL_MASK);
    uint8_t pol = (uint8_t)((con & PIC_PWM6CON_POL) != 0U);

    if ((con & PIC_PWM6CON_EN) == 0U)
    {
        sim_pwm_hi[k] = 0U;
    }
    else if (((tsel == 1U) || (tsel == 2U)) && (base[tsel - 1U].live != 0U))
    {
        if (base[tsel - 1U].wrap != 0U)
        {
            sim_pwm_duty[k] = (uint16_t)(((uint16_t)pic16f188xx_sim_sfr[sim_pwm_dch[k]] << 2) |
                (uint16_t)(pic16f188xx_sim_sfr[sim_pwm_dcl[k]] >> PIC_PWMDCL_DC_SHIFT));
            sim_pwm_hi[k] = (uint8_t)(sim_pwm_duty[k] != 0U);
        }
        if ((sim_pwm_hi[k] != 0U) && (base[tsel - 1U].pos >= sim_pwm_duty[k]))
        {
            sim_pwm_hi[k] = 0U;
        }
    }
    sim_pwm_pin[k] = (uint8_t)(sim_pwm_hi[k] ^ pol);
    if (sim_pwm_pin[k] != 0U)
    {
        pic16f188xx_sim_sfr[sim_pwm_con[k]] |= PIC_PWM6CON_OUT;
    }
    else
    {
        pic16f188xx_sim_sfr[sim_pwm_con[k]] &= (uint8_t)~PIC_PWM6CON_OUT;
    }
}

/**
 * @brief Advance every CCP and PWM module by a tick.
 * @param base PWM time bases: [0] Timer2, [1] Timer4. Each is the timer's
 *        match tick, FOSC/4 liveness and TMRx x 4 plus the prescaler
 *        fraction, the 10-bit time base.
 */
void pic16f188xx_sim_ccp_step(const pic16f188xx_sim_pwm_base_t base[2])
{
    uint16_t tmr1 = (uint16_t)(((uint16_t)pic16f188xx_sim_sfr[PIC_REG_TMR1H] << 8) |
        pic16f188xx_sim_sfr[PIC_REG_TMR1L]);
    uint8_t changed = (uint8_t)(tmr1 != sim_t1_prev);
    unsigned int i;

    sim_t1_prev = tmr1;
    for (i = 0U; i < SIM_CCP_N; i++)
    {
        sim_step_ccp_one(i, tmr1, changed, base);
    }
    for (i = 0U; i < SIM_PWM_N; i++)
    {
        sim_step_pwm_one(i, base);
    }
}

/**
 * @brief Apply a write to a CCPxCON or PWMxCON register.
 * @details OUT is read-only and keeps its hardware value. A CCPxCON write
 * that changes EN or MODE resets the output latch, the pulse and the
 * capture prescaler at once, as clearing CCPxCON does on silicon.
 * @param addr SFR address written.
 * @param value Value written.
 * @return 1 when the address is a CCP or PWM control register, else 0.
 */
uint8_t pic16f188xx_sim_ccp_write(uint16_t addr, uint8_t value)
{
    unsigned int i;

    for (i = 0U; i < SIM_CCP_N; i++)
    {
        if (addr == sim_ccp_con[i])
        {
            uint8_t img = (uint8_t)(value & (PIC_CCP1CON_EN | PIC_CCP1CON_MODE_MASK));
            uint8_t keep = (uint8_t)(pic16f188xx_sim_sfr[addr] & PIC_CCP1CON_OUT);

            if (img != sim_ccp_mode_img[i])
            {
                sim_ccp_mode_img[i] = img;
                sim_ccp_out_lat[i] = 0U;
                sim_ccp_pulse[i] = 0U;
                sim_ccp_edges[i] = 0U;
                keep = 0U;
            }
            pic16f188xx_sim_sfr[addr] = (uint8_t)((value & (uint8_t)~PIC_CCP1CON_OUT) | keep);
            return 1U;
        }
    }
    for (i = 0U; i < SIM_PWM_N; i++)
    {
        if (addr == sim_pwm_con[i])
        {
            pic16f188xx_sim_sfr[addr] = (uint8_t)((value & (uint8_t)~PIC_PWM6CON_OUT) |
                (pic16f188xx_sim_sfr[addr] & PIC_PWM6CON_OUT));
            return 1U;
        }
    }
    return 0U;
}

/**
 * @brief Read the modelled output latch of a CCP instance.
 * @param instance Instance 1..5.
 * @return 1 when the output is high, 0 when low or the instance is invalid.
 */
uint8_t pic16f188xx_sim_ccp_out(uint8_t instance)
{
    if ((instance < 1U) || (instance > SIM_CCP_N))
    {
        return 0U;
    }
    return sim_ccp_out_lat[instance - 1U];
}

/**
 * @brief Read the modelled output level of a dedicated PWM module.
 * @param instance Module 6 or 7.
 * @return The pin level after polarity (1 high), 0 for an invalid module.
 */
uint8_t pic16f188xx_sim_pwm_out(uint8_t instance)
{
    if ((instance < 6U) || (instance > 7U))
    {
        return 0U;
    }
    return sim_pwm_pin[instance - 6U];
}
