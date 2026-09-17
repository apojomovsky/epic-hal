/* Host simulation backend for the PIC16F818/819 HAL: the 512-byte
 * register file the host SFR macros dereference, plus the Timer0,
 * Timer1, Timer2, CCP1, SSP, ADC and EEPROM models. Register addresses
 * are DS39598F Figures 2-3/2-4; each model cites its register section
 * (Timer0 Register 2-2, Timer1 Register 7-1, Timer2 Register 8-1, CCP1
 * Register 9-1, SSP Registers 10-1/10-2, ADC Registers 11-1/11-2,
 * EEPROM Registers 3-1/2-6). The sim never bit-bangs external pins: the
 * rig drives and observes them via the sim header. */

#include "pic16f818_819_sim.h"
#include "pic16f818_819_sfr.h"
#include <string.h>

/* register file. */

/* SFR backing store; indices match the DS39598F map (Bank 0 =
 * 0x00..0x1F, Bank 1 = 0x80..0x9F, Bank 2 = 0x100..0x10F, Bank 3 =
 * 0x18C..0x18D used). Size covers all four banks. */
uint8_t pic16f818_819_sim_sfr[0x200];

/** Pin latch overrides set by the host application (per pin, A and B). */
static uint8_t sim_input_override[2] = {0};
static uint8_t sim_input_value   [2] = {0};

/* Optional ISR hook. */
static pic16f818_819_sim_irq_cb_t sim_irq_cb = 0;

/* Prescaler/postscaler accumulators and the CCP1 edge latches. File
 * scope rather than function-local statics so sim_reset() returns the
 * whole model to its power-on state. */
static uint16_t sim_t0_prescaler = 0U;
static uint8_t  sim_t1_prescaler = 0U;
static uint16_t sim_t2_prescaler = 0U;
static uint8_t  sim_t2_post      = 0U;
static uint8_t  sim_ccp1_matched = 0U;
static uint8_t  sim_ccp1_pin     = 0U;
static uint8_t  sim_ccp1_div     = 0U;

/* Forward declarations for the per-peripheral model steps. */
/**
 * @brief Advance the Timer0 model by one instruction cycle.
 */
static void sim_step_timer0(void);
/**
 * @brief Advance the Timer1 model by one instruction cycle.
 */
static void sim_step_timer1(void);
/**
 * @brief Advance the Timer2 model by one instruction cycle.
 */
static void sim_step_timer2(void);
/**
 * @brief Advance the CCP1 compare model by one instruction cycle.
 */
static void sim_step_ccp1(void);
/**
 * @brief Feed a CCP1 capture-input edge into the capture model.
 * @param level the new RB3 level (0 or 1); the model detects the edge.
 */
static void sim_ccp1_capture_edge(uint8_t level);

/* GPIO model. */

/**
 * @brief Read the latched value of a port.
 * @param port the port letter, 'A' or 'B'.
 * @return the port latch byte, or 0xFF for an invalid port.
 */
static uint8_t port_latch(char port)
{
    switch (port)
    {
        case 'A': case 'a': return pic16f818_819_sim_sfr[PIC_REG_PORTA];
        case 'B': case 'b': return pic16f818_819_sim_sfr[PIC_REG_PORTB];
        default:             return 0xFFU;
    }
}

/**
 * @brief Read the TRIS register of a port.
 * @param port the port letter, 'A' or 'B'.
 * @return the TRIS byte, or 0xFF for an invalid port.
 */
static uint8_t tris_reg(char port)
{
    switch (port)
    {
        case 'A': case 'a': return pic16f818_819_sim_sfr[PIC_REG_TRISA];
        case 'B': case 'b': return pic16f818_819_sim_sfr[PIC_REG_TRISB];
        default:             return 0xFFU;
    }
}

/**
 * @brief Map a port letter to the override-array index (0..1).
 * @param port the port letter, 'A' or 'B'.
 * @return the index 0..1 (0 for an invalid port).
 */
static uint8_t port_index(char port)
{
    switch (port)
    {
        case 'A': case 'a': return 0;
        case 'B': case 'b': return 1;
        default:             return 0;
    }
}

/* public API. */

/**
 * @brief Reset the simulator: zero the register file, load power-on
 *        reset values, and clear the input overrides and IRQ hook.
 */
