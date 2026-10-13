/**
 * Host model of the PIC16F1508/1509 ports, interrupt-on-change, and CLC cells.
 * Models only what the GPIO/IOC/CLC contracts observe: no timers, USART,
 * EEPROM, or ADC, and no time base (one pin sample per step call).
 */
#include <stddef.h>
#include <string.h>
#include "pic16f1508.h"
#include "pic16f1508_sim.h"

uint8_t pic16f1508_sim_sfr[0x1000];

static const uint16_t sim_tris[3] = {PIC_REG_TRISA, PIC_REG_TRISB, PIC_REG_TRISC};
static const uint16_t sim_lat[3]  = {PIC_REG_LATA,  PIC_REG_LATB,  PIC_REG_LATC};
static const uint16_t sim_port[3] = {PIC_REG_PORTA, PIC_REG_PORTB, PIC_REG_PORTC};
static const uint8_t  sim_mask[3] = {PIC16F1508_FAMILY_PORTA_MASK,
                                     PIC16F1508_FAMILY_PORTB_MASK,
                                     PIC16F1508_FAMILY_PORTC_MASK};

/* Externally driven level per port bit; undriven pins read 0. */
static uint8_t sim_input_value[3];
static uint8_t sim_last_portb;
static pic16f1508_sim_irq_cb_t sim_irq_cb;
/* Injected D1..D4 levels per cell in bits 3:0. SEL codes are stored but not
 * decoded (the model has no signal sources behind them), so tests drive Dx here. */
static uint8_t sim_clc_d[4];
/* Sequential bit, last gate-1 level for clock edges, and last output for INTN/INTP. */
static uint8_t sim_clc_q[4];
static uint8_t sim_clc_g1[4];
static uint8_t sim_clc_out[4];

/* Cell register block stride; matches the driver layout (CON, POL, SEL0, SEL1, GLS0..3). */
#define SIM_CLC_STRIDE 8U

/**
 * @brief Evaluate one gate from its GLS image and the injected Dx levels.
 * @param gls GLS register image; each input owns an N bit then a T bit.
 * @param d Injected D1..D4 levels in bits 3:0.
 * @param pol Gate polarity bit: inverted output when set.
 * @return Gate output after polarity.
 */
static uint8_t sim_clc_gate(uint8_t gls, uint8_t d, uint8_t pol)
{
    uint8_t g = 0U;
    uint8_t i;

    for (i = 0U; i < 4U; i++)
    {
        uint8_t dv = (uint8_t)((d >> i) & 1U);
        uint8_t t = (uint8_t)((gls >> (uint8_t)(i * 2U + 1U)) & 1U);
        uint8_t n = (uint8_t)((gls >> (uint8_t)(i * 2U)) & 1U);

        /* Dual select reads back zero before polarity; the part may glitch. */
        if ((t != 0U) && (n != 0U))
        {
            return pol;
        }
        if (((t != 0U) && (dv != 0U)) || ((n != 0U) && (dv == 0U)))
        {
            g = 1U;
        }
    }
    return (uint8_t)(g ^ pol);
}

/**
 * @brief Run one cell's mode logic over its four gate outputs.
 * @details Gate roles are DS40001609E Figure 24-3 (page 262): S-R latch S=G1|G2
 * and R=G3|G4; flip-flops clock on the G1 rise with D=G2 and R=G3, S=G4 on the
 * 1-input D, D=G2|G4 on the 2-input D, K=G4 on the J-K (J=G2); transparent
 * latch D=G2, LE=G3, S=G4, R=G1. The figure draws AND gates in front of the
 * OR-XOR output while Register 24-1 and section 24.1.3 name it OR-XOR; the
 * model follows the name. The datasheet gives no priority between S and R, so S
 * wins here by assumption.
 * @param n Cell index 0..3; carries the sequential and edge state.
 * @param mode LCxMODE value 0..7.
 * @param g0 Gate outputs G1..G4 packed in bits 3:0.
 * @return Raw cell output before the POL bit.
 */
