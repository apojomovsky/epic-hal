/**
 * Host model of the ports, interrupt-on-change and Timer0. IOC edge
 * masks set IOCxF and PIR0 IOCIF; the callback gates on flag, enable,
 * PEIE and GIE. Timer0 advances per tick with prescaler, 8/16-bit
 * counting, postscaler and T0OUT modelled; only internal sources tick.
 * Edge detection covers PORTA..PORTC; PORTD/PORTE join the level model.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "pic16f188xx.h"
#include "pic16f188xx_sim.h"
#include "peripherals/pic16f188xx_timer0.h"

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
    pic16f188xx_sim_sfr[PIC_REG_TMR0H] = 0xFFU;
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
    }
}

/**
 * @brief Advance Timer0 by one instruction tick.
 * @details Counts only for the internal sources (FOSC/4, HFINTOSC,
 * LFINTOSC, SOSC); the T0CKIPPS and LC1 rows have no stimulus model and
 * freeze the counter. 8-bit mode matches TMR0L against TMR0H and resets
 * on match; 16-bit mode rolls TMR0H:TMR0L over. Divider state clears
 * while halted, so a stopped timer restarts from whole ratios.
 */
static void sim_step_timer0(void)
{
    uint8_t con0;
    uint8_t con1;
    uint8_t cs;

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
    if ((cs != (uint8_t)TIMER0_CS_FOSC4) && (cs != (uint8_t)TIMER0_CS_HFINTOSC) &&
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
 * @brief Deliver a pending Timer0 interrupt to the registered callback.
 * @details Gated on TMR0IF, TMR0IE, PEIE and GIE, matching silicon: a
 * pending flag is taken as soon as GIE allows it.
 */
static void sim_deliver_timer0(void)
{
    if (((pic16f188xx_sim_sfr[PIC_REG_PIR0] & PIC_PIR0_TMR0IF) != 0U) &&
        ((pic16f188xx_sim_sfr[PIC_REG_PIE0] & PIC_PIE0_TMR0IE) != 0U) &&
        ((pic16f188xx_sim_sfr[PIC_REG_INTCON] & PIC_INTCON_PEIE) != 0U) &&
        (sim_gie() != 0U) &&
        (sim_irq_cb != NULL))
    {
        sim_irq_cb();
    }
}

/**
 * @brief Sample pins, IOC and Timer0 once per instruction tick.
 * @param ticks Instruction ticks to advance (each ticks Timer0 once).
 */
void pic16f188xx_sim_step(uint32_t ticks)
{
    uint32_t i;

    for (i = 0U; i < ticks; i++)
    {
        sim_refresh_ports();
        sim_step_ioc();
        sim_step_timer0();
        sim_deliver_timer0();
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