void pic16f818_819_sim_reset(void)
{
    memset(pic16f818_819_sim_sfr, 0, sizeof pic16f818_819_sim_sfr);

    /* Power-on reset images (DS39598F Table 2-1 and the DFP EDC por
     * fields): STATUS keeps TO/PD, PIE1/PIE2 and the peripheral control
     * registers clear, OPTION_REG and the TRIS registers read as
     * inputs, PR2 is all ones. */
    pic16f818_819_sim_sfr[PIC_REG_STATUS]   = PIC_STATUS_POR_VALUE;
    pic16f818_819_sim_sfr[PIC_REG_INTCON]   = PIC_INTCON_POR_VALUE;
    pic16f818_819_sim_sfr[PIC_REG_PIR1]     = PIC_PIR1_POR_VALUE;
    pic16f818_819_sim_sfr[PIC_REG_PIR2]     = PIC_PIR2_POR_VALUE;
    pic16f818_819_sim_sfr[PIC_REG_PIE1]     = PIC_PIE1_POR_VALUE;
    pic16f818_819_sim_sfr[PIC_REG_PIE2]     = PIC_PIE2_POR_VALUE;
    pic16f818_819_sim_sfr[PIC_REG_PCON]     = PIC_PCON_POR_VALUE;
    pic16f818_819_sim_sfr[PIC_REG_OSCCON]   = PIC_OSCCON_POR_VALUE;
    pic16f818_819_sim_sfr[PIC_REG_OSCTUNE]  = PIC_OSCTUNE_POR_VALUE;
    pic16f818_819_sim_sfr[PIC_REG_T1CON]    = PIC_T1CON_POR_VALUE;
    pic16f818_819_sim_sfr[PIC_REG_T2CON]    = PIC_T2CON_POR_VALUE;
    pic16f818_819_sim_sfr[PIC_REG_ADCON0]   = PIC_ADCON0_POR_VALUE;
    pic16f818_819_sim_sfr[PIC_REG_ADCON1]   = PIC_ADCON1_POR_VALUE;
    pic16f818_819_sim_sfr[PIC_REG_OPTION]   = PIC_OPTION_POR_VALUE;
    pic16f818_819_sim_sfr[PIC_REG_TRISA]    = PIC_TRISA_POR_VALUE;
    pic16f818_819_sim_sfr[PIC_REG_TRISB]    = PIC_TRISB_POR_VALUE;
    pic16f818_819_sim_sfr[PIC_REG_PR2]      = PIC_PR2_POR_VALUE;
    pic16f818_819_sim_sfr[PIC_REG_EECON1]   = PIC_EECON1_POR_VALUE;
    pic16f818_819_sim_sfr[PIC_REG_SSPCON]   = PIC_SSPCON_POR_VALUE;
    pic16f818_819_sim_sfr[PIC_REG_SSPSTAT]  = PIC_SSPSTAT_POR_VALUE;
    pic16f818_819_sim_sfr[PIC_REG_CCP1CON]  = PIC_CCP1CON_POR_VALUE;

    /* Both ports power up with every pin an input and no external
     * drive, so the latches read 0 (DS39598F §5.0, Table 2-1's POR
     * column). */
    pic16f818_819_sim_sfr[PIC_REG_PORTA] = 0x00U;
    pic16f818_819_sim_sfr[PIC_REG_PORTB] = 0x00U;

    memset(sim_input_override, 0, sizeof sim_input_override);
    memset(sim_input_value,    0, sizeof sim_input_value);

    sim_t0_prescaler = 0U;
    sim_t1_prescaler = 0U;
    sim_t2_prescaler = 0U;
    sim_t2_post      = 0U;
    sim_ccp1_matched = 0U;
    sim_ccp1_pin     = 0U;
    sim_ccp1_div     = 0U;
}

/**
 * @brief Advance the simulation by `ticks` instruction cycles.
 * @param ticks the number of cycles to advance.
 */
void pic16f818_819_sim_step(uint32_t ticks)
{
    for (uint32_t i = 0; i < ticks; i++)
    {
        sim_step_timer0();
        sim_step_timer1();
        sim_step_timer2();
        sim_step_ccp1();
    }
}

/* Timer0 step. */

/**
 * @brief Advance the Timer0 model by one instruction cycle: apply the
 *        prescaler, increment TMR0, and set TMR0IF on overflow.
 */
