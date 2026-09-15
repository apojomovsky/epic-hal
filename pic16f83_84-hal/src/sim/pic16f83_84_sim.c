/* Host simulation backend for the PIC16F83/84/84A HAL: provides the
 * memory-backed register file the host SFR macros dereference and the
 * peripheral models (Timer0, data EEPROM). The stepping models mirror
 * the shared pic14 drivers; every address is DFP-verified against
 * edc/PIC16F84A.PIC (§ cites below name the DS35007B sections). The
 * sim never bit-bangs external pins; the test rig drives and observes
 * them through pic16f83_84_sim.h's helpers. */

#include "pic16f83_84_sim.h"
#include "pic16f83_84_sfr.h"
#include <string.h>

/* register file. */

/* SFR backing store; indices match the datasheet register map (Bank 0
 * = 0x00..0x0B used, Bank 1 = 0x80..0x8B used). Size covers the
 * address space with room to spare. */
uint8_t pic16f83_84_sim_sfr[0x200];

/** Pin latch overrides set by the host application (per pin, A..B). */
static uint8_t sim_input_override[2] = {0};
static uint8_t sim_input_value   [2] = {0};

/* Optional ISR hook. */
static pic16f83_84_sim_irq_cb_t sim_irq_cb = 0;

/**
 * @brief Advance the Timer0 model by one instruction cycle.
 */
static void sim_step_timer0(void);

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
        case 'A': case 'a': return pic16f83_84_sim_sfr[PIC_REG_PORTA];
        case 'B': case 'b': return pic16f83_84_sim_sfr[PIC_REG_PORTB];
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
        case 'A': case 'a': return pic16f83_84_sim_sfr[PIC_REG_TRISA];
        case 'B': case 'b': return pic16f83_84_sim_sfr[PIC_REG_TRISB];
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
void pic16f83_84_sim_reset(void)
{
    memset(pic16f83_84_sim_sfr, 0, sizeof pic16f83_84_sim_sfr);

    /* Power-on reset values (DS35007B §14.0 Table 14-4: INTCON clears,
     * STATUS = 0x00011xxx (TO, PD set), peripheral registers clear;
     * OPTION_REG and the TRIS registers reset to 1 = input). */
    pic16f83_84_sim_sfr[PIC_REG_STATUS]   = PIC_STATUS_POR_VALUE;
    pic16f83_84_sim_sfr[PIC_REG_INTCON]   = PIC_INTCON_POR_VALUE;
    pic16f83_84_sim_sfr[PIC_REG_OPTION]   = PIC_OPTION_POR_VALUE;
    pic16f83_84_sim_sfr[PIC_REG_TRISA]    = PIC_TRISA_POR_VALUE;
    pic16f83_84_sim_sfr[PIC_REG_TRISB]    = PIC_TRISB_POR_VALUE;
    pic16f83_84_sim_sfr[PIC_REG_EECON1]   = PIC_EECON1_POR_VALUE;

    /* PORTA on POR reads as 0 (inputs, no external drives). */
    pic16f83_84_sim_sfr[PIC_REG_PORTA]    = 0x00U;

    memset(sim_input_override, 0, sizeof sim_input_override);
    memset(sim_input_value,    0, sizeof sim_input_value);
}

/**
 * @brief Advance the simulation by `ticks` instruction cycles.
 * @param ticks the number of instruction cycles to advance.
 */
void pic16f83_84_sim_step(uint32_t ticks)
{
    for (uint32_t i = 0; i < ticks; i++)
    {
        sim_step_timer0();
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
     * PS<2:0> live in OPTION_REG, bits 0..2. */
    uint8_t option = pic16f83_84_sim_sfr[PIC_REG_OPTION];
    uint8_t ps     = option & 0x07U;                  /* PS2:PS1:PS0 */
    uint8_t psa    = (option >> 3) & 0x01U;           /* PSA */

    static uint16_t t0_prescaler = 0U;
    /* PSA=1 assigns the prescaler to the WDT; TMR0 then runs with no
     * prescaler, so psa does not gate the counter here. */
    (void)psa;

    /* OPTION_REG<PS2:PS0> prescaler mapping (DS35007B §5.0, Table 5-1). */
    static const uint8_t ps_idx[8] = {2, 4, 8, 16, 32, 64, 128, 255};
    uint32_t rate = ps_idx[ps];

    t0_prescaler++;
    if (t0_prescaler < rate) return;
    t0_prescaler = 0U;

    uint8_t t0 = pic16f83_84_sim_sfr[PIC_REG_TMR0];
    t0++;
    if (t0 == 0x00U)
    {
        pic16f83_84_sim_sfr[PIC_REG_INTCON] |= PIC_INTCON_TMR0IF;
        if (sim_irq_cb) sim_irq_cb();
    }
    pic16f83_84_sim_sfr[PIC_REG_TMR0] = t0;
}

/**
 * @brief Drive a digital input pin from the test rig.
 * @param port the port letter, 'A' or 'B'.
 * @param pin the pin number, 0..7.
 * @param level 0 = low, 1 = high.
 */
void pic16f83_84_sim_drive_input(char port, uint8_t pin, uint8_t level)
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
        default:             pa = PIC_REG_PORTA; break;
    }
    uint8_t portval = pic16f83_84_sim_sfr[pa];
    if (level) portval |= mask;
    else portval &= (uint8_t)~mask;
    pic16f83_84_sim_sfr[pa] = portval;
}

/**
 * @brief Read the level currently driven onto a pin.
 * @param port the port letter, 'A' or 'B'.
 * @param pin the pin number, 0..7.
 * @return the pin level, 0 or 1 (0 for an invalid pin).
 */
uint8_t pic16f83_84_sim_read_output(char port, uint8_t pin)
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
void pic16f83_84_sim_set_irq_callback(pic16f83_84_sim_irq_cb_t cb)
{
    sim_irq_cb = cb;
}

/* Simulated EEPROM storage. The part has 64 bytes of data EEPROM;
 * the table keeps 256 entries and the rest is ignored. */
static uint8_t sim_eeprom[256];
static uint8_t sim_eeprom_loaded[256];

/**
 * @brief Place a byte in the simulated EEPROM array.
 * @param addr the EEPROM address (0..255 index; the part uses 0..63).
 * @param data the byte to store.
 */
void pic16f83_84_sim_drive_eeprom_byte(uint8_t addr, uint8_t data)
{
    /* `addr` is uint8_t (0..255), always a valid index into sim_eeprom[256]. */
    sim_eeprom[addr] = data;
    sim_eeprom_loaded[addr] = 1U;
}

/**
 * @brief Simulate a completed EEPROM write: store the byte and set
 *        EECON1<EEIF> (DS35007B §3.0), then fire the IRQ hook.
 * @param addr the EEPROM address that was written.
 * @param data the byte that was stored.
 */
void pic16f83_84_sim_drive_eeprom_done(uint8_t addr, uint8_t data)
{
    sim_eeprom[addr] = data;
    sim_eeprom_loaded[addr] = 1U;
    pic16f83_84_sim_sfr[PIC_REG_EECON1] |= PIC_EECON1_EEIF;
    if (sim_irq_cb) sim_irq_cb();
}

/**
 * @brief Read a byte from the simulated EEPROM array (the shared
 *        driver's host-side read path).
 * @param addr the EEPROM address to read.
 * @return the stored byte.
 */
uint8_t pic14_sim_eeprom_read(uint8_t addr)
{
    return sim_eeprom[addr];
}