static uint8_t sim_clc_mode(uint8_t n, uint8_t mode, uint8_t g0)
{
    uint8_t g1 = (uint8_t)(g0 & 1U);
    uint8_t g2 = (uint8_t)((g0 >> 1U) & 1U);
    uint8_t g3 = (uint8_t)((g0 >> 2U) & 1U);
    uint8_t g4 = (uint8_t)((g0 >> 3U) & 1U);
    uint8_t rise = (uint8_t)(g1 & (uint8_t)~sim_clc_g1[n]);
    uint8_t q = sim_clc_q[n];

    sim_clc_g1[n] = g1;
    switch (mode)
    {
    case 0U:
        q = (uint8_t)(((g1 & g2) | (g3 & g4)) & 1U);
        break;
    case 1U:
        q = (uint8_t)(((g1 | g2) ^ (g3 | g4)) & 1U);
        break;
    case 2U:
        q = (uint8_t)(g1 & g2 & g3 & g4);
        break;
    case 3U:
        q = (uint8_t)(g1 | g2 | ((uint8_t)~(g3 | g4) & q));
        break;
    case 4U:
        if (g4 != 0U)
        {
            q = 1U;
        }
        else if (g3 != 0U)
        {
            q = 0U;
        }
        else if (rise != 0U)
        {
            q = g2;
        }
        break;
    case 5U:
        if (g3 != 0U)
        {
            q = 0U;
        }
        else if (rise != 0U)
        {
            q = (uint8_t)(g2 | g4);
        }
        break;
    case 6U:
        if (g3 != 0U)
        {
            q = 0U;
        }
        else if (rise != 0U)
        {
            if ((g2 != 0U) && (g4 == 0U))
            {
                q = 1U;
            }
            else if ((g2 == 0U) && (g4 != 0U))
            {
                q = 0U;
            }
            else if ((g2 != 0U) && (g4 != 0U))
            {
                q = (uint8_t)~q;
            }
        }
        break;
    case 7U:
        if (g4 != 0U)
        {
            q = 1U;
        }
        else if (g1 != 0U)
        {
            q = 0U;
        }
        else if (g3 != 0U)
        {
            q = g2;
        }
        break;
    default:
        q = 0U;
        break;
    }
    sim_clc_q[n] = (uint8_t)(q & 1U);
    return sim_clc_q[n];
}

/**
 * @brief Recompute every enabled cell, mirror outputs, and flag edges.
 * @details A disabled cell reads 0 with no edge and no flag; its sequential
 * bit is kept. Flags set on a configured edge even with the PIE bit clear,
 * and the callback runs only under CLCxIF, CLCxIE, PEIE, and GIE.
 */
