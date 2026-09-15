/* Host simulation backend for the PIC16F63x/67x/68x HAL: provides the
 * 512-byte memory-backed register file the host SFR macros dereference
 * and the peripheral models (Timer0, Timer1, comparators, EEPROM). The
 * stepping models mirror the shared pic14 drivers; banked addresses
 * index the file directly (Bank 2 = 0x100..0x11F, Bank 3 =
 * 0x180..0x19F). The sim never bit-bangs external pins; the test rig
 * drives and observes them through pic16f63x_67x_68x_sim.h's helpers. */

#include "pic16f63x_67x_68x_sim.h"
#include "pic16f63x_67x_68x_sfr.h"
#include <string.h>

/* register file. */

/* SFR backing store; indices match the datasheet register map. Size
 * covers the highest banked address (SRCON at 0x19E) with room to
 * spare. */
uint8_t pic16f63x_67x_68x_sim_sfr[0x200];

/** Pin latch overrides set by the host application (per pin, A..C). */
static uint8_t sim_input_override[3] = {0};
static uint8_t sim_input_value   [3] = {0};

/* Optional ISR hook. */
static pic16f63x_67x_68x_sim_irq_cb_t sim_irq_cb = 0;

/* Forward declarations for the per-timer step helpers. */
/**
 * @brief Advance the Timer0 model by one instruction cycle.
 */
static void sim_step_timer0(void);
/**
 * @brief Advance the Timer1 model by one instruction cycle.
 */
static void sim_step_timer1(void);

/* GPIO model. */

/**
 * @brief Read the latched value of a port.
 * @param port the port letter, 'A', 'B' or 'C'.
 * @return the port latch byte, or 0xFF for an invalid port.
 */
static uint8_t port_latch(char port)
{
    switch (port)
    {
        case 'A': case 'a': return pic16f63x_67x_68x_sim_sfr[PIC_REG_PORTA];
        case 'B': case 'b': return pic16f63x_67x_68x_sim_sfr[PIC_REG_PORTB];
        case 'C': case 'c': return pic16f63x_67x_68x_sim_sfr[PIC_REG_PORTC];
        default:             return 0xFFU;
    }
}

/**
 * @brief Read the TRIS register of a port.
 * @param port the port letter, 'A', 'B' or 'C'.
 * @return the TRIS byte, or 0xFF for an invalid port.
 */
static uint8_t tris_reg(char port)
{
    switch (port)
    {
        case 'A': case 'a': return pic16f63x_67x_68x_sim_sfr[PIC_REG_TRISA];
        case 'B': case 'b': return pic16f63x_67x_68x_sim_sfr[PIC_REG_TRISB];
        case 'C': case 'c': return pic16f63x_67x_68x_sim_sfr[PIC_REG_TRISC];
        default:             return 0xFFU;
    }
}

/**
 * @brief Map a port letter to the override-array index (0..2).
 * @param port the port letter, 'A', 'B' or 'C'.
 * @return the index 0..2 (0 for an invalid port).
 */
static uint8_t port_index(char port)
{
    switch (port)
    {
        case 'A': case 'a': return 0;
        case 'B': case 'b': return 1;
        case 'C': case 'c': return 2;
        default:             return 0;
    }
}

/* public API. */

/**
 * @brief Reset the simulator: zero the register file, load power-on
 *        reset values, and clear the input overrides and IRQ hook.
 */
