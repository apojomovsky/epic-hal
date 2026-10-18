/**
 * Host model of the ports, IOC and Timer0..Timer2 (Timer4 lives in
 * pic16f188xx_sim_timer4.c). IOC edges (PORTA..PORTC)
 * set IOCxF and PIR0 IOCIF; callbacks gate on flag, enable, PEIE and GIE.
 * Timer0 counts T0CKIPPS edges on its PPS pin; Timer1 and Timer2 read the
 * POR default PPS pins (RC0, RB5, RC3), Timer2 models every defined
 * T2HLT mode (DS40001802H Table 29-1) and freezes reserved codes.
 * The serial, EEPROM, NCO1 and WDT/Sleep models live in sibling sim files.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "pic16f188xx.h"
#include "pic16f188xx_sim.h"
#include "pic16f188xx_sim_priv.h"
#include "peripherals/pic16f188xx_timer0.h"
#include "peripherals/pic16f188xx_timer1.h"
#include "peripherals/pic16f188xx_timer2.h"

uint8_t pic16f188xx_sim_sfr[0x2000];

static const uint16_t sim_tris[] = {
    PIC_REG_TRISA, PIC_REG_TRISB, PIC_REG_TRISC,
#if PIC16F188XX_FAMILY_HAS_PORTD
    PIC_REG_TRISD,
#endif
#if PIC16F188XX_FAMILY_HAS_PORTE
    PIC_REG_TRISE,
#endif
};
static const uint16_t sim_lat[]  = {
    PIC_REG_LATA,  PIC_REG_LATB,  PIC_REG_LATC,
#if PIC16F188XX_FAMILY_HAS_PORTD
    PIC_REG_LATD,
#endif
#if PIC16F188XX_FAMILY_HAS_PORTE
    PIC_REG_LATE,
#endif
};
static const uint16_t sim_port[] = {
    PIC_REG_PORTA, PIC_REG_PORTB, PIC_REG_PORTC,
#if PIC16F188XX_FAMILY_HAS_PORTD
    PIC_REG_PORTD,
#endif
#if PIC16F188XX_FAMILY_HAS_PORTE
    PIC_REG_PORTE,
#endif
};
static const uint16_t sim_ioc_pos[3] = {PIC_REG_IOCAP, PIC_REG_IOCBP, PIC_REG_IOCCP};
static const uint16_t sim_ioc_neg[3] = {PIC_REG_IOCAN, PIC_REG_IOCBN, PIC_REG_IOCCN};
static const uint16_t sim_ioc_flag[3] = {PIC_REG_IOCAF, PIC_REG_IOCBF, PIC_REG_IOCCF};
static const uint8_t  sim_mask[] = {
    PIC16F188XX_FAMILY_PORTA_MASK,
    PIC16F188XX_FAMILY_PORTB_MASK,
    PIC16F188XX_FAMILY_PORTC_MASK,
#if PIC16F188XX_FAMILY_HAS_PORTD
    PIC16F188XX_FAMILY_PORTD_MASK,
#endif
#if PIC16F188XX_FAMILY_HAS_PORTE
    PIC16F188XX_FAMILY_PORTE_MASK,
#endif
};

/* Latch/level model covers every port; edge detection covers PORTA..PORTC. */
#define SIM_NPORTS (sizeof sim_port / sizeof sim_port[0])

/* Externally driven level per port bit; undriven pins read 0. */
static uint8_t sim_input_value[SIM_NPORTS];
static uint8_t sim_last_port[3];
static pic16f188xx_sim_irq_cb_t sim_irq_cb;

/* Timer0 divider state. Cleared whenever the timer halts, so each Start
 * counts whole ratios from zero and test phases stay independent. */
static uint32_t sim_t0_prescale_acc;
static uint8_t sim_t0_post_acc;
/* T0CKIPPS pin level seen on the previous tick, for edge detection. */
static uint8_t sim_t0cki_last;

/* Timer1 and Timer2 state. Dividers clear whenever their timer halts,
 * like Timer0, so each Start counts whole ratios from zero. The pulse
 * flags carry one-tick events (Timer0 overflow, Timer2 postscaled) to
 * the Timer1 clock and gate selectors within the same tick. */
static uint32_t sim_t1_prescale_acc;
static uint8_t sim_t1_sig_prev;
static uint8_t sim_t1_toggle_ff;
static uint8_t sim_t1_sp_seen;
static uint32_t sim_t2_prescale_acc;
static uint8_t sim_t2_post_acc;
static uint8_t sim_t2_ers_prev;
/* Start latch for the edge, monostable and level modes: set by the start
 * edge or level, cleared at the period match. ON cycling re-arms it, so
 * a stale start never leaks across Start calls. */
static uint8_t sim_t2_run;
/* ON level on the previous tick, to spot the re-arming ON rise. */
static uint8_t sim_t2_on_prev;
static uint8_t sim_t0_pulse;
static uint8_t sim_t2_pulse;
static uint8_t sim_t2_wrap;
#define SIM_PIR6_CCP_MASK 0x1FU

