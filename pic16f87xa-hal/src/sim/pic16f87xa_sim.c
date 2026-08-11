/* Host simulation backend for the PIC16F87XA HAL: provides the 512-byte
 * memory-backed register file the host SFR macros dereference and the
 * peripheral models (Timer0/1/2, ADC, USART, MSSP, EEPROM). The sim
 * never bit-bangs external pins; the test rig drives and observes them
 * through pic16f87xa_sim.h's helpers. */

#include "pic16f87xa_sim.h"
#include "pic16f87xa_sfr.h"
#include <string.h>

/* register file. */

/* SFR backing store; indices match the datasheet register map (Bank 0
 * = 0x00..0x1F, Bank 1 = 0x80..0x9F, etc.). Size covers the EEPROM bank
 * (Bank 2 = 0x100..0x11F) plus Bank 3 reserved. */
uint8_t pic16f87xa_sim_sfr[0x200];

/** Pin latch overrides set by the host application (per pin, A..E). */
static uint8_t sim_input_override[5] = {0};
static uint8_t sim_input_value   [5] = {0};

/* Optional ISR hook. */
static pic16f87xa_sim_irq_cb_t sim_irq_cb = 0;

/* Forward declarations for the per-timer step helpers. */
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
 * @brief Advance the USART model by one instruction cycle.
 */
static void sim_step_usart(void);

/* GPIO model. */

/**
 * @brief Read the latched value of a port.
 * @param port the port letter, 'A'..'E'.
 * @return the port latch byte, or 0xFF for an invalid port.
 */
static uint8_t port_latch(char port)
{
    switch (port) {
        case 'A': case 'a': return pic16f87xa_sim_sfr[PIC_REG_PORTA];
        case 'B': case 'b': return pic16f87xa_sim_sfr[PIC_REG_PORTB];
        case 'C': case 'c': return pic16f87xa_sim_sfr[PIC_REG_PORTC];
        case 'D': case 'd': return pic16f87xa_sim_sfr[PIC_REG_PORTD];
        case 'E': case 'e': return pic16f87xa_sim_sfr[PIC_REG_PORTE];
        default:             return 0xFFU;
    }
}

/**
 * @brief Read the TRIS register of a port.
 * @param port the port letter, 'A'..'E'.
 * @return the TRIS byte, or 0xFF for an invalid port.
 */
static uint8_t tris_reg(char port)
{
    switch (port) {
        case 'A': case 'a': return pic16f87xa_sim_sfr[PIC_REG_TRISA];
        case 'B': case 'b': return pic16f87xa_sim_sfr[PIC_REG_TRISB];
        case 'C': case 'c': return pic16f87xa_sim_sfr[PIC_REG_TRISC];
        case 'D': case 'd': return pic16f87xa_sim_sfr[PIC_REG_TRISD];
        case 'E': case 'e': return pic16f87xa_sim_sfr[PIC_REG_TRISE];
        default:             return 0xFFU;
    }
}

/**
 * @brief Map a port letter to the override-array index (0..4).
 * @param port the port letter, 'A'..'E'.
 * @return the index 0..4 (0 for an invalid port).
 */
static uint8_t port_index(char port)
{
    switch (port) {
        case 'A': case 'a': return 0;
        case 'B': case 'b': return 1;
        case 'C': case 'c': return 2;
        case 'D': case 'd': return 3;
        case 'E': case 'e': return 4;
        default:             return 0;
    }
}

/* public API. */

/**
 * @brief Reset the simulator: zero the register file, load power-on
 *        reset values, and clear the input overrides and IRQ hook.
 */