static void sim_step_timer0(void)
{
    /* PS2:PS0 select the 1:2 to 1:256 ratio (DS39598F Register 2-2).
     * PSA assigns the prescaler to the WDT, so with PSA=1 TMR0 counts
     * raw instruction cycles: the prescaler is not an extra gate here. */
    uint8_t option = pic16f818_819_sim_sfr[PIC_REG_OPTION];

    static const uint16_t ps_idx[8] = {2, 4, 8, 16, 32, 64, 128, 256};
    uint32_t rate = ps_idx[option & PIC_OPTION_PS_MASK];

    sim_t0_prescaler++;
    if (sim_t0_prescaler < rate) return;
    sim_t0_prescaler = 0U;

    uint8_t t0 = pic16f818_819_sim_sfr[PIC_REG_TMR0];
    t0++;
    if (t0 == 0x00U)
    {
        pic16f818_819_sim_sfr[PIC_REG_INTCON] |= PIC_INTCON_TMR0IF;
        if (sim_irq_cb) sim_irq_cb();
    }
    pic16f818_819_sim_sfr[PIC_REG_TMR0] = t0;
}

/* Timer1 step. */

/**
 * @brief Advance the Timer1 model by one instruction cycle: apply the
 *        prescaler, increment the 16-bit pair, and set TMR1IF on
 *        overflow.
 */
static void sim_step_timer1(void)
{
    /* T1CON (DS39598F Register 7-1): bit 0 TMR1ON, bit 1 TMR1CS, bit 2
     * T1SYNC, bit 3 T1OSCEN, bits 5:4 T1CKPS1:T1CKPS0. This Timer1 has
     * no gate input, so there is no TMR1GE/T1GINV either. */
    uint8_t t1con = pic16f818_819_sim_sfr[PIC_REG_T1CON];
    if (!(t1con & PIC_T1CON_TMR1ON)) return;

    /* TMR1CS = 1 (external or T1OSC clock) has no real signal in the
     * sim, which therefore advances it at the configured prescaler rate
     * per instruction cycle: only the overflow and IRQ plumbing matter
     * to host tests. */
    static const uint8_t ps_idx[4] = {1, 2, 4, 8};
    uint32_t rate = ps_idx[(t1con >> 4) & 0x3U];

    sim_t1_prescaler++;
    if (sim_t1_prescaler < rate) return;
    sim_t1_prescaler = 0U;

    uint16_t full = (uint16_t)(((uint16_t)pic16f818_819_sim_sfr[PIC_REG_TMR1H] << 8) |
                               (uint16_t)pic16f818_819_sim_sfr[PIC_REG_TMR1L]);
    full++;
    pic16f818_819_sim_sfr[PIC_REG_TMR1L] = (uint8_t)(full & 0xFFU);
    pic16f818_819_sim_sfr[PIC_REG_TMR1H] = (uint8_t)(full >> 8);
    if (full == 0U)
    {
        pic16f818_819_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_TMR1IF;
        if (sim_irq_cb) sim_irq_cb();
    }
}

/* Timer2 step. */

/**
 * @brief Advance the Timer2 model by one instruction cycle: apply the
 *        prescaler, increment TMR2, and fire TMR2IF through the
 *        postscaler when the period completes.
 */
static void sim_step_timer2(void)
{
    /* T2CON (DS39598F Register 8-1): bits 1:0 T2CKPS1:T2CKPS0, bit 2
     * TMR2ON, bits 6:3 TOUTPS3:TOUTPS0. */
    uint8_t t2con = pic16f818_819_sim_sfr[PIC_REG_T2CON];
    if (!(t2con & PIC_T2CON_TMR2ON)) return;

    /* T2CKPS1:T2CKPS0 select 1:1, 1:4, 1:16 or 1:16. */
    static const uint8_t pre_idx[4] = {1, 4, 16, 16};
    uint32_t pre = pre_idx[t2con & PIC_T2CON_T2CKPS_MASK];
    /* TOUTPS3:TOUTPS0 select 1:(N+1). */
    uint8_t post = (uint8_t)(((t2con & PIC_T2CON_TOUTPS_MASK) >>
                              PIC_T2CON_TOUTPS_POS) + 1U);
    uint8_t pr2 = pic16f818_819_sim_sfr[PIC_REG_PR2];

    sim_t2_prescaler++;
    if (sim_t2_prescaler < pre) return;
    sim_t2_prescaler = 0U;

    /* TMR2 counts up to PR2 and clears on the next cycle, so one period
     * is (PR2 + 1) ticks and TMR2IF marks each completed period
     * (DS39598F §8.0). */
    uint8_t t2 = pic16f818_819_sim_sfr[PIC_REG_TMR2];
    t2++;
    if (t2 > pr2)
    {
        t2 = 0U;
        sim_t2_post++;
        if (sim_t2_post >= post)
        {
            sim_t2_post = 0U;
            pic16f818_819_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_TMR2IF;
            if (sim_irq_cb) sim_irq_cb();
        }
    }
    pic16f818_819_sim_sfr[PIC_REG_TMR2] = t2;
}