void pic16f63x_67x_68x_sim_reset(void)
{
    memset(pic16f63x_67x_68x_sim_sfr, 0, sizeof pic16f63x_67x_68x_sim_sfr);

    /* Power-on reset values (DS40001262F memory map; STATUS 0x18 has
     * TO/PD set). Fresh-POR PCON keeps SBOREN set with both status
     * bits readable, the way the 628A sim models a just-powered part
     * (the EDC por string leaves nBOR/nPOR unknown). */
    pic16f63x_67x_68x_sim_sfr[PIC_REG_STATUS]   = PIC_STATUS_POR_VALUE;
    pic16f63x_67x_68x_sim_sfr[PIC_REG_PCON]     = 0x13U;
    pic16f63x_67x_68x_sim_sfr[PIC_REG_INTCON]   = PIC_INTCON_POR_VALUE;
    pic16f63x_67x_68x_sim_sfr[PIC_REG_PIR1]     = PIC_PIR1_POR_VALUE;
    pic16f63x_67x_68x_sim_sfr[PIC_REG_PIR2]     = PIC_PIR2_POR_VALUE;
    pic16f63x_67x_68x_sim_sfr[PIC_REG_PIE1]     = PIC_PIE1_POR_VALUE;
    pic16f63x_67x_68x_sim_sfr[PIC_REG_PIE2]     = PIC_PIE2_POR_VALUE;
    pic16f63x_67x_68x_sim_sfr[PIC_REG_T1CON]    = PIC_T1CON_POR_VALUE;
    pic16f63x_67x_68x_sim_sfr[PIC_REG_OPTION]   = PIC_OPTION_POR_VALUE;
    pic16f63x_67x_68x_sim_sfr[PIC_REG_ANSEL]    = PIC_ANSEL_POR_VALUE;
    pic16f63x_67x_68x_sim_sfr[PIC_REG_CM2CON1]  = PIC_CM2CON1_POR_VALUE;
    /* ANSELH POR (677): the 631 has no ANSELH (reads 0 on silicon);
     * the shared sim carries the superset so the 677 digital path
     * (ANSELH cleared by Init) is exercised on host. */
    pic16f63x_67x_68x_sim_sfr[PIC_REG_ANSELH]   = PIC_ANSELH_POR_VALUE;

    /* TRIS defaults: 1 = input on every implemented pin. */
    pic16f63x_67x_68x_sim_sfr[PIC_REG_TRISA]    = PIC_TRISA_POR_VALUE;
    pic16f63x_67x_68x_sim_sfr[PIC_REG_TRISB]    = PIC_TRISB_POR_VALUE;
    pic16f63x_67x_68x_sim_sfr[PIC_REG_TRISC]    = PIC_TRISC_POR_VALUE;

    memset(sim_input_override, 0, sizeof sim_input_override);
    memset(sim_input_value,    0, sizeof sim_input_value);
}

/**
 * @brief Advance the simulation by `ticks` instruction cycles.
 * @param ticks the number of cycles to advance.
 */
void pic16f63x_67x_68x_sim_step(uint32_t ticks)
{
    for (uint32_t i = 0; i < ticks; i++)
    {
        sim_step_timer0();
        sim_step_timer1();
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
    uint8_t option = pic16f63x_67x_68x_sim_sfr[PIC_REG_OPTION];
    uint8_t ps     = option & 0x07U;                  /* PS2:PS1:PS0 */
    uint8_t psa    = (option >> 3) & 0x01U;           /* PSA */

    static uint16_t t0_prescaler = 0U;
    /* PSA=1 assigns the prescaler to the WDT; TMR0 then runs with no
     * prescaler, so psa does not gate the counter here. */
    (void)psa;

    /* OPTION_REG<PS2:PS0> prescaler mapping (DS40001262F §5.0). */
    static const uint8_t ps_idx[8] = {2, 4, 8, 16, 32, 64, 128, 255};
    uint32_t rate = ps_idx[ps];

    t0_prescaler++;
    if (t0_prescaler < rate) return;
    t0_prescaler = 0U;

    uint8_t t0 = pic16f63x_67x_68x_sim_sfr[PIC_REG_TMR0];
    t0++;
    if (t0 == 0x00U)
    {
        pic16f63x_67x_68x_sim_sfr[PIC_REG_INTCON] |= PIC_INTCON_TMR0IF;
        if (sim_irq_cb) sim_irq_cb();
    }
    pic16f63x_67x_68x_sim_sfr[PIC_REG_TMR0] = t0;
}

/* Timer1 step. */

/**
 * @brief Advance the Timer1 model by one instruction cycle: apply the
 *        prescaler, increment the 16-bit counter, and set TMR1IF on
 *        overflow.
 */
static void sim_step_timer1(void)
{
    /* T1CON layout (DS40001262F §6.0, Register 6-1):
     *   bit 0  TMR1ON
     *   bit 1  TMR1CS
     *   bit 2  T1SYNC
     *   bit 3  T1OSCEN
     *   bit 4  T1CKPS0
     *   bit 5  T1CKPS1
     */
    uint8_t t1con = pic16f63x_67x_68x_sim_sfr[PIC_REG_T1CON];
    if (!(t1con & 0x01U)) return;     /* TMR1ON = 0 → stopped. */
    /* TMR1CS = 1 (external): the sim does not model a real signal, so
     * it advances at the configured prescaler rate per instruction
     * cycle; only the overflow/IRQ plumbing is reproduced. */

    static const uint8_t ps_idx[4] = {1, 2, 4, 8};
    uint32_t rate = ps_idx[(t1con >> 4) & 0x3U];

    static uint8_t t1_prescaler = 0U;
    t1_prescaler++;
    if (t1_prescaler < rate) return;
    t1_prescaler = 0U;

    /* 16-bit increment. */
    uint8_t lo = pic16f63x_67x_68x_sim_sfr[PIC_REG_TMR1L];
    uint8_t hi = pic16f63x_67x_68x_sim_sfr[PIC_REG_TMR1H];
    uint16_t full = (uint16_t)(((uint16_t)hi << 8) | lo);
    full++;
    pic16f63x_67x_68x_sim_sfr[PIC_REG_TMR1L] = (uint8_t)(full & 0xFFU);
    pic16f63x_67x_68x_sim_sfr[PIC_REG_TMR1H] = (uint8_t)(full >> 8);
    if (full == 0U)
    {
        pic16f63x_67x_68x_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_TMR1IF;
        if (sim_irq_cb) sim_irq_cb();
    }
}

/**
 * @brief Drive a digital input pin from the test rig.
 * @param port the port letter, 'A', 'B' or 'C'.
 * @param pin the pin number, 0..7.
 * @param level 0 = low, 1 = high.
 */
void pic16f63x_67x_68x_sim_drive_input(char port, uint8_t pin, uint8_t level)
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
    switch (port)
    {
        case 'A': case 'a': pa = PIC_REG_PORTA; break;
        case 'B': case 'b': pa = PIC_REG_PORTB; break;
        case 'C': case 'c': pa = PIC_REG_PORTC; break;
        default:             pa = PIC_REG_PORTA; break;
    }
    uint8_t portval = pic16f63x_67x_68x_sim_sfr[pa];
    if (level) portval |= mask;
    else portval &= (uint8_t)~mask;
    pic16f63x_67x_68x_sim_sfr[pa] = portval;
}

