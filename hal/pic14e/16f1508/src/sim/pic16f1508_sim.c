/**
 * Host model of the PIC16F1508/1509 ports, interrupt-on-change, CLC cells,
 * and timers 1 and 2. Models only what the GPIO/IOC/CLC/TMR contracts
 * observe: timers count instruction cycles (ticks per step call), with
 * prescalers, PR2 match and postscaler for TMR2 and 16-bit overflow for
 * TMR1. External clock sources are not modeled except through
 * pic16f1508_sim_timer1_external_pulse.
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
/* Hook-driven D1..D4 levels per cell in bits 3:0, with a valid bit per Dx.
 * A driven Dx wins over the SEL decode below, so probes that drive every Dx
 * observe exactly what they drive. An undriven Dx decodes its SEL code. */
static uint8_t sim_clc_d[4];
static uint8_t sim_clc_hooked[4];
/* Sequential bit, last gate-1 level for clock edges, and last output for INTN/INTP. */
static uint8_t sim_clc_q[4];
static uint8_t sim_clc_g1[4];
static uint8_t sim_clc_out[4];
/* Timer prescaler and postscaler residuals: consumed input cycles toward the
 * next TMR1 count, and consumed matches toward the next TMR2IF. */
static uint16_t sim_t1_prescaler;
static uint16_t sim_t2_prescaler;
static uint8_t sim_t2_postscaler;
/* T1_overflow and T2_match pulses seen since the last CLC sample. Silicon
 * feeds the CLC the overflow and the pre-postscaler PR2 match as pulses, not
 * the latched TMR1IF/TMR2IF levels (DS40001609E Figures 19-1 and 20-1). */
static uint8_t sim_t1_overflow_pulse;
static uint8_t sim_t2_match_pulse;
/* Last observed TMR2 and T2CON: a change is a software write, which clears
 * the prescaler and postscaler counters (DS40001609E §20.1). */
static uint8_t sim_t2_last_tmr;
static uint8_t sim_t2_last_con;

/**
 * @brief Test whether the simulated GIE bit is set.
 * @return Non-zero when interrupts can vector, zero otherwise.
 */
static uint8_t sim_gie(void)
{
    return (pic16f1508_sim_sfr[PIC_REG_INTCON] & PIC_INTCON_GIE) ? 1U : 0U;
}

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
 * @brief Evaluate one cell's mode logic without touching sequential state.
 * @details The settle loop calls this for provisional outputs; the wrapper
 * below commits q and the G1 edge memory once per step, so a clocked cell
 * samples the settled cascade on a single edge.
 * @param mode LCxMODE value 0..7.
 * @param g0 Gate outputs G1..G4 packed in bits 3:0.
 * @param q Sequential bit held at step entry.
 * @param g1prev G1 level held at step entry, for edge detection.
 * @return Raw cell output before the POL bit.
 */
static uint8_t sim_clc_mode_next(uint8_t mode, uint8_t g0, uint8_t q, uint8_t g1prev)
{
    uint8_t g1 = (uint8_t)(g0 & 1U);
    uint8_t g2 = (uint8_t)((g0 >> 1U) & 1U);
    uint8_t g3 = (uint8_t)((g0 >> 2U) & 1U);
    uint8_t g4 = (uint8_t)((g0 >> 3U) & 1U);
    uint8_t rise = (uint8_t)(g1 & (uint8_t)~g1prev);

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
    return (uint8_t)(q & 1U);
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
    uint8_t q = sim_clc_mode_next(mode, g0, sim_clc_q[n], sim_clc_g1[n]);

    sim_clc_g1[n] = (uint8_t)(g0 & 1U);
    sim_clc_q[n] = (uint8_t)(q & 1U);
    return sim_clc_q[n];
}

/**
 * @brief Read the fixed input pin behind CLCxIN0 or CLCxIN1.
 * @details Pin map is the pin-function tables (DS40001609E section 2):
 * CLC1 on RA3/RC7, CLC2 on RC3/RC4, CLC3 on RB4/RC6, CLC4 on RB5/RA1.
 * PORTx holds the driven level for input pins after the refresh ahead of
 * the CLC step.
 * @param cell Cell index 0..3.
 * @param second Zero for CLCxIN0, nonzero for CLCxIN1.
 * @return Pinned level, 0 or 1.
 */