static void sim_step_clc(void)
{
    uint8_t n;

    for (n = 0U; n < 4U; n++)
    {
        uint16_t base = (uint16_t)(PIC_REG_CLC1CON + ((uint16_t)n * SIM_CLC_STRIDE));
        uint8_t con = pic16f1508_sim_sfr[base];
        uint8_t pol = pic16f1508_sim_sfr[base + 1U];
        uint8_t g0 = 0U;
        uint8_t out;
        uint8_t i;

        for (i = 0U; i < 4U; i++)
        {
            uint8_t g = sim_clc_gate(pic16f1508_sim_sfr[base + 4U + i], sim_clc_d[n],
                                     (uint8_t)((pol >> i) & 1U));
            g0 = (uint8_t)(g0 | (uint8_t)(g << i));
        }
        if ((con & PIC_CLCCON_EN) != 0U)
        {
            out = sim_clc_mode(n, (uint8_t)(con & PIC_CLCCON_MODE_MASK), g0);
            out = (uint8_t)(out ^ ((pol & PIC_CLCPOL_POL) != 0U ? 1U : 0U));
            if ((out != sim_clc_out[n]) && (sim_clc_out[n] == 0U) && ((con & PIC_CLCCON_INTP) != 0U))
            {
                pic16f1508_sim_sfr[PIC_REG_PIR3] |= (uint8_t)(1U << n);
            }
            if ((out != sim_clc_out[n]) && (sim_clc_out[n] != 0U) && ((con & PIC_CLCCON_INTN) != 0U))
            {
                pic16f1508_sim_sfr[PIC_REG_PIR3] |= (uint8_t)(1U << n);
            }
        }
        else
        {
            sim_clc_g1[n] = (uint8_t)(g0 & 1U);
            out = 0U;
        }
        sim_clc_out[n] = out;
        if (out != 0U)
        {
            pic16f1508_sim_sfr[base] |= PIC_CLCCON_OUT;
            pic16f1508_sim_sfr[PIC_REG_CLCDATA] |= (uint8_t)(1U << n);
        }
        else
        {
            pic16f1508_sim_sfr[base] &= (uint8_t)~PIC_CLCCON_OUT;
            pic16f1508_sim_sfr[PIC_REG_CLCDATA] &= (uint8_t)~(1U << n);
        }
    }
    if ((((pic16f1508_sim_sfr[PIC_REG_PIR3] & pic16f1508_sim_sfr[PIC_REG_PIE3]) & 0x0FU) != 0U) &&
        ((pic16f1508_sim_sfr[PIC_REG_INTCON] & (PIC_INTCON_PEIE | PIC_INTCON_GIE)) ==
         (PIC_INTCON_PEIE | PIC_INTCON_GIE)) &&
        (sim_irq_cb != NULL))
    {
        sim_irq_cb();
    }
}

/**
 * @brief Map a port letter to its table index.
 * @param port 'A', 'B', or 'C'.
 * @return 0..2, or -1 for anything else.
 */
static int port_index(char port)
{
    switch (port)
    {
    case 'A': return 0;
    case 'B': return 1;
    case 'C': return 2;
    default:  return -1;
    }
}

/**
 * @brief Recompute PORTx from the latch (output pins) and driven levels (input pins).
 */
static void sim_refresh_ports(void)
{
    int i;
    for (i = 0; i < 3; i++)
    {
        uint8_t tris = pic16f1508_sim_sfr[sim_tris[i]];
        uint8_t lat  = pic16f1508_sim_sfr[sim_lat[i]];
        uint8_t out  = (uint8_t)(lat & (uint8_t)~tris);
        uint8_t in   = (uint8_t)(sim_input_value[i] & tris);
        pic16f1508_sim_sfr[sim_port[i]] = (uint8_t)((out | in) & sim_mask[i]);
    }
}

/**
 * @brief Set IOCBF/IOCIF on RB4..RB7 edges and run the callback when pending.
 * @details The callback is level-gated on IOCIF, IOCIE, and GIE, matching
 * silicon: a pending flag is taken as soon as GIE allows it.
 */
static void sim_step_ioc(void)
{
    uint8_t portb   = pic16f1508_sim_sfr[PIC_REG_PORTB];
    uint8_t changed = (uint8_t)(portb ^ sim_last_portb);
    uint8_t rise    = (uint8_t)(changed & portb);
    uint8_t fall    = (uint8_t)(changed & (uint8_t)~portb);
    uint8_t hits    = (uint8_t)(((rise & pic16f1508_sim_sfr[PIC_REG_IOCBP]) |
                                 (fall & pic16f1508_sim_sfr[PIC_REG_IOCBN])) &
                                PIC16F1508_FAMILY_IOCB_MASK);

    sim_last_portb = portb;
    if (hits != 0U)
    {
        pic16f1508_sim_sfr[PIC_REG_IOCBF] |= hits;
        pic16f1508_sim_sfr[PIC_REG_INTCON] |= PIC_INTCON_IOCIF;
    }

    if (((pic16f1508_sim_sfr[PIC_REG_INTCON] & (PIC_INTCON_IOCIF | PIC_INTCON_IOCIE | PIC_INTCON_GIE)) ==
         (PIC_INTCON_IOCIF | PIC_INTCON_IOCIE | PIC_INTCON_GIE)) &&
        (sim_irq_cb != NULL))
    {
        sim_irq_cb();
    }
}