/* The PPS select registers of these inputs are not modelled: the timer
 * input rows read the pins they select at POR (DS40001802H Table 13-1):
 * T1CKIPPS RC0, T1GPPS RB5 and T2INPPS RC3. An output pin reads back its
 * latch, so a test can drive an input through pic16f188xx_sim_drive_input
 * or by making the pin an output. */
#define SIM_PIN_T1CKI() ((uint8_t)(pic16f188xx_sim_sfr[PIC_REG_PORTC] & 0x01U))
#define SIM_PIN_T1G()   ((uint8_t)((pic16f188xx_sim_sfr[PIC_REG_PORTB] >> 5) & 0x01U))
#define SIM_PIN_T2IN()  ((uint8_t)((pic16f188xx_sim_sfr[PIC_REG_PORTC] >> 3) & 0x01U))
static uint8_t sim_t1ck_prev;

/**
 * @brief Test whether the simulated GIE bit is set.
 * @return Non-zero when interrupts can vector, zero otherwise.
 */
static uint8_t sim_gie(void)
{
    return (pic16f188xx_sim_sfr[PIC_REG_INTCON] & PIC_INTCON_GIE) ? 1U : 0U;
}

/**
 * @brief Map a port letter to its table index.
 * @param port 'A'..'C', plus 'D'/'E' on 40-pin parts.
 * @return Table index, or -1 for anything else.
 */
static int port_index(char port)
{
    switch (port)
    {
    case 'A': return 0;
    case 'B': return 1;
    case 'C': return 2;
#if PIC16F188XX_FAMILY_HAS_PORTD
    case 'D': return 3;
#endif
#if PIC16F188XX_FAMILY_HAS_PORTE
    case 'E': return (3 + PIC16F188XX_FAMILY_HAS_PORTD);
#endif
    default:  return -1;
    }
}

/**
 * @brief Recompute PORTx from the latch (output pins) and driven levels (input pins).
 */
static void sim_refresh_ports(void)
{
    unsigned int i;
    for (i = 0; i < SIM_NPORTS; i++)
    {
        uint8_t tris = pic16f188xx_sim_sfr[sim_tris[i]];
        uint8_t lat  = pic16f188xx_sim_sfr[sim_lat[i]];
        uint8_t out  = (uint8_t)(lat & (uint8_t)~tris);
        uint8_t in   = (uint8_t)(sim_input_value[i] & tris);
        pic16f188xx_sim_sfr[sim_port[i]] = (uint8_t)((out | in) & sim_mask[i]);
    }
}

/**
 * @brief Set the IOCxF flags and PIR0.IOCIF on armed edges of every port.
 * @details The callback is level-gated on IOCIF, IOCIE, PEIE, and GIE,
 * matching silicon: a pending flag is taken as soon as GIE allows it.
 */
static void sim_step_ioc(void)
{
    int i;

    for (i = 0; i < 3; i++)
    {
        uint8_t port    = pic16f188xx_sim_sfr[sim_port[i]];
        uint8_t changed = (uint8_t)(port ^ sim_last_port[i]);
        uint8_t rise    = (uint8_t)(changed & port);
        uint8_t fall    = (uint8_t)(changed & (uint8_t)~port);
        uint8_t hits    = (uint8_t)(((rise & pic16f188xx_sim_sfr[sim_ioc_pos[i]]) |
                                    (fall & pic16f188xx_sim_sfr[sim_ioc_neg[i]])) &
                                   sim_mask[i]);

        sim_last_port[i] = port;
        if (hits != 0U)
        {
            pic16f188xx_sim_sfr[sim_ioc_flag[i]] |= hits;
            pic16f188xx_sim_sfr[PIC_REG_PIR0] |= PIC_PIR0_IOCIF;
        }
    }

    if (((pic16f188xx_sim_sfr[PIC_REG_PIR0] & PIC_PIR0_IOCIF) != 0U) &&
        ((pic16f188xx_sim_sfr[PIC_REG_PIE0] & PIC_PIE0_IOCIE) != 0U) &&
        ((pic16f188xx_sim_sfr[PIC_REG_INTCON] & PIC_INTCON_PEIE) != 0U) &&
        (sim_gie() != 0U) &&
        (sim_irq_cb != NULL))
    {
        sim_irq_cb();
    }
}

/**
 * @brief Reset the simulated SFRs to power-up state with all pins as inputs.
 * @details TRIS powers up all-input and ANSEL all-analog (EDC por); WPU and
 * the IOC edge registers power up cleared; TMR0H powers up 0xFF (EDC por).
 */