static uint8_t sim_clc_pin(uint8_t cell, uint8_t second)
{
    static const uint8_t port[4][2] = {{0U, 2U}, {2U, 2U}, {1U, 2U}, {1U, 0U}};
    static const uint8_t pin[4][2] = {{3U, 7U}, {3U, 4U}, {4U, 6U}, {5U, 1U}};
    static const uint16_t reg[3] = {PIC_REG_PORTA, PIC_REG_PORTB, PIC_REG_PORTC};
    uint8_t side = (second != 0U) ? 1U : 0U;

    return (uint8_t)((pic16f1508_sim_sfr[reg[port[cell][side]]] >> pin[cell][side]) & 1U);
}

/**
 * @brief Resolve one Dx multiplexer to its Table 24-1 signal level.
 * @details Inverts EPIC_CLC_SourceCode: D1 sees LCx_in[0..7], D2 [4..11],
 * D3 [8..15], D4 [12..15] at codes 0..3 then [0..3] at codes 4..7.
 * LCxOUT reads the post-POL wire (Register 24-1: LCxOUT is after LCxPOL,
 * sampled from lcx_out, the same wire Table 24-1 taps). T1_overflow and
 * T2_match read the pulse seen since the previous sample, so a single-cycle
 * pulse is never lost and TMR2IF stays out of it: T2_match is ahead of the
 * postscaler. Anything without a sim source reads 0.
 * @param cell Cell index 0..3.
 * @param mux Data input 0..3 (D1..D4).
 * @param tap Cell outputs settling this step, post-POL.
 * @return Decoded level, 0 or 1.
 */
static uint8_t sim_clc_sel(uint8_t cell, uint8_t mux, const uint8_t tap[4])
{
    uint16_t base = (uint16_t)(PIC_REG_CLC1CON + ((uint16_t)cell * SIM_CLC_STRIDE));
    uint8_t code = (mux < 2U) ? pic16f1508_sim_sfr[base + 2U] : pic16f1508_sim_sfr[base + 3U];
    uint8_t in;

    code = (uint8_t)((code >> (((mux & 1U) != 0U) ? PIC_CLCSEL_HIGH_SHIFT : 0U)) & 0x07U);
    if (mux == 0U)
    {
        in = code;
    }
    else if (mux == 1U)
    {
        in = (uint8_t)(code + 4U);
    }
    else if (mux == 2U)
    {
        in = (uint8_t)(code + 8U);
    }
    else
    {
        in = (code < 4U) ? (uint8_t)(code + 12U) : (uint8_t)(code - 4U);
    }
    if (in < 2U)
    {
        return sim_clc_pin(cell, in);
    }
    if (in == 6U)
    {
        return sim_t1_overflow_pulse;
    }
    if (in == 7U)
    {
        return sim_t2_match_pulse;
    }
    if ((in >= 8U) && (in < 12U))
    {
        return tap[in - 8U];
    }
    return 0U;
}

/**
 * @brief Evaluate one cell's four gates over its Dx levels.
 * @param cell Cell index 0..3.
 * @param tap Cell outputs settling this step, post-POL.
 * @return Gate outputs G1..G4 packed in bits 3:0.
 */
static uint8_t sim_clc_gates(uint8_t cell, const uint8_t tap[4])
{
    uint16_t base = (uint16_t)(PIC_REG_CLC1CON + ((uint16_t)cell * SIM_CLC_STRIDE));
    uint8_t pol = pic16f1508_sim_sfr[base + 1U];
    uint8_t d = 0U;
    uint8_t g0 = 0U;
    uint8_t i;

    for (i = 0U; i < 4U; i++)
    {
        uint8_t dv = (((sim_clc_hooked[cell] >> i) & 1U) != 0U)
            ? (uint8_t)((sim_clc_d[cell] >> i) & 1U)
            : sim_clc_sel(cell, i, tap);
        d = (uint8_t)(d | (uint8_t)(dv << i));
    }
    for (i = 0U; i < 4U; i++)
    {
        uint8_t g = sim_clc_gate(pic16f1508_sim_sfr[base + 4U + i], d,
                                 (uint8_t)((pol >> i) & 1U));
        g0 = (uint8_t)(g0 | (uint8_t)(g << i));
    }
    return g0;
}

/**
 * @brief Test whether a mode settles without a clock edge.
 * @details Modes 0..3 and 7 are level logic (the S-R and transparent latches
 * are asynchronous), so their outputs join the combinational fixed point.
 * Modes 4..6 capture on the G1 rise and sample the settled cascade once.
 * @param mode LCxMODE value 0..7.
 * @return Non-zero for combinational modes, zero for clocked modes.
 */
static uint8_t sim_clc_is_combinational(uint8_t mode)
{
    return ((mode == 4U) || (mode == 5U) || (mode == 6U)) ? 0U : 1U;
}