void pic16f87xa_sim_reset(void)
{
    memset(pic16f87xa_sim_sfr, 0, sizeof pic16f87xa_sim_sfr);

    /* Power-on reset values from DS39582B Table 14-6. */
    pic16f87xa_sim_sfr[PIC_REG_STATUS]   = PIC_STATUS_POR_VALUE;
    pic16f87xa_sim_sfr[PIC_REG_PCON]     = PIC_PCON_POR_VALUE;
    pic16f87xa_sim_sfr[PIC_REG_INTCON]   = PIC_INTCON_POR_VALUE;
    pic16f87xa_sim_sfr[PIC_REG_PIR1]     = PIC_PIR1_POR_VALUE;
    pic16f87xa_sim_sfr[PIC_REG_PIR2]     = PIC_PIR2_POR_VALUE;
    /* PIR1 <TXIF> resets to 1 (TXREG empty after POR, §10.2.1).
     * Bit 4 of PIR1 (at 0x0C). */
    pic16f87xa_sim_sfr[0x0CU] |= 0x10U;

    /* PIE1 / PIE2, Bank 1 mirrors of PIR1 / PIR2, reset to 0. */
    pic16f87xa_sim_sfr[0x8CU] = PIC_PIE1_POR_VALUE;
    pic16f87xa_sim_sfr[0x8DU] = PIC_PIE2_POR_VALUE;
    pic16f87xa_sim_sfr[PIC_REG_T1CON]    = PIC_T1CON_POR_VALUE;
    pic16f87xa_sim_sfr[PIC_REG_T2CON]    = PIC_T2CON_POR_VALUE;
    pic16f87xa_sim_sfr[PIC_REG_ADCON0]   = PIC_ADCON0_POR_VALUE;
    pic16f87xa_sim_sfr[PIC_REG_ADCON1]   = PIC_ADCON1_POR_VALUE;

    /* PORTA on POR reads as analog input '0' (DS39582B §4.1). */
    pic16f87xa_sim_sfr[PIC_REG_PORTA]    = 0x00U;
    pic16f87xa_sim_sfr[PIC_REG_PORTE]    = 0x00U;

    /* TRIS defaults: 1 = input, so PORTA:6 and others = 1. */
    pic16f87xa_sim_sfr[PIC_REG_TRISA]    = 0x3FU;     /* 6-bit wide. */
    pic16f87xa_sim_sfr[PIC_REG_TRISB]    = 0xFFU;
    pic16f87xa_sim_sfr[PIC_REG_TRISC]    = 0xFFU;
    pic16f87xa_sim_sfr[PIC_REG_TRISD]    = 0xFFU;
    pic16f87xa_sim_sfr[PIC_REG_TRISE]    = 0x07U;

    memset(sim_input_override, 0, sizeof sim_input_override);
    memset(sim_input_value,    0, sizeof sim_input_value);
}

/**
 * @brief Advance the simulation by `ticks` instruction cycles.
 * @param ticks the number of cycles to advance.
 */
void pic16f87xa_sim_step(uint32_t ticks)
{
    for (uint32_t i = 0; i < ticks; i++) {
        sim_step_timer0();
        sim_step_timer1();
        sim_step_timer2();
        sim_step_usart();
    }
}

/* Timer0 step. */

/**
 * @brief Advance the Timer0 model by one instruction cycle: apply the
 *        prescaler, increment TMR0, and set TMR0IF on overflow.
 */
static void sim_step_timer0(void)
{
    /* Read the active Timer0 prescaler.
     * T0PS<2:0> live in OPTION_REG, bits 0..2. */
    uint8_t option = pic16f87xa_sim_sfr[PIC_REG_OPTION];
    uint8_t ps     = option & 0x07U;                  /* PS2:PS1:PS0 */
    uint8_t psa    = (option >> 3) & 0x01U;           /* PSA */

    static uint16_t t0_prescaler = 0U;
    /* PSA=1 assigns the prescaler to the WDT; TMR0 then runs with no
     * prescaler, so psa does not gate the counter here. */
    (void)psa;

    /* OPTION_REG<PS2:PS0> prescaler mapping (DS39582B §5.0, Table 5-1). */
    static const uint8_t ps_idx[8] = {2, 4, 8, 16, 32, 64, 128, 255};
    uint32_t rate = ps_idx[ps];

    t0_prescaler++;
    if (t0_prescaler < rate) return;
    t0_prescaler = 0U;

    uint8_t t0 = pic16f87xa_sim_sfr[PIC_REG_TMR0];
    t0++;
    if (t0 == 0x00U) {
        pic16f87xa_sim_sfr[PIC_REG_INTCON] |= PIC_INTCON_TMR0IF;
        if (sim_irq_cb) sim_irq_cb();
    }
    pic16f87xa_sim_sfr[PIC_REG_TMR0] = t0;
}

/* Timer1 step. */