/* CCP1 steps. */

/**
 * @brief Advance the CCP1 compare model by one instruction cycle: latch
 *        PIR1<CCP1IF> once while TMR1H:TMR1L matches CCPR1H:CCPR1L.
 */
static void sim_step_ccp1(void)
{
    /* CCP1CON<3:0> is the mode field (DS39598F Register 9-1): 0010 and
     * 1000..1011 are the compare modes, and a match sets CCP1IF
     * (DS39598F §9.2). 11xx is PWM duty, which has no event to model
     * here, and the pin actions of compare mode are not modelled. */
    uint8_t mode = pic16f818_819_sim_sfr[PIC_REG_CCP1CON] & 0x0FU;
    if ((mode != 0x02U) && ((mode < 0x08U) || (mode > 0x0BU)))
    {
        sim_ccp1_matched = 0U;
        return;
    }

    uint16_t tmr1 = (uint16_t)(((uint16_t)pic16f818_819_sim_sfr[PIC_REG_TMR1H] << 8) |
                               (uint16_t)pic16f818_819_sim_sfr[PIC_REG_TMR1L]);
    uint16_t ccpr1 = (uint16_t)(((uint16_t)pic16f818_819_sim_sfr[PIC_REG_CCP1RH] << 8) |
                                (uint16_t)pic16f818_819_sim_sfr[PIC_REG_CCP1RL]);
    if (tmr1 != ccpr1)
    {
        sim_ccp1_matched = 0U;
        return;
    }
    /* Fire on the entry into the match only: the two counters stay
     * equal for as long as TMR1 sits on the compare value. */
    if (!sim_ccp1_matched)
    {
        sim_ccp1_matched = 1U;
        pic16f818_819_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_CCP1IF;
        if (sim_irq_cb) sim_irq_cb();
    }
}

/**
 * @brief Feed an RB3 level into the CCP1 capture model: on the edge the
 *        armed capture mode selects, latch Timer1 into CCPR1 and set
 *        PIR1<CCP1IF>.
 * @param level the new RB3 level, 0 or 1.
 */
static void sim_ccp1_capture_edge(uint8_t level)
{
    if (level == sim_ccp1_pin) return;
    sim_ccp1_pin = level;

    /* CCP1M3:CCP1M0 capture modes (DS39598F Register 9-1, §9.1): 0100
     * every falling edge, 0101 every rising edge, 0110 every 4th and
     * 0111 every 16th rising edge. Every other mode ignores the pin. */
    uint8_t mode = pic16f818_819_sim_sfr[PIC_REG_CCP1CON] & 0x0FU;
    if (mode == 0x04U)
    {
        sim_ccp1_div = 0U;
        if (level != 0U) return;
    }
    else if ((mode >= 0x05U) && (mode <= 0x07U))
    {
        if (level == 0U) return;
        sim_ccp1_div++;
        uint8_t divisor = (mode == 0x05U) ? 1U : ((mode == 0x06U) ? 4U : 16U);
        if (sim_ccp1_div < divisor) return;
        sim_ccp1_div = 0U;
    }
    else
    {
        sim_ccp1_div = 0U;
        return;
    }

    pic16f818_819_sim_sfr[PIC_REG_CCP1RH] = pic16f818_819_sim_sfr[PIC_REG_TMR1H];
    pic16f818_819_sim_sfr[PIC_REG_CCP1RL] = pic16f818_819_sim_sfr[PIC_REG_TMR1L];
    pic16f818_819_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_CCP1IF;
    if (sim_irq_cb) sim_irq_cb();
}