/* Sweeps bounding the settle loop; a 4-cell cascade settles within 4. */
#define SIM_CLC_SETTLE_PASSES 4U

/**
 * @brief Settle the combinational cells against the current tap levels.
 * @details One bounded fixed-point sweep over modes 0..3 and 7; clocked
 * cells hold their entry outputs while it runs, so same-clock flop pairs
 * still advance one stage per step.
 * @param cur Cell outputs settling this step, post-POL, updated in place.
 * @param q_hold Provisional sequential bits, updated in place.
 */
static void sim_clc_settle(uint8_t cur[4], uint8_t q_hold[4])
{
    uint8_t n;
    uint8_t pass;

    for (pass = 0U; pass < SIM_CLC_SETTLE_PASSES; pass++)
    {
        uint8_t calm = 1U;

        for (n = 0U; n < 4U; n++)
        {
            uint16_t base = (uint16_t)(PIC_REG_CLC1CON + ((uint16_t)n * SIM_CLC_STRIDE));
            uint8_t con = pic16f1508_sim_sfr[base];
            uint8_t mode = (uint8_t)(con & PIC_CLCCON_MODE_MASK);
            uint8_t out;

            if (((con & PIC_CLCCON_EN) == 0U) ||
                (sim_clc_is_combinational(mode) == 0U))
            {
                continue;
            }
            out = sim_clc_mode_next(mode, sim_clc_gates(n, cur), q_hold[n], sim_clc_g1[n]);
            q_hold[n] = out;
            out = (uint8_t)(out ^ ((pic16f1508_sim_sfr[base + 1U] & PIC_CLCPOL_POL) != 0U ? 1U : 0U));
            if (out != cur[n])
            {
                cur[n] = out;
                calm = 0U;
            }
        }
        if (calm != 0U)
        {
            break;
        }
    }
}

/**
 * @brief Recompute every enabled cell, mirror outputs, and flag edges.
 * @details A disabled cell reads 0 with no edge and no flag; its sequential
 * bit is kept. Flags set on a configured edge even with the PIE bit clear,
 * and the callback runs only under CLCxIF, CLCxIE, PEIE, and GIE. The step
 * settles the combinational cells, samples the result into the clocked cells
 * on one G1 edge, then settles again, so a cascade lands in one step like
 * the unclocked lcx_out wire (DS40001609E Figure 24-1, page 257). An
 * unstable loop keeps its last sweep and re-evaluates each step, the way
 * silicon outruns the Q1 sample instead of holding still.
 */