/**
 * @brief Read the level currently driven onto a pin.
 * @param port the port letter, 'A', 'B' or 'C'.
 * @param pin the pin number, 0..7.
 * @return the pin level, 0 or 1 (0 for an invalid pin).
 */
uint8_t pic16f63x_67x_68x_sim_read_output(char port, uint8_t pin)
{
    if (pin > 7U) return 0U;
    uint8_t latch = port_latch(port);
    uint8_t tris = tris_reg(port);
    /* An output pin shows the latch; an input pin shows the driven
     * level (or 0 if undriven). */
    if (!(tris & (1U << pin)))
    {
        return (latch >> pin) & 0x01U;
    }
    uint8_t idx = port_index(port);
    if (sim_input_override[idx] & (1U << pin))
    {
        return (sim_input_value[idx] >> pin) & 0x01U;
    }
    return 0U;
}

/**
 * @brief Install or remove the interrupt callback.
 * @param cb the callback to fire on a simulated interrupt, or NULL to
 *        unregister.
 */
void pic16f63x_67x_68x_sim_set_irq_callback(pic16f63x_67x_68x_sim_irq_cb_t cb)
{
    sim_irq_cb = cb;
}

/**
 * @brief Drive a comparator output and raise its change flag.
 * @param comp 1 for C1, 2 for C2.
 * @param level 0 = output low, 1 = output high.
 */
void pic16f63x_67x_68x_sim_drive_comparator(uint8_t comp, uint8_t level)
{
    /* uint16_t: Bank-2 CMxCON0 addresses (0x119/0x11A) do not
     * fit a uint8_t index. */
    uint16_t cm_addr = (comp == 2U) ? PIC_REG_CM2CON0 : PIC_REG_CM1CON0;
    uint8_t flag = (comp == 2U) ? PIC_PIR2_C2IF : PIC_PIR2_C1IF;
    uint8_t cm = pic16f63x_67x_68x_sim_sfr[cm_addr];
    if (level) cm |= PIC_CMx_CxOUT;
    else       cm &= (uint8_t)~PIC_CMx_CxOUT;
    pic16f63x_67x_68x_sim_sfr[cm_addr] = cm;
    /* A real output transition raises the change flag. */
    pic16f63x_67x_68x_sim_sfr[PIC_REG_PIR2] |= flag;
    if (sim_irq_cb) sim_irq_cb();
}

/* Simulated EEPROM storage, 256 entries (the 631 implements 128;
 * the 677 implements all 256). */
static uint8_t sim_eeprom[256];
static uint8_t sim_eeprom_loaded[256];

/**
 * @brief Place a byte in the simulated EEPROM array.
 * @param addr the EEPROM address, 0..255 (631: 0..127).
 * @param data the byte to store.
 */
void pic16f63x_67x_68x_sim_drive_eeprom_byte(uint8_t addr, uint8_t data)
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
void pic16f63x_67x_68x_sim_drive_eeprom_done(uint8_t addr, uint8_t data)
{
    sim_eeprom[addr] = data;
    sim_eeprom_loaded[addr] = 1U;
    /* Set PIR2<EEIF> (bit 4). */
    pic16f63x_67x_68x_sim_sfr[PIC_REG_PIR2] |= PIC_PIR2_EEIF;
    if (sim_irq_cb) sim_irq_cb();
}

/**
 * @brief Read a byte from the simulated EEPROM array.
 * @param addr the EEPROM address, 0..255.
 * @return the stored byte.
 */
uint8_t pic14_sim_eeprom_read(uint8_t addr)
{
    return sim_eeprom[addr];
}