/**
 * @brief Advance the Timer1 model by one instruction cycle: apply the
 *        prescaler, increment the 16-bit counter, and set TMR1IF on
 *        overflow.
 */
static void sim_step_timer1(void)
{
    /* T1CON layout (DS39582B Register 6-1):
     *   bit 0  TMR1ON
     *   bit 1  TMR1CS
     *   bit 2  T1SYNC
     *   bit 3  T1OSCEN
     *   bit 4  T1CKPS0
     *   bit 5  T1CKPS1
     */
    uint8_t t1con = pic16f87xa_sim_sfr[PIC_REG_T1CON];
    if (!(t1con & 0x01U)) return;     /* TMR1ON = 0 → stopped. */
    /* TMR1CS = 1 (external / T1OSC): the sim does not model a real
     * signal, so it advances at the configured prescaler rate per
     * instruction cycle. T1OSC firmware (example_idle_blink) then runs
     * on the host; the 32 kHz rate itself is not reproduced, only the
     * overflow/IRQ plumbing. */

    static const uint8_t ps_idx[4] = {1, 2, 4, 8};
    uint32_t rate = ps_idx[(t1con >> 4) & 0x3U];

    static uint8_t t1_prescaler = 0U;
    t1_prescaler++;
    if (t1_prescaler < rate) return;
    t1_prescaler = 0U;

    /* 16-bit increment, big-endian in registers. */
    uint8_t lo = pic16f87xa_sim_sfr[PIC_REG_TMR1L];
    uint8_t hi = pic16f87xa_sim_sfr[PIC_REG_TMR1H];
    uint16_t full = (uint16_t)(((uint16_t)hi << 8) | lo);
    full++;
    pic16f87xa_sim_sfr[PIC_REG_TMR1L] = (uint8_t)(full & 0xFFU);
    pic16f87xa_sim_sfr[PIC_REG_TMR1H] = (uint8_t)(full >> 8);
    if (full == 0U) {
        /* PIR1 is at 0x0C (DS39582B Table 3-1 / Figure 2-3). */
        pic16f87xa_sim_sfr[0x0CU] |= 0x01U;   /* TMR1IF. */
        if (sim_irq_cb) sim_irq_cb();
    }
}

/* Timer2 step. */

/**
 * @brief Advance the Timer2 model by one instruction cycle: apply the
 *        prescaler, increment TMR2, and fire TMR2IF (through the
 *        postscaler) when the period completes.
 */
static void sim_step_timer2(void)
{
    /* T2CON layout (DS39582B Register 7-1):
     *   bit 0  T2CKPS0
     *   bit 1  T2CKPS1
     *   bit 2  TMR2ON
     *   bit 3..6 TOUTPS0..TOUTPS3
     */
    uint8_t t2con = pic16f87xa_sim_sfr[PIC_REG_T2CON];
    if (!(t2con & 0x04U)) return;     /* TMR2ON = 0 → stopped. */

    /* T2CKPS1:T2CKPS0 → 1:1, 1:4, 1:16, 1:16 (DS39582B Register 7-1). */
    static const uint8_t pre_idx[4] = {1, 4, 16, 16};
    uint32_t pre = pre_idx[t2con & 0x3U];
    /* TOUTPS3:TOUTPS0 → 1:(N+1). */
    uint8_t  post = (uint8_t)(((t2con >> 3) & 0xFU) + 1U);

    /* Read PR2 (Bank 1, address 0x92 per DS39582B §7.0). */
    uint8_t pr2 = pic16f87xa_sim_sfr[0x92U];

    static uint16_t t2_prescaler = 0U;
    static uint8_t  t2_post      = 0U;

    t2_prescaler++;
    if (t2_prescaler < pre) return;
    t2_prescaler = 0U;

    /* Increment TMR2. DS39582B §7.0: TMR2 increments until it matches
     * PR2; on the next cycle it resets. TMR2IF fires once per period
     * of (PR2+1) cycles. */
    uint8_t t2 = pic16f87xa_sim_sfr[PIC_REG_TMR2];
    t2++;
    if (t2 > pr2) {
        /* Period complete: TMR2IF (after postscaler) fires here. */
        t2 = 0U;
        t2_post++;
        if (t2_post >= post) {
            t2_post = 0U;
            pic16f87xa_sim_sfr[0x0CU] |= 0x02U;   /* PIR1<TMR2IF>. */
            if (sim_irq_cb) sim_irq_cb();
        }
    }
    pic16f87xa_sim_sfr[PIC_REG_TMR2] = t2;
}