void pic16f188xx_sim_reset(void)
{
    int i;

    memset(pic16f188xx_sim_sfr, 0, sizeof pic16f188xx_sim_sfr);
    pic16f188xx_sim_sfr[PIC_REG_TRISA]  = PIC16F188XX_FAMILY_PORTA_MASK;
    pic16f188xx_sim_sfr[PIC_REG_TRISB]  = PIC16F188XX_FAMILY_PORTB_MASK;
    pic16f188xx_sim_sfr[PIC_REG_TRISC]  = PIC16F188XX_FAMILY_PORTC_MASK;
#if PIC16F188XX_FAMILY_HAS_PORTD
    pic16f188xx_sim_sfr[PIC_REG_TRISD]  = PIC16F188XX_FAMILY_PORTD_MASK;
#endif
#if PIC16F188XX_FAMILY_HAS_PORTE
    pic16f188xx_sim_sfr[PIC_REG_TRISE]  = PIC16F188XX_FAMILY_PORTE_MASK;
#endif
    pic16f188xx_sim_sfr[PIC_REG_ANSELA] = PIC16F188XX_FAMILY_PORTA_MASK;
    pic16f188xx_sim_sfr[PIC_REG_ANSELB] = PIC16F188XX_FAMILY_PORTB_MASK;
    pic16f188xx_sim_sfr[PIC_REG_ANSELC] = PIC16F188XX_FAMILY_PORTC_MASK;
#if PIC16F188XX_FAMILY_HAS_PORTD
    pic16f188xx_sim_sfr[PIC_REG_ANSELD] = PIC16F188XX_FAMILY_PORTD_MASK;
#endif
#if PIC16F188XX_FAMILY_HAS_PORTE
    pic16f188xx_sim_sfr[PIC_REG_ANSELE] = PIC16F188XX_FAMILY_PORTE_MASK;
#endif
    memset(sim_input_value, 0, sizeof sim_input_value);
    sim_irq_cb = NULL;
    sim_t0_prescale_acc = 0U;
    sim_t0_post_acc = 0U;
    sim_t0cki_last = 0U;
    sim_t1_prescale_acc = 0U;
    sim_t1_sig_prev = 0U;
    sim_t1_toggle_ff = 0U;
    sim_t1_sp_seen = 0U;
    sim_t2_prescale_acc = 0U;
    sim_t2_post_acc = 0U;
    sim_t2_ers_prev = 0U;
    sim_t2_run = 0U;
    sim_t2_on_prev = 0U;
    sim_t0_pulse = 0U;
    sim_t2_pulse = 0U;
    sim_t2_wrap = 0U;
    sim_t1ck_prev = 0U;
    pic16f188xx_sim_sfr[PIC_REG_TMR0H] = 0xFFU;
    pic16f188xx_sim_sfr[PIC_REG_PR2] = 0xFFU;
    pic16f188xx_sim_serial_reset();
    pic16f188xx_sim_sys_reset();
    pic16f188xx_sim_ccp_reset();
    pic16f188xx_sim_timer4_reset();
    sim_refresh_ports();
    for (i = 0; i < 3; i++)
    {
        sim_last_port[i] = pic16f188xx_sim_sfr[sim_port[i]];
    }
}

/**
 * @brief Fold one postscaler event: count it, and on terminal count set
 * TMR0IF and toggle T0OUT (TMR0_out toggles the output per postscaled
 * match/rollover, DS40001825F §27.7).
 */
static void sim_t0_post_event(void)
{
    uint8_t outps = (uint8_t)(pic16f188xx_sim_sfr[PIC_REG_T0CON0] &
        PIC_T0CON0_T0OUTPS_MASK);

    sim_t0_post_acc++;
    if (sim_t0_post_acc > outps)
    {
        sim_t0_post_acc = 0U;
        pic16f188xx_sim_sfr[PIC_REG_PIR0] |= PIC_PIR0_TMR0IF;
        pic16f188xx_sim_sfr[PIC_REG_T0CON0] ^= PIC_T0CON0_T0OUT;
        sim_t0_pulse = 1U;
    }
}

/**
 * @brief Advance Timer0 by one instruction tick.
 * @details The internal sources (FOSC/4, HFINTOSC, LFINTOSC, SOSC) tick
 * every cycle. T0CKIPPS counts a rising edge on the pin T0CKIPPS selects
 * (PORTA or PORTB), the inverted row a falling edge; the pin level is
 * tracked even while the timer is halted, so enabling it mid-level does
 * not count an edge. The LC1 row has no stimulus model and freezes the
 * counter. 8-bit mode matches TMR0L against TMR0H and resets on match;
 * 16-bit mode rolls TMR0H:TMR0L over. Divider state clears while halted,
 * so a stopped timer restarts from whole ratios.
 */