/* Pin injection and observation. */

/**
 * @brief Drive a digital input pin from the test rig.
 * @param port the port letter, 'A' or 'B'.
 * @param pin the pin number, 0..7.
 * @param level 0 = low, 1 = high.
 */
void pic16f818_819_sim_drive_input(char port, uint8_t pin, uint8_t level)
{
    if (pin > 7U) return;
    uint8_t idx  = port_index(port);
    uint8_t mask = (uint8_t)(1U << pin);
    sim_input_override[idx] |= mask;
    if (level) sim_input_value[idx] |= mask;
    else       sim_input_value[idx] &= (uint8_t)~mask;

    /* Also update the PORT register so EPIC_GPIO_ReadPin sees the
     * externally driven value, but only where TRIS selects an input:
     * writing it to an output pin would clobber the latch the firmware
     * wrote (DS39598F §5.0, a TRIS=1 read returns the pin state). */
    uint8_t pa;
    switch (port)
    {
        case 'A': case 'a': pa = PIC_REG_PORTA; break;
        case 'B': case 'b': pa = PIC_REG_PORTB; break;
        default:             pa = PIC_REG_PORTA; break;
    }
    if (tris_reg(port) & mask)
    {
        uint8_t portval = pic16f818_819_sim_sfr[pa];
        if (level) portval |= mask;
        else       portval &= (uint8_t)~mask;
        pic16f818_819_sim_sfr[pa] = portval;
    }

    /* CCP1 captures on RB3 (DS39598F Table 1-2), so a driven RB3 edge
     * reaches the capture model. */
    if ((idx == 1U) && (pin == 3U)) sim_ccp1_capture_edge(level);
}

/**
 * @brief Read the level currently driven onto a pin.
 * @param port the port letter, 'A' or 'B'.
 * @param pin the pin number, 0..7.
 * @return the pin level, 0 or 1 (0 for an invalid pin).
 */
uint8_t pic16f818_819_sim_read_output(char port, uint8_t pin)
{
    if (pin > 7U) return 0U;
    uint8_t idx  = port_index(port);
    uint8_t mask = (uint8_t)(1U << pin);
    uint8_t tris = tris_reg(port);

    if (tris & mask)
    {
        /* Pin configured as input: return the externally driven level. */
        return (sim_input_override[idx] & mask) ?
               ((sim_input_value[idx] & mask) ? 1U : 0U) :
               /* No override, input floats to 0. */
               0U;
    }
    /* Pin configured as output: return the latch bit. */
    return (port_latch(port) & mask) ? 1U : 0U;
}

/**
 * @brief Install or remove the simulated-interrupt callback.
 * @param cb the callback to fire on a simulated interrupt, or NULL to
 *        unregister.
 */
void pic16f818_819_sim_set_irq_callback(pic16f818_819_sim_irq_cb_t cb)
{
    sim_irq_cb = cb;
}

/* Peripheral event injection. */

/**
 * @brief Inject a byte into the SSP receiver: store it in SSPBUF, set
 *        SSPSTAT<BF> and PIR1<SSPIF>.
 * @param data the byte to inject.
 */
void pic16f818_819_sim_drive_ssp_rx(uint8_t data)
{
    /* SSPBUF is Bank 0; SSPSTAT is Bank 1 at 0x94 (DS39598F Registers
     * 10-1/10-2), so its write is bracketed by the STATUS bank select
     * the target's banked access uses. */
    pic16f818_819_sim_sfr[PIC_REG_SSPBUF] = data;
    {
        uint8_t prev = (uint8_t)((pic16f818_819_sim_sfr[PIC_REG_STATUS] >> 5) & 0x03U);
        pic16f818_819_sim_sfr[PIC_REG_STATUS] =
            (uint8_t)((pic16f818_819_sim_sfr[PIC_REG_STATUS] & 0x1FU) | (1U << 5));
        pic16f818_819_sim_sfr[PIC_REG_SSPSTAT] |= PIC_SSPSTAT_BF;
        pic16f818_819_sim_sfr[PIC_REG_STATUS] =
            (uint8_t)((pic16f818_819_sim_sfr[PIC_REG_STATUS] & 0x1FU) | (prev << 5));
    }
    pic16f818_819_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_SSPIF;
    if (sim_irq_cb) sim_irq_cb();
}