/* USART step. */

/**
 * @brief Re-assert PIR1<TXIF> each cycle while TXEN is set, modeling
 *        instantaneous transmit completion.
 */
static void sim_step_usart(void)
{
    /* Re-assert TXIF every cycle when TXEN is set: writing TXREG clears
     * it (EPIC_USART_Transmit), and this step models the instantaneous
     * transmit completion. RCIF is set by
     * pic16f87xa_sim_drive_usart_rx(). */
    uint8_t txsta = EPIC_REG8(PIC_REG_TXSTA);
    if (txsta & PIC_TXSTA_TXEN) {
        EPIC_REG8(0x0CU) |= 0x10U;     /* PIR1<TXIF> */
    }
}

/**
 * @brief Drive a digital input pin from the test rig.
 * @param port the port letter, 'A'..'E'.
 * @param pin the pin number, 0..7.
 * @param level 0 = low, 1 = high.
 */
void pic16f87xa_sim_drive_input(char port, uint8_t pin, uint8_t level)
{
    if (pin > 7U) return;
    uint8_t idx = port_index(port);
    uint8_t mask = (uint8_t)(1U << pin);
    sim_input_override[idx] |= mask;
    if (level) sim_input_value[idx] |= mask;
    else       sim_input_value[idx] &= (uint8_t)~mask;

    /* Also update the PORT register so EPIC_GPIO_ReadPin sees the
     * externally driven value for input pins, matching real hardware
     * (TRIS=1 reads return the pin's external state). */
    uint8_t pa;
    switch (port) {
        case 'A': case 'a': pa = PIC_REG_PORTA; break;
        case 'B': case 'b': pa = PIC_REG_PORTB; break;
        case 'C': case 'c': pa = PIC_REG_PORTC; break;
        case 'D': case 'd': pa = PIC_REG_PORTD; break;
        case 'E': case 'e': pa = PIC_REG_PORTE; break;
        default:             pa = PIC_REG_PORTA; break;
    }
    uint8_t portval = pic16f87xa_sim_sfr[pa];
    if (level) portval |= mask;
    else portval &= (uint8_t)~mask;
    pic16f87xa_sim_sfr[pa] = portval;
}

/**
 * @brief Read the level currently driven onto a pin.
 * @param port the port letter, 'A'..'E'.
 * @param pin the pin number, 0..7.
 * @return the pin level, 0 or 1 (0 for an invalid pin).
 */