static void sim_step_timer0(void)
{
    uint8_t con0;
    uint8_t con1;
    uint8_t cs;
    uint8_t sel = pic16f188xx_sim_sfr[PIC_REG_T0CKIPPS];
    uint8_t level = (uint8_t)((pic16f188xx_sim_sfr[sim_port[(sel >> 3) & 1U]] >>
        (sel & 7U)) & 1U);
    uint8_t rise = (uint8_t)((sim_t0cki_last == 0U) && (level != 0U));
    uint8_t fall = (uint8_t)((sim_t0cki_last != 0U) && (level == 0U));

    sim_t0cki_last = level;
    con0 = pic16f188xx_sim_sfr[PIC_REG_T0CON0];
    if ((con0 & PIC_T0CON0_T0EN) == 0U)
    {
        sim_t0_prescale_acc = 0U;
        sim_t0_post_acc = 0U;
        return;
    }
    con1 = pic16f188xx_sim_sfr[PIC_REG_T0CON1];
    cs = (uint8_t)(((unsigned int)con1 & PIC_T0CON1_T0CS_MASK) >>
        PIC_T0CON1_T0CS_SHIFT);
    if (cs == (uint8_t)TIMER0_CS_T0CKIPPS)
    {
        if (rise == 0U)
        {
            return;
        }
    }
    else if (cs == (uint8_t)TIMER0_CS_T0CKIPPS_INV)
    {
        if (fall == 0U)
        {
            return;
        }
    }
    else if ((cs != (uint8_t)TIMER0_CS_FOSC4) && (cs != (uint8_t)TIMER0_CS_HFINTOSC) &&
        (cs != (uint8_t)TIMER0_CS_LFINTOSC) && (cs != (uint8_t)TIMER0_CS_SOSC))
    {
        return;
    }
    sim_t0_prescale_acc++;
    if (sim_t0_prescale_acc < (1UL << (con1 & PIC_T0CON1_T0CKPS_MASK)))
    {
        return;
    }
    sim_t0_prescale_acc = 0U;
    if ((con0 & PIC_T0CON0_T016BIT) != 0U)
    {
        uint16_t ctr = (uint16_t)(((uint16_t)pic16f188xx_sim_sfr[PIC_REG_TMR0H] << 8) |
            pic16f188xx_sim_sfr[PIC_REG_TMR0L]);
        ctr++;
        if (ctr == 0U)
        {
            sim_t0_post_event();
        }
        pic16f188xx_sim_sfr[PIC_REG_TMR0L] = (uint8_t)ctr;
        pic16f188xx_sim_sfr[PIC_REG_TMR0H] = (uint8_t)(ctr >> 8);
    }
    else
    {
        uint16_t next = (uint16_t)pic16f188xx_sim_sfr[PIC_REG_TMR0L] + 1U;
        if (next > pic16f188xx_sim_sfr[PIC_REG_TMR0H])
        {
            pic16f188xx_sim_sfr[PIC_REG_TMR0L] = 0U;
            sim_t0_post_event();
        }
        else
        {
            pic16f188xx_sim_sfr[PIC_REG_TMR0L] = (uint8_t)next;
        }
    }
}

/**
 * @brief Advance Timer2 by one instruction tick.
 * @details Internal sources only (FOSC counts four clocks per tick). A
 * match clears TMR2 and steps the postscaler; its terminal count sets
 * TMR2IF and pulses TMR2_postscaled. Every defined T2HLT mode runs
 * against RC3 (DS40001802H Table 29-1): edge-start waits for its edge,
 * monostable restarts on it with ON kept, level modes follow ers.
 * Reserved codes freeze the counter.
 */