/**
 * @brief Drive an A/D conversion to completion: clear GO/DONE, store
 *        the result in ADRESH:ADRESL in the selected justification and
 *        set PIR1<ADIF>.
 * @param result the 10-bit conversion result, 0..1023.
 */
void pic16f818_819_sim_drive_adc_done(uint16_t result)
{
    result &= 0x03FFU;
    /* Clear GO/DONE (DS39598F Register 11-1, bit 2). */
    pic16f818_819_sim_sfr[PIC_REG_ADCON0] &= (uint8_t)~PIC_ADCON0_GO_DONE;

    uint8_t adfm = (uint8_t)(pic16f818_819_sim_sfr[PIC_REG_ADCON1] & PIC_ADCON1_ADFM);
    if (adfm)
    {
        /* ADFM = 1, right justified (DS39598F §11.4): ADRESH<1:0> =
         * result<9:8>, ADRESL = result<7:0>. */
        pic16f818_819_sim_sfr[PIC_REG_ADRESH] = (uint8_t)((result >> 8) & 0x03U);
    }
    else
    {
        /* ADFM = 0, left justified: ADRESH = result<9:2> and
         * ADRESL<7:6> = result<1:0> (EPIC_ADC_Read shifts it back). */
        pic16f818_819_sim_sfr[PIC_REG_ADRESH] = (uint8_t)((result >> 2) & 0xFFU);
    }

    /* ADRESL is Bank 1 at 0x9E, so the write is bracketed by the STATUS
     * bank select (DS39598F Figure 2-4). */
    {
        uint8_t lo   = adfm ? (uint8_t)(result & 0xFFU)
                            : (uint8_t)((result & 0x03U) << 6);
        uint8_t prev = (uint8_t)((pic16f818_819_sim_sfr[PIC_REG_STATUS] >> 5) & 0x03U);
        pic16f818_819_sim_sfr[PIC_REG_STATUS] =
            (uint8_t)((pic16f818_819_sim_sfr[PIC_REG_STATUS] & 0x1FU) | (1U << 5));
        pic16f818_819_sim_sfr[PIC_REG_ADRESL] = lo;
        pic16f818_819_sim_sfr[PIC_REG_STATUS] =
            (uint8_t)((pic16f818_819_sim_sfr[PIC_REG_STATUS] & 0x1FU) | (prev << 5));
    }

    pic16f818_819_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_ADIF;
    if (sim_irq_cb) sim_irq_cb();
}

/* Simulated EEPROM storage. The 16F819 has 256 bytes and the 16F818 the
 * lower 128 of the same table. The host driver reaches it through
 * pic14_sim_eeprom_read(); the Bank 2 data pair (0x10C/0x10D) and Bank 3
 * control pair (0x18C/0x18D) stay ordinary register-file bytes here, as
 * this flat model has no RD/write strobe (DS39598F Register 3-1). */
static uint8_t sim_eeprom[256];

/**
 * @brief Place a byte in the simulated EEPROM array.
 * @param addr the EEPROM address, 0..255.
 * @param data the byte to store.
 */
void pic16f818_819_sim_drive_eeprom_byte(uint8_t addr, uint8_t data)
{
    /* `addr` is uint8_t (0..255), always a valid index into sim_eeprom[256]. */
    sim_eeprom[addr] = data;
}

/**
 * @brief Simulate a completed EEPROM write: store the byte and set
 *        PIR2<EEIF>.
 * @param addr the EEPROM address that was written.
 * @param data the byte that was stored.
 */
void pic16f818_819_sim_drive_eeprom_done(uint8_t addr, uint8_t data)
{
    sim_eeprom[addr] = data;
    /* The write-complete flag is PIR2<4>, not EECON1<4> (DS39598F
     * Registers 2-6 and 3-1). */
    pic16f818_819_sim_sfr[PIC_REG_PIR2] |= PIC_PIR2_EEIF;
    if (sim_irq_cb) sim_irq_cb();
}

/**
 * @brief Read a byte from the simulated EEPROM array (the shared host
 *        driver's read path).
 * @param addr the EEPROM address, 0..255.
 * @return the stored byte.
 */
uint8_t pic14_sim_eeprom_read(uint8_t addr)
{
    return sim_eeprom[addr];
}