/**
 * @brief Reset the simulated SFRs to power-up state with all pins as inputs.
 */
void pic16f1508_sim_reset(void)
{
    memset(pic16f1508_sim_sfr, 0, sizeof pic16f1508_sim_sfr);
    /* Power-up direction: all inputs. OPTION_REG reset value 0xFF. */
    pic16f1508_sim_sfr[PIC_REG_TRISA]  = PIC16F1508_FAMILY_PORTA_MASK;
    pic16f1508_sim_sfr[PIC_REG_TRISB]  = 0xFFU;
    pic16f1508_sim_sfr[PIC_REG_TRISC]  = 0xFFU;
    pic16f1508_sim_sfr[PIC_REG_OPTION] = 0xFFU;
    memset(sim_input_value, 0, sizeof sim_input_value);
    memset(sim_clc_d, 0, sizeof sim_clc_d);
    memset(sim_clc_q, 0, sizeof sim_clc_q);
    memset(sim_clc_g1, 0, sizeof sim_clc_g1);
    memset(sim_clc_out, 0, sizeof sim_clc_out);
    sim_irq_cb = NULL;
    sim_refresh_ports();
    sim_last_portb = pic16f1508_sim_sfr[PIC_REG_PORTB];
}

/**
 * @brief Advance the simulation by one step, refreshing ports, IOC, and CLC.
 * @param ticks Elapsed tick count, unused by this model.
 */
void pic16f1508_sim_step(uint32_t ticks)
{
    (void)ticks;
    sim_refresh_ports();
    sim_step_ioc();
    sim_step_clc();
}

/**
 * @brief Set the external level presented on an input pin.
 * @param port Port letter, 'A', 'B' or 'C'. Invalid ports are ignored.
 * @param pin Pin number 0..7. Values above 7 are ignored.
 * @param level Nonzero drives high, zero drives low.
 */
void pic16f1508_sim_drive_input(char port, uint8_t pin, uint8_t level)
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
 * @brief Drive one Dx level into a CLC cell's gate logic.
 * @param cell Cell index 0..3 (CLC1 is 0). Out of range is ignored.
 * @param input Data input 1..4. Any other value is ignored.
 * @param level Nonzero drives high, zero drives low.
 */
void pic16f1508_sim_drive_clc_input(uint8_t cell, uint8_t input, uint8_t level)
{
    uint8_t bit;

    if ((cell > 3U) || (input < 1U) || (input > 4U))
    {
        return;
    }
    bit = (uint8_t)(1U << (input - 1U));
    if (level != 0U)
    {
        sim_clc_d[cell] |= bit;
    }
    else
    {
        sim_clc_d[cell] &= (uint8_t)~bit;
    }
}

/**
 * @brief Read the simulated output level of one pin.
 * @param port Port letter, 'A', 'B' or 'C'.
 * @param pin Pin number 0..7.
 * @return 1 when the pin drives high, 0 when low or invalid.
 */
uint8_t pic16f1508_sim_read_output(char port, uint8_t pin)
{
    int i = port_index(port);

    if ((i < 0) || (pin > 7U))
    {
        return 0U;
    }

    sim_refresh_ports();
    return (uint8_t)((pic16f1508_sim_sfr[sim_port[i]] >> pin) & 1U);
}

/**
 * @brief Register the callback run when IOCIF and IOCIE and GIE are all set.
 * @param cb Callback, or NULL to disable delivery.
 */
void pic16f1508_sim_set_irq_callback(pic16f1508_sim_irq_cb_t cb)
{
    sim_irq_cb = cb;
}