static void sim_step_timer2(void)
{
    uint8_t con = pic16f188xx_sim_sfr[PIC_REG_T2CON];
    uint8_t mode = (uint8_t)(pic16f188xx_sim_sfr[PIC_REG_T2HLT] & PIC_T2HLT_MODE_MASK);
    uint8_t cs = (uint8_t)(pic16f188xx_sim_sfr[PIC_REG_T2CLKCON] & PIC_T2CLKCON_CS_MASK);
    uint8_t ers = 0U;
    uint8_t prev = sim_t2_ers_prev;
    uint8_t rise;
    uint8_t fall;
    uint8_t any;
    uint8_t enable = 1U;
    uint8_t reset = 0U;
    uint8_t clocks = 1U;
    uint8_t outps = (uint8_t)(con & PIC_T2CON_OUTPS_MASK);
    uint32_t ratio = 1UL << ((con & PIC_T2CON_CKPS_MASK) >> PIC_T2CON_CKPS_SHIFT);

    if ((pic16f188xx_sim_sfr[PIC_REG_T2RST] & PIC_T2RST_RSEL_MASK) ==
        (uint8_t)TIMER2_RST_T2INPPS)
    {
        ers = SIM_PIN_T2IN();
    }
    sim_t2_ers_prev = ers;
    rise = (uint8_t)((ers != 0U) && (prev == 0U));
    fall = (uint8_t)((ers == 0U) && (prev != 0U));
    any = (uint8_t)(ers != prev);
    if ((con & PIC_T2CON_ON) == 0U)
    {
        sim_t2_prescale_acc = 0U;
        sim_t2_post_acc = 0U;
        sim_t2_run = 0U;
        sim_t2_on_prev = 0U;
        return;
    }
    if (sim_t2_on_prev == 0U)
    {
        /* Fresh arm: a start edge or level is still owed, so a stale
         * run never leaks across Start calls (Table 29-1 Note 1). */
        sim_t2_run = 0U;
    }
    sim_t2_on_prev = 1U;
    switch (mode)
    {
    case TIMER2_MODE_FREE_SOFTWARE:
    case TIMER2_MODE_ONESHOT_SOFTWARE:
        break;
    case TIMER2_MODE_FREE_GATE_HIGH:
        enable = ers;
        break;
    case TIMER2_MODE_FREE_GATE_LOW:
        enable = (uint8_t)(ers == 0U);
        break;
    case TIMER2_MODE_FREE_RESET_ANY_EDGE:
        reset = (uint8_t)(ers != prev);
        break;
    case TIMER2_MODE_FREE_RESET_RISING:
        reset = (uint8_t)((ers != 0U) && (prev == 0U));
        break;
    case TIMER2_MODE_FREE_RESET_FALLING:
        reset = (uint8_t)((ers == 0U) && (prev != 0U));
        break;
    case TIMER2_MODE_FREE_RESET_LOW:
        reset = (uint8_t)(ers == 0U);
        enable = ers;
        break;
    case TIMER2_MODE_FREE_RESET_HIGH:
        reset = ers;
        enable = (uint8_t)(ers == 0U);
        break;
    case TIMER2_MODE_ONESHOT_RISING:
        /* No reset row: an edge only starts an idle timer, one mid-run
         * is ignored (Table 29-1 Reset is blank). */
        if ((rise != 0U) && (sim_t2_run == 0U))
        {
            sim_t2_run = 1U;
            reset = 1U;
        }
        enable = sim_t2_run;
        break;
    case TIMER2_MODE_ONESHOT_FALLING:
        if ((fall != 0U) && (sim_t2_run == 0U))
        {
            sim_t2_run = 1U;
            reset = 1U;
        }
        enable = sim_t2_run;
        break;
    case TIMER2_MODE_ONESHOT_ANY_EDGE:
        if ((any != 0U) && (sim_t2_run == 0U))
        {
            sim_t2_run = 1U;
            reset = 1U;
        }
        enable = sim_t2_run;
        break;
    case TIMER2_MODE_ONESHOT_RISE_RST_RISE:
        /* The start edge doubles as the running reset. */
        if (rise != 0U)
        {
            sim_t2_run = 1U;
            reset = 1U;
        }
        enable = sim_t2_run;
        break;
    case TIMER2_MODE_ONESHOT_FALL_RST_FALL:
        if (fall != 0U)
        {
            sim_t2_run = 1U;
            reset = 1U;
        }
        enable = sim_t2_run;
        break;
    case TIMER2_MODE_ONESHOT_RISE_RST_LOW:
        /* Edge starts, opposite level holds in reset while running. */
        if ((rise != 0U) && (sim_t2_run == 0U))
        {
            sim_t2_run = 1U;
        }
        if (ers == 0U)
        {
            reset = 1U;
        }
        enable = (uint8_t)((ers != 0U) && (sim_t2_run != 0U));
        break;
    case TIMER2_MODE_ONESHOT_FALL_RST_HIGH:
        if ((fall != 0U) && (sim_t2_run == 0U))
        {
            sim_t2_run = 1U;
        }
        if (ers != 0U)
        {
            reset = 1U;
        }
        enable = (uint8_t)((ers == 0U) && (sim_t2_run != 0U));
        break;
    case TIMER2_MODE_MONO_RISING:
        /* Retriggerable: every edge restarts from zero, ON stays set. */
        if (rise != 0U)
        {
            sim_t2_run = 1U;
            reset = 1U;
        }
        enable = sim_t2_run;
        break;
    case TIMER2_MODE_MONO_FALLING:
        if (fall != 0U)
        {
            sim_t2_run = 1U;
            reset = 1U;
        }
        enable = sim_t2_run;
        break;
    case TIMER2_MODE_MONO_ANY_EDGE:
        if (any != 0U)
        {
            sim_t2_run = 1U;
            reset = 1U;
        }
        enable = sim_t2_run;
        break;
    case TIMER2_MODE_ONESHOT_HIGH_START_LOW_RST:
        /* Level start: the active level runs, the other holds in reset. */
        if (ers == 0U)
        {
            sim_t2_run = 0U;
            reset = 1U;
        }
        else
        {
            sim_t2_run = 1U;
        }
        enable = sim_t2_run;
        break;
    case TIMER2_MODE_ONESHOT_LOW_START_HIGH_RST:
        if (ers != 0U)
        {
            sim_t2_run = 0U;
            reset = 1U;
        }
        else
        {
            sim_t2_run = 1U;
        }
        enable = sim_t2_run;
        break;
    default:
        return;
    }
    if (reset != 0U)
    {
        pic16f188xx_sim_sfr[PIC_REG_TMR2] = 0U;
        sim_t2_prescale_acc = 0U;
        sim_t2_post_acc = 0U;
    }
    if (enable == 0U)
    {
        return;
    }
    if (cs == (uint8_t)TIMER2_CS_FOSC)
    {
        clocks = 4U;
    }
    else if ((cs != (uint8_t)TIMER2_CS_FOSC4) && (cs != (uint8_t)TIMER2_CS_HFINTOSC) &&
             (cs != (uint8_t)TIMER2_CS_LFINTOSC) && (cs != (uint8_t)TIMER2_CS_MFINTOSC16) &&
             (cs != (uint8_t)TIMER2_CS_SOSC))
    {
        return;
    }
    while (clocks != 0U)
    {
        clocks--;
        sim_t2_prescale_acc++;
        if (sim_t2_prescale_acc < ratio)
        {
            continue;
        }
        sim_t2_prescale_acc = 0U;
        if (pic16f188xx_sim_sfr[PIC_REG_TMR2] != pic16f188xx_sim_sfr[PIC_REG_PR2])
        {
            pic16f188xx_sim_sfr[PIC_REG_TMR2]++;
            continue;
        }
        pic16f188xx_sim_sfr[PIC_REG_TMR2] = 0U;
        sim_t2_wrap = 1U;
        sim_t2_post_acc++;
        if (sim_t2_post_acc > outps)
        {
            sim_t2_post_acc = 0U;
            pic16f188xx_sim_sfr[PIC_REG_PIR4] |= PIC_PIR4_TMR2IF;
            sim_t2_pulse = 1U;
        }
        if (((mode >= (uint8_t)TIMER2_MODE_ONESHOT_SOFTWARE) &&
             (mode <= (uint8_t)TIMER2_MODE_ONESHOT_FALL_RST_HIGH)) ||
            (mode == (uint8_t)TIMER2_MODE_ONESHOT_HIGH_START_LOW_RST) ||
            (mode == (uint8_t)TIMER2_MODE_ONESHOT_LOW_START_HIGH_RST))
        {
            /* Note 2: the match clears ON and stops TMR2 at 00h. */
            pic16f188xx_sim_sfr[PIC_REG_T2CON] &= (uint8_t)~PIC_T2CON_ON;
            sim_t2_run = 0U;
            sim_t2_prescale_acc = 0U;
            sim_t2_post_acc = 0U;
            return;
        }
        if ((mode >= (uint8_t)TIMER2_MODE_MONO_RISING) &&
            (mode <= (uint8_t)TIMER2_MODE_MONO_ANY_EDGE))
        {
            /* Note 3: the match stops TMR2 at 00h with ON kept, and the
             * next start edge runs again. */
            sim_t2_run = 0U;
            sim_t2_prescale_acc = 0U;
            sim_t2_post_acc = 0U;
            return;
        }
    }
}