uint8_t pic16f87xa_sim_read_output(char port, uint8_t pin)
{
    if (pin > 7U) return 0U;
    uint8_t idx  = port_index(port);
    uint8_t mask = (uint8_t)(1U << pin);
    uint8_t tris = tris_reg(port);

    if (tris & mask) {
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
void pic16f87xa_sim_set_irq_callback(pic16f87xa_sim_irq_cb_t cb)
{
    sim_irq_cb = cb;
}

/**
 * @brief Inject a byte into the USART receiver: store it in RCREG and
 *        set PIR1<RCIF>.
 * @param data the byte to inject.
 */
void pic16f87xa_sim_drive_usart_rx(uint8_t data)
{
    /* Place the byte in RCREG (0x1A, DS39582B §10.x). */
    pic16f87xa_sim_sfr[PIC_REG_RCREG] = data;
    /* Set PIR1<RCIF> (bit 5). */
    pic16f87xa_sim_sfr[0x0CU] |= 0x20U;
    if (sim_irq_cb) sim_irq_cb();
}

/**
 * @brief Inject a byte into the SSP receiver: store it in SSPBUF, set
 *        SSPSTAT<BF> and PIR1<SSPIF>.
 * @param data the byte to inject.
 */
void pic16f87xa_sim_drive_ssp_rx(uint8_t data)
{
    /* Place byte in SSPBUF (0x13, DS39582B §9.x). */
    pic16f87xa_sim_sfr[PIC_REG_SSPBUF] = data;
    /* Set SSPSTAT<BF> (Bank 1, addr 0x94, bit 0). */
    {
        uint8_t prev = (pic16f87xa_sim_sfr[PIC_REG_STATUS] >> 5) & 0x03U;
        pic16f87xa_sim_sfr[PIC_REG_STATUS] =
            (uint8_t)((pic16f87xa_sim_sfr[PIC_REG_STATUS] & 0x1FU) | (1U << 5));
        pic16f87xa_sim_sfr[0x94U] |= 0x01U;
        pic16f87xa_sim_sfr[PIC_REG_STATUS] =
            (uint8_t)((pic16f87xa_sim_sfr[PIC_REG_STATUS] & 0x1FU) | (prev << 5));
    }
    /* Set PIR1<SSPIF> (bit 3). */
    pic16f87xa_sim_sfr[0x0CU] |= 0x08U;
    if (sim_irq_cb) sim_irq_cb();
}

/**
 * @brief Drive an A/D conversion to completion: clear GO/DONE, store the
 *        result right-justified in ADRESH:ADRESL and set PIR1<ADIF>.
 * @param result the 10-bit conversion result, 0..1023.
 */
void pic16f87xa_sim_drive_adc_done(uint16_t result)
{
    /* Clear GO/DONE in ADCON0. */
    pic16f87xa_sim_sfr[0x1FU] &= (uint8_t)~0x04U;
    /* Store result right-justified in ADRESH:ADRESL.
     * DS39582B §11.4.1 + Figure 11-4 (ADFM=1):
     *   ADRESH[1:0] = result[9:8], ADRESH[7:2] = 0
     *   ADRESL[7:0] = result[7:0]
     */
    pic16f87xa_sim_sfr[0x1EU] = (uint8_t)((result >> 8) & 0x03U);
    {
        uint8_t prev = (pic16f87xa_sim_sfr[PIC_REG_STATUS] >> 5) & 0x03U;
        pic16f87xa_sim_sfr[PIC_REG_STATUS] =
            (uint8_t)((pic16f87xa_sim_sfr[PIC_REG_STATUS] & 0x1FU) | (1U << 5));
        pic16f87xa_sim_sfr[0x9EU] = (uint8_t)(result & 0xFFU);
        pic16f87xa_sim_sfr[PIC_REG_STATUS] =
            (uint8_t)((pic16f87xa_sim_sfr[PIC_REG_STATUS] & 0x1FU) | (prev << 5));
    }
    /* Set PIR1<ADIF> (bit 6). */
    pic16f87xa_sim_sfr[0x0CU] |= 0x40U;
    if (sim_irq_cb) sim_irq_cb();
}

/* Simulated EEPROM storage.  All four parts use the same 256-byte
 * table; the smaller 28-pin parts have 128 bytes of data EEPROM
 * and ignore the upper half. */
static uint8_t sim_eeprom[256];
static uint8_t sim_eeprom_loaded[256];

/**
 * @brief Place a byte in the simulated EEPROM array.
 * @param addr the EEPROM address, 0..255.
 * @param data the byte to store.
 */
void pic16f87xa_sim_drive_eeprom_byte(uint8_t addr, uint8_t data)
{
    /* `addr` is uint8_t (0..255), always a valid index into sim_eeprom[256]. */
    sim_eeprom[addr] = data;
    sim_eeprom_loaded[addr] = 1U;
}

/**
 * @brief Simulate a completed EEPROM write: store the byte and set
 *        PIR2<EEIF>.
 * @param addr the EEPROM address that was written.
 * @param data the byte that was stored.
 */
void pic16f87xa_sim_drive_eeprom_done(uint8_t addr, uint8_t data)
{
    sim_eeprom[addr] = data;
    sim_eeprom_loaded[addr] = 1U;
    /* Set PIR2<EEIF> (bit 4). */
    pic16f87xa_sim_sfr[0x0DU] |= 0x10U;
    if (sim_irq_cb) sim_irq_cb();
}

/**
 * @brief Read a byte from the simulated EEPROM array.
 * @param addr the EEPROM address, 0..255.
 * @return the stored byte.
 */
uint8_t pic16f87xa_sim_eeprom_read(uint8_t addr)
{
    return sim_eeprom[addr];
}