static void sim_step_clc(void)
{
    uint8_t n;
    uint8_t cur[4] = {sim_clc_out[0], sim_clc_out[1], sim_clc_out[2], sim_clc_out[3]};
    uint8_t entry[4] = {sim_clc_out[0], sim_clc_out[1], sim_clc_out[2], sim_clc_out[3]};
    uint8_t q_hold[4] = {sim_clc_q[0], sim_clc_q[1], sim_clc_q[2], sim_clc_q[3]};
    uint8_t fro[4];

    sim_clc_settle(cur, q_hold);
    fro[0] = cur[0];
    fro[1] = cur[1];
    fro[2] = cur[2];
    fro[3] = cur[3];
    for (n = 0U; n < 4U; n++)
    {
        uint16_t base = (uint16_t)(PIC_REG_CLC1CON + ((uint16_t)n * SIM_CLC_STRIDE));
        uint8_t con = pic16f1508_sim_sfr[base];
        uint8_t mode = (uint8_t)(con & PIC_CLCCON_MODE_MASK);
        uint8_t out;

        if (((con & PIC_CLCCON_EN) != 0U) &&
            (sim_clc_is_combinational(mode) == 0U))
        {
            /* Frozen taps: same-clock flop pairs advance one stage per step
             * whatever the wiring order, since no committed Q feeds a later
             * sample in this loop. */
            out = sim_clc_mode(n, mode, sim_clc_gates(n, fro));
            out = (uint8_t)(out ^ ((pic16f1508_sim_sfr[base + 1U] & PIC_CLCPOL_POL) != 0U ? 1U : 0U));
            cur[n] = out;
        }
    }
    sim_clc_settle(cur, q_hold);
    for (n = 0U; n < 4U; n++)
    {
        uint16_t base = (uint16_t)(PIC_REG_CLC1CON + ((uint16_t)n * SIM_CLC_STRIDE));
        uint8_t con = pic16f1508_sim_sfr[base];
        uint8_t out;

        if ((con & PIC_CLCCON_EN) != 0U)
        {
            if (sim_clc_is_combinational((uint8_t)(con & PIC_CLCCON_MODE_MASK)) != 0U)
            {
                sim_clc_g1[n] = (uint8_t)(sim_clc_gates(n, cur) & 1U);
                sim_clc_q[n] = q_hold[n];
            }
            out = cur[n];
            if ((out != entry[n]) && (entry[n] == 0U) && ((con & PIC_CLCCON_INTP) != 0U))
            {
                pic16f1508_sim_sfr[PIC_REG_PIR3] |= (uint8_t)(1U << n);
            }
            if ((out != entry[n]) && (entry[n] != 0U) && ((con & PIC_CLCCON_INTN) != 0U))
            {
                pic16f1508_sim_sfr[PIC_REG_PIR3] |= (uint8_t)(1U << n);
            }
        }
        else
        {
            sim_clc_g1[n] = (uint8_t)(sim_clc_gates(n, cur) & 1U);
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
    sim_t1_overflow_pulse = 0U;
    sim_t2_match_pulse = 0U;
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
 * @brief Count one prescaled TMR1 tick: increment TMR1H:TMR1L and flag overflow.
 */
static void sim_timer1_count(void)
{
    uint8_t lo = pic16f1508_sim_sfr[PIC_REG_TMR1L];
    uint8_t hi = pic16f1508_sim_sfr[PIC_REG_TMR1H];
    uint16_t full = (uint16_t)(((uint16_t)hi << 8) | lo);

    full++;
    pic16f1508_sim_sfr[PIC_REG_TMR1L] = (uint8_t)(full & 0xFFU);
    pic16f1508_sim_sfr[PIC_REG_TMR1H] = (uint8_t)(full >> 8);
    if (full == 0x0000U)
    {
        pic16f1508_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_TMR1IF;
        sim_t1_overflow_pulse = 1U;
        /* The flag latches regardless; the vector needs GIE, as on silicon. */
        if ((sim_irq_cb != NULL) && (sim_gie() != 0U))
        {
            sim_irq_cb();
        }
    }
}

/**
 * @brief Advance the TMR1 model by one instruction cycle.
 * @details T1CON layout (DS40001609E Register 19-1): TMR1CS<1:0> in bits 7:6,
 * T1CKPS<1:0> in bits 5:4, TMR1ON in bit 0. Only the internal FOSC/4 source
 * (TMR1CS 00) advances here; any other source halts, since the model has no
 * signal behind it. External T1CKI edges arrive through
 * pic16f1508_sim_timer1_external_pulse instead.
 */
static void sim_step_timer1(void)
{
    static const uint16_t ps_ratio[4] = { 1, 2, 4, 8 };
    uint8_t t1con = pic16f1508_sim_sfr[PIC_REG_T1CON];
    uint8_t ckps;

    if ((t1con & PIC_T1CON_TMR1ON) == 0U)
    {
        return;
    }
    if ((t1con & PIC_T1CON_TMR1CS_MASK) != 0U)
    {
        return;
    }
    ckps = (uint8_t)(((unsigned int)t1con & PIC_T1CON_T1CKPS_MASK) >> PIC_T1CON_T1CKPS_SHIFT);
    sim_t1_prescaler++;
    if (sim_t1_prescaler < ps_ratio[ckps & 0x03U])
    {
        return;
    }
    sim_t1_prescaler = 0U;
    sim_timer1_count();
}

/**
 * @brief Advance the TMR2 model by one instruction cycle.
 * @details T2CON layout (DS40001609E Register 20-1): T2OUTPS<3:0> in bits
 * 6:3, TMR2ON in bit 2, T2CKPS<1:0> in bits 1:0 with ratios 1:1, 1:4, 1:16
 * and 1:64. Each prescaled tick compares TMR2 against PR2: a match resets
 * TMR2 to 0 and feeds the postscaler, which sets TMR2IF every N+1 matches.
 */
static void sim_step_timer2(void)
{
    static const uint16_t ps_ratio[4] = { 1, 4, 16, 64 };
    uint8_t t2con = pic16f1508_sim_sfr[PIC_REG_T2CON];
    uint8_t tmr2;
    uint8_t pr2;
    uint8_t outps;

    if ((t2con & PIC_T2CON_TMR2ON) == 0U)
    {
        return;
    }
    /* A changed TMR2 or T2CON is a software write: both scaler counters
     * clear, so the next match counts from zero again. */
    tmr2 = pic16f1508_sim_sfr[PIC_REG_TMR2];
    if ((t2con != sim_t2_last_con) || (tmr2 != sim_t2_last_tmr))
    {
        sim_t2_prescaler = 0U;
        sim_t2_postscaler = 0U;
        sim_t2_last_con = t2con;
        sim_t2_last_tmr = tmr2;
    }
    sim_t2_prescaler++;
    if (sim_t2_prescaler < ps_ratio[t2con & PIC_T2CON_T2CKPS_MASK])
    {
        return;
    }
    sim_t2_prescaler = 0U;
    pr2 = pic16f1508_sim_sfr[PIC_REG_PR2];
    if (tmr2 == pr2)
    {
        pic16f1508_sim_sfr[PIC_REG_TMR2] = 0U;
        sim_t2_last_tmr = 0U;
        sim_t2_match_pulse = 1U;
        sim_t2_postscaler++;
        outps = (uint8_t)(((unsigned int)t2con & PIC_T2CON_T2OUTPS_MASK) >> PIC_T2CON_T2OUTPS_SHIFT);
        if (sim_t2_postscaler > outps)
        {
            sim_t2_postscaler = 0U;
            pic16f1508_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_TMR2IF;
            /* The flag latches regardless; the vector needs GIE. */
            if ((sim_irq_cb != NULL) && (sim_gie() != 0U))
            {
                sim_irq_cb();
            }
        }
    }
    else
    {
        pic16f1508_sim_sfr[PIC_REG_TMR2] = (uint8_t)(tmr2 + 1U);
        sim_t2_last_tmr = (uint8_t)(tmr2 + 1U);
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
    /* PR2 powers up at 0xFF (DS40001609E §20.1). */
    pic16f1508_sim_sfr[PIC_REG_PR2] = 0xFFU;
    memset(sim_input_value, 0, sizeof sim_input_value);
    memset(sim_clc_d, 0, sizeof sim_clc_d);
    memset(sim_clc_hooked, 0, sizeof sim_clc_hooked);
    memset(sim_clc_q, 0, sizeof sim_clc_q);
    memset(sim_clc_g1, 0, sizeof sim_clc_g1);
    memset(sim_clc_out, 0, sizeof sim_clc_out);
    sim_t1_prescaler = 0U;
    sim_t2_prescaler = 0U;
    sim_t2_postscaler = 0U;
    sim_t2_last_tmr = 0U;
    sim_t2_last_con = 0U;
    sim_t1_overflow_pulse = 0U;
    sim_t2_match_pulse = 0U;
    sim_irq_cb = NULL;
    sim_refresh_ports();
    sim_last_portb = pic16f1508_sim_sfr[PIC_REG_PORTB];
}

/**
 * @brief Advance the simulation by a tick count, then sample pins, IOC and CLC once.
 * @details Each tick advances the timers by one instruction cycle. Ports, IOC
 * and CLC still sample once per call, as before: at one tick per call the
 * behavior matches the old model exactly.
 * @param ticks Instruction cycles to run the timers for.
 */
void pic16f1508_sim_step(uint32_t ticks)
{
    uint32_t i;

    for (i = 0U; i < ticks; i++)
    {
        sim_step_timer1();
        sim_step_timer2();
    }
    sim_refresh_ports();
    sim_step_ioc();
    sim_step_clc();
}

/**
 * @brief Inject one T1CKI rising edge into the TMR1 counter path.
 * @details Applies the T1CKPS prescaler, then counts exactly like an internal
 * tick. Edges advance the counter only while TMR1ON is set with the external
 * source selected (TMR1CS 10); any other setup ignores them.
 */
void pic16f1508_sim_timer1_external_pulse(void)
{
    static const uint16_t ps_ratio[4] = { 1, 2, 4, 8 };
    uint8_t t1con = pic16f1508_sim_sfr[PIC_REG_T1CON];
    uint8_t ckps;

    if ((t1con & PIC_T1CON_TMR1ON) == 0U)
    {
        return;
    }
    if ((t1con & PIC_T1CON_TMR1CS_MASK) != (uint8_t)(0x2U << PIC_T1CON_TMR1CS_SHIFT))
    {
        return;
    }
    ckps = (uint8_t)(((unsigned int)t1con & PIC_T1CON_T1CKPS_MASK) >> PIC_T1CON_T1CKPS_SHIFT);
    sim_t1_prescaler++;
    if (sim_t1_prescaler < ps_ratio[ckps & 0x03U])
    {
        return;
    }
    sim_t1_prescaler = 0U;
    sim_timer1_count();
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
 * @details Marks the Dx hook-driven: it wins over the SEL decode until reset.
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
    sim_clc_hooked[cell] |= bit;
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