/**
 * @brief Feed one post-selector clock event through Timer1's prescaler
 * and counter, setting TMR1IF on the FFFFh to 0000h rollover.
 */
static void sim_t1_clock_event(void)
{
    uint32_t ratio = 1UL << ((pic16f188xx_sim_sfr[PIC_REG_T1CON] & PIC_T1CON_CKPS_MASK) >>
        PIC_T1CON_CKPS_SHIFT);
    uint16_t ctr;

    sim_t1_prescale_acc++;
    if (sim_t1_prescale_acc < ratio)
    {
        return;
    }
    sim_t1_prescale_acc = 0U;
    ctr = (uint16_t)(((uint16_t)pic16f188xx_sim_sfr[PIC_REG_TMR1H] << 8) |
        pic16f188xx_sim_sfr[PIC_REG_TMR1L]);
    ctr++;
    if (ctr == 0U)
    {
        pic16f188xx_sim_sfr[PIC_REG_PIR4] |= PIC_PIR4_TMR1IF;
    }
    pic16f188xx_sim_sfr[PIC_REG_TMR1L] = (uint8_t)ctr;
    pic16f188xx_sim_sfr[PIC_REG_TMR1H] = (uint8_t)(ctr >> 8);
}

/**
 * @brief Evaluate the Timer1 gate for this tick and publish GVAL.
 * @details The selected signal is the driven T1G pin, the Timer0
 * overflow pulse or the Timer2 postscaled pulse; every other source row
 * reads low. GPOL picks the active level, GTM turns rising edges into a
 * toggled level (cleared while the timer is off), and GSPM lets one
 * pulse through per GGO/DONE arm: GGO clears on the trailing edge, and
 * clears at once when GSPM is clear.
 * @param on Non-zero while T1CON.ON is set.
 * @return Gate level that would let the timer count (the GVAL value).
 */
static uint8_t sim_t1_gate(uint8_t on)
{
    uint8_t gcon = pic16f188xx_sim_sfr[PIC_REG_T1GCON];
    uint8_t raw = 0U;
    uint8_t sig;
    uint8_t gate;

    switch (pic16f188xx_sim_sfr[PIC_REG_T1GATE] & PIC_T1GATE_GSS_MASK)
    {
    case TIMER1_GATE_T1GPPS:    raw = SIM_PIN_T1G(); break;
    case TIMER1_GATE_TMR0_OVF:  raw = sim_t0_pulse; break;
    case TIMER1_GATE_TMR2_POST: raw = sim_t2_pulse; break;
    default:                    break;
    }
    sig = ((gcon & PIC_T1GCON_GPOL) != 0U) ? raw : (uint8_t)(raw == 0U);
    if (on == 0U)
    {
        sim_t1_toggle_ff = 0U;
    }
    if (((gcon & PIC_T1GCON_GTM) != 0U) && (on != 0U))
    {
        if ((sig != 0U) && (sim_t1_sig_prev == 0U))
        {
            sim_t1_toggle_ff ^= 1U;
        }
        gate = sim_t1_toggle_ff;
    }
    else
    {
        sim_t1_toggle_ff = 0U;
        gate = sig;
    }
    sim_t1_sig_prev = sig;
    if ((gcon & PIC_T1GCON_GSPM) == 0U)
    {
        gcon &= (uint8_t)~PIC_T1GCON_GGO_nDONE;
        sim_t1_sp_seen = 0U;
    }
    else if ((gcon & PIC_T1GCON_GGO_nDONE) == 0U)
    {
        sim_t1_sp_seen = 0U;
        gate = 0U;
    }
    else if (gate != 0U)
    {
        sim_t1_sp_seen = 1U;
    }
    else if (sim_t1_sp_seen != 0U)
    {
        gcon &= (uint8_t)~PIC_T1GCON_GGO_nDONE;
        sim_t1_sp_seen = 0U;
    }
    else
    {
        gate = 0U;
    }
    if (gate != 0U)
    {
        gcon |= PIC_T1GCON_GVAL;
    }
    else
    {
        gcon &= (uint8_t)~PIC_T1GCON_GVAL;
    }
    pic16f188xx_sim_sfr[PIC_REG_T1GCON] = gcon;
    return gate;
}

/**
 * @brief Advance Timer1 by one instruction tick.
 * @details Clock events come from FOSC/4, FOSC (four per tick),
 * HFINTOSC, LFINTOSC, MFINTOSC and SOSC every tick, from each Timer0
 * overflow pulse, and from each rising edge of the driven T1CKI pin;
 * every other source row freezes the counter. Counting needs ON, and
 * GE adds the gate. The prescaler clears while halted or gated off.
 */
static void sim_step_timer1(void)
{
    uint8_t on = (uint8_t)((pic16f188xx_sim_sfr[PIC_REG_T1CON] & PIC_T1CON_ON) != 0U);
    uint8_t gate = sim_t1_gate(on);
    uint8_t gate_en = (uint8_t)((pic16f188xx_sim_sfr[PIC_REG_T1GCON] & PIC_T1GCON_GE) != 0U);
    uint8_t ck = SIM_PIN_T1CKI();
    uint8_t events = 0U;

    switch (pic16f188xx_sim_sfr[PIC_REG_T1CLK] & PIC_T1CLK_CS_MASK)
    {
    case TIMER1_CS_FOSC4:
    case TIMER1_CS_HFINTOSC:
    case TIMER1_CS_LFINTOSC:
    case TIMER1_CS_MFINTOSC:
    case TIMER1_CS_SOSC:
        events = 1U;
        break;
    case TIMER1_CS_FOSC:
        events = 4U;
        break;
    case TIMER1_CS_TMR0_OVF:
        events = sim_t0_pulse;
        break;
    case TIMER1_CS_T1CKIPPS:
        events = (uint8_t)((ck != 0U) && (sim_t1ck_prev == 0U));
        break;
    default:
        break;
    }
    sim_t1ck_prev = ck;
    if (on == 0U)
    {
        sim_t1_prescale_acc = 0U;
        return;
    }
    if ((gate_en != 0U) && (gate == 0U))
    {
        return;
    }
    while (events != 0U)
    {
        events--;
        sim_t1_clock_event();
    }
}

/**
 * @brief Advance the CCP and PWM models for this tick.
 * @details Hands them the PWM time bases: for Timer2 whether TMR2 matched
 * PR2 this tick, whether it runs from FOSC/4 (the only clock PWM accepts),
 * and the 10-bit time base, TMR2 x 4 plus the prescaler position scaled to
 * quarter steps; Timer4 supplies the same view.
 */
static void sim_step_ccp(void)
{
    pic16f188xx_sim_pwm_base_t base[2];
    uint8_t con = pic16f188xx_sim_sfr[PIC_REG_T2CON];
    uint32_t ratio = 1UL << ((con & PIC_T2CON_CKPS_MASK) >> PIC_T2CON_CKPS_SHIFT);

    base[0].wrap = sim_t2_wrap;
    base[0].live = (uint8_t)(((con & PIC_T2CON_ON) != 0U) &&
        ((pic16f188xx_sim_sfr[PIC_REG_T2CLKCON] & PIC_T2CLKCON_CS_MASK) ==
         (uint8_t)TIMER2_CS_FOSC4));
    base[0].pos = (uint16_t)(((uint32_t)pic16f188xx_sim_sfr[PIC_REG_TMR2] * 4UL) +
        ((sim_t2_prescale_acc * 4UL) / ratio));
    pic16f188xx_sim_timer4_base(&base[1]);
    pic16f188xx_sim_ccp_step(base);
}

/**
 * @brief Deliver a pending timer interrupt to the registered callback.
 * @details Gated on a Timer0, Timer1, Timer2 or Timer4 flag with its enable,
 * plus PEIE and GIE, matching silicon: a pending flag is taken as soon
 * as GIE allows it. One call per tick: the callback is the dispatcher,
 * which routes every pending enabled source.
 */
static void sim_deliver_timers(void)
{
    uint8_t pending = (uint8_t)((pic16f188xx_sim_sfr[PIC_REG_PIR0] &
                                 pic16f188xx_sim_sfr[PIC_REG_PIE0] & PIC_PIR0_TMR0IF) |
                                (pic16f188xx_sim_sfr[PIC_REG_PIR4] &
                                 pic16f188xx_sim_sfr[PIC_REG_PIE4] &
                                 (PIC_PIR4_TMR1IF | PIC_PIR4_TMR2IF | PIC_PIR4_TMR4IF)) |
                                (pic16f188xx_sim_sfr[PIC_REG_PIR6] &
                                 pic16f188xx_sim_sfr[PIC_REG_PIE6] & SIM_PIR6_CCP_MASK));

    if ((pending != 0U) &&
        ((pic16f188xx_sim_sfr[PIC_REG_INTCON] & PIC_INTCON_PEIE) != 0U) &&
        (sim_gie() != 0U) &&
        (sim_irq_cb != NULL))
    {
        sim_irq_cb();
    }
}

/**
 * @brief Deliver a pending EUSART or MSSP1 interrupt to the registered callback.
 * @details Gated on a PIR3 flag with its PIE3 enable (SSP1, TX or RX),
 * PEIE and GIE, matching silicon. TXIF stays set while the buffer is empty,
 * so a TX callback that does not mask TXIE is re-entered every tick.
 */
static void sim_deliver_serial(void)
{
    uint8_t pending = (uint8_t)(pic16f188xx_sim_sfr[PIC_REG_PIR3] &
        pic16f188xx_sim_sfr[PIC_REG_PIE3] &
        (uint8_t)(PIC_PIR3_SSP1IF | PIC_PIR3_TXIF | PIC_PIR3_RCIF));

    if ((pending != 0U) &&
        ((pic16f188xx_sim_sfr[PIC_REG_INTCON] & PIC_INTCON_PEIE) != 0U) &&
        (sim_gie() != 0U) &&
        (sim_irq_cb != NULL))
    {
        sim_irq_cb();
    }
}

/**
 * @brief Deliver a pending NVM or NCO1 interrupt to the registered callback.
 * @details Gated on a PIR7 flag with its PIE7 enable (NVMIF, NCO1IF), PEIE
 * and GIE, matching silicon.
 */
static void sim_deliver_sys(void)
{
    uint8_t pending = (uint8_t)(pic16f188xx_sim_sfr[PIC_REG_PIR7] &
        pic16f188xx_sim_sfr[PIC_REG_PIE7] &
        (uint8_t)(PIC_PIR7_NVMIF | PIC_PIR7_NCO1IF));

    if ((pending != 0U) &&
        ((pic16f188xx_sim_sfr[PIC_REG_INTCON] & PIC_INTCON_PEIE) != 0U) &&
        (sim_gie() != 0U) &&
        (sim_irq_cb != NULL))
    {
        sim_irq_cb();
    }
}

/**
 * @brief Sample pins, IOC, the timers, the EUSART, MSSP1, the EEPROM and NCO1
 *        once per instruction tick.
 * @param ticks Instruction ticks to advance (each ticks every model once).
 */
void pic16f188xx_sim_step(uint32_t ticks)
{
    uint32_t i;

    for (i = 0U; i < ticks; i++)
    {
        sim_refresh_ports();
        sim_step_ioc();
        sim_step_timer0();
        sim_step_timer2();
        sim_step_timer1();
        pic16f188xx_sim_timer4_step();
        sim_step_ccp();
        sim_t0_pulse = 0U;
        sim_t2_pulse = 0U;
        sim_t2_wrap = 0U;
        pic16f188xx_sim_serial_step();
        pic16f188xx_sim_sys_step();
        sim_deliver_timers();
        sim_deliver_serial();
        sim_deliver_sys();
    }
}

/**
 * @brief Set the external level presented on an input pin.
 * @param port Port letter, 'A'..'C' ('D'/'E' on 40-pin parts). Invalid ports are ignored.
 * @param pin Pin number 0..7. Values above 7 are ignored.
 * @param level Nonzero drives high, zero drives low.
 */
void pic16f188xx_sim_drive_input(char port, uint8_t pin, uint8_t level)
{
    int i = port_index(port);
    uint8_t bit;

    if ((i < 0) || (pin > 7U))
    {
        return;
    }

    bit = (uint8_t)(1U << pin);
    if (level != 0U)
    {
        sim_input_value[i] |= bit;
    }
    else
    {
        sim_input_value[i] &= (uint8_t)~bit;
    }
}

/**
 * @brief Read the simulated output level of one pin.
 * @param port Port letter, 'A'..'C' ('D'/'E' on 40-pin parts).
 * @param pin Pin number 0..7.
 * @return 1 when the pin drives high, 0 when low or invalid.
 */
uint8_t pic16f188xx_sim_read_output(char port, uint8_t pin)
{
    int i = port_index(port);

    if ((i < 0) || (pin > 7U))
    {
        return 0U;
    }

    sim_refresh_ports();
    return (uint8_t)((pic16f188xx_sim_sfr[sim_port[i]] >> pin) & 1U);
}

/**
 * @brief Register the callback run when IOCIF, IOCIE, PEIE and GIE are all set.
 * @param cb Callback, or NULL to disable delivery.
 */
void pic16f188xx_sim_set_irq_callback(pic16f188xx_sim_irq_cb_t cb)
{
    sim_irq_cb = cb;
}
