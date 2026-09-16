/* Host simulation backend for the PIC16F5x HAL: provides the
 * memory-backed register file the host SFR macros dereference, the
 * control shadow registers TRIS/OPTION route into, and the Timer0
 * model. The stepping model mirrors the 14-bit families' sim; every
 * address is DFP-verified against edc/PIC16F54.PIC (DS cites below
 * name the DS41213D sections). The sim never bit-bangs external pins;
 * the test rig drives and observes them through pic16f5x_sim.h's
 * helpers. */

#include "pic16f5x_sim.h"
#include "pic16f5x_sfr.h"
#include <string.h>

/* register file. */

/* SFR backing store; indices match the datasheet register map (16F54
 * file space 0x00..0x06 used, GPR to 0x1F; the 4-bank parts extend to
 * 0x7F). Size covers the address space with room to spare. */
uint8_t pic16f5x_sim_sfr[256];

/* Control shadow registers (write-only on the real die; the host
 * platform header routes EPIC_TRIS_WRITE / EPIC_OPTION_WRITE here so
 * tests can read back what the driver programmed). */
uint8_t pic16f5x_sim_trisa = 0xFFU;
uint8_t pic16f5x_sim_trisb = 0xFFU;
#if PIC16F5X_FAMILY_HAS_PORTC
uint8_t pic16f5x_sim_trisc = 0xFFU;
#endif
#if PIC16F5X_FAMILY_HAS_PORTD
uint8_t pic16f5x_sim_trisd = 0xFFU;
#endif
#if PIC16F5X_FAMILY_HAS_PORTE
uint8_t pic16f5x_sim_trise = 0xFFU;
#endif
uint8_t pic16f5x_sim_option = 0xFFU;

/** Pin latch overrides set by the host application (per pin, A..B). */
static uint8_t sim_input_override[2] = {0};
static uint8_t sim_input_value   [2] = {0};

/* GPIO model. */

/**
 * @brief Map a port letter to the PORTx register address.
 * @param port the port letter, 'A' or 'B'.
 * @return the file-register address.
 */
static uint16_t port_reg(char port)
{
    switch (port)
    {
#if PIC16F5X_FAMILY_HAS_PORTA
        case 'A': case 'a': return PIC_REG_PORTA;
#endif
        case 'B': case 'b': return PIC_REG_PORTB;
#if PIC16F5X_FAMILY_HAS_PORTC
        case 'C': case 'c': return PIC_REG_PORTC;
#endif
#if PIC16F5X_FAMILY_HAS_PORTD
        case 'D': case 'd': return PIC_REG_PORTD;
#endif
#if PIC16F5X_FAMILY_HAS_PORTE
        case 'E': case 'e': return PIC_REG_PORTE;
#endif
        default:             return PIC_REG_PORTB;
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
#if PIC16F5X_FAMILY_HAS_PORTA
        case 'A': case 'a': return 0;
#endif
        case 'B': case 'b': return 1;
        default:             return 1;
    }
}

/* public API. */

/**
 * @brief Reset the simulator: zero the register file, load power-on
 *        reset values, and clear the input overrides.
 */
void pic16f5x_sim_reset(void)
{
    memset(pic16f5x_sim_sfr, 0, sizeof pic16f5x_sim_sfr);

    /* Power-on reset (DS41213D §3.0 Table 3-1): STATUS = 0x00011xxx
     * (nPD, nTO set, PA=0); TMR0/PCL/PORTA/PORTB clear; TRIS and
     * OPTION control registers reset to 1 = input / option POR. */
    pic16f5x_sim_sfr[PIC_REG_STATUS]   = PIC_STATUS_POR_VALUE;
    pic16f5x_sim_trisa = 0xFFU;
    pic16f5x_sim_trisb = 0xFFU;
#if PIC16F5X_FAMILY_HAS_PORTC
    pic16f5x_sim_trisc = 0xFFU;
#endif
#if PIC16F5X_FAMILY_HAS_PORTD
    pic16f5x_sim_trisd = 0xFFU;
#endif
#if PIC16F5X_FAMILY_HAS_PORTE
    pic16f5x_sim_trise = 0xFFU;
#endif
    pic16f5x_sim_option = PIC_OPTION_POR_VALUE;
    memset(sim_input_override, 0, sizeof sim_input_override);
    memset(sim_input_value,    0, sizeof sim_input_value);
}

/* Timer0 step. */

/**
 * @brief Advance the Timer0 model by one instruction cycle: apply the
 *        prescaler from the OPTION shadow, increment TMR0.
 */
static void sim_step_timer0(void)
{
    uint8_t option = pic16f5x_sim_option;
    uint8_t ps     = option & PIC_OPTION_PS_MASK;
    uint8_t psa    = (option >> 3) & 0x01U;

    static uint16_t t0_prescaler = 0U;
    /* PSA=1 assigns the prescaler to the WDT; TMR0 then runs with no
     * prescaler, so psa does not gate the counter here. */
    (void)psa;

    /* OPTION<PS2:PS0> prescaler mapping (DS41213D §5.0, Table 5-1):
     * 1:2..1:256, matching the driver's ps_ratio table. The 1:256
     * entry is 256, not 255 (the 1-byte counter window would make
     * 255 a one-tick-slow alias; MPLAB SIM and XC8 agree on 256). */
    static const uint16_t ps_idx[8] = {2, 4, 8, 16, 32, 64, 128, 256};
    uint32_t rate = ps_idx[ps];

    t0_prescaler++;
    if (t0_prescaler < rate) return;
    t0_prescaler = 0U;

    uint8_t t0 = pic16f5x_sim_sfr[PIC_REG_TMR0];
    t0++;
    pic16f5x_sim_sfr[PIC_REG_TMR0] = t0;
}

/**
 * @brief Advance the simulation by `ticks` instruction cycles.
 * @param ticks the number of instruction cycles to advance.
 */
void pic16f5x_sim_step(uint32_t ticks)
{
    for (uint32_t i = 0; i < ticks; i++)
    {
        sim_step_timer0();
    }
}

/**
 * @brief Implemented-pin mask for a port letter, mirroring the
 *        target driver's port_pin_mask (see pic16f5x_gpio.c).
 * @param port the port letter ('A'..'E').
 * @return the bitmask of implemented pins.
 */
static uint8_t port_pin_mask(char port)
{
#if PIC16F5X_FAMILY_HAS_PORTE
    if (port == 'E' || port == 'e') return 0xF0U;
#endif
#if PIC16F5X_FAMILY_HAS_PORTA
    if (port == 'A' || port == 'a') return 0x0FU;
#endif
#if PIC16F5X_FAMILY_HAS_SIXBIT_PORTS
    if (port == 'B' || port == 'b' || port == 'C' || port == 'c')
    {
        return 0x3FU;
    }
#endif
    return 0xFFU;
}

/**
 * @brief Drive a digital input pin from the test rig.
 * @param port the port letter, 'A' or 'B'.
 * @param pin the pin number, 0..7.
 * @param level 0 = low, 1 = high.
 */
void pic16f5x_sim_drive_input(char port, uint8_t pin, uint8_t level)
{
    if (pin > 7U) return;
    uint8_t mask = (uint8_t)(1U << pin);
    /* Pins the die does not implement are a no-op, exactly as on
     * silicon (DS41213D/DS41319 port tables). */
    if ((mask & port_pin_mask(port)) == 0U) return;
    uint8_t idx = port_index(port);
    sim_input_override[idx] |= mask;
    if (level) sim_input_value[idx] |= mask;
    else       sim_input_value[idx] &= (uint8_t)~mask;

    /* Also update the PORT register so EPIC_GPIO_ReadPin sees the
     * externally driven value for input pins, matching real hardware
     * (TRIS=1 reads return the pin's external state). */
    uint16_t pa = port_reg(port);
    uint8_t portval = pic16f5x_sim_sfr[pa];
    if (level) portval |= mask;
    else portval &= (uint8_t)~mask;
    pic16f5x_sim_sfr[pa] = portval;
}

/**
 * @brief Read the level currently driven onto a pin.
 * @param port the port letter, 'A' or 'B'.
 * @param pin the pin number, 0..7.
 * @return the pin level, 0 or 1 (0 for an invalid pin).
 */
uint8_t pic16f5x_sim_read_output(char port, uint8_t pin)
{
    if (pin > 7U) return 0U;
    uint8_t mask = (uint8_t)(1U << pin);
    if ((mask & port_pin_mask(port)) == 0U) return 0U;
    uint8_t idx  = port_index(port);
    uint8_t tris = pic16f5x_sim_trisb;
#if PIC16F5X_FAMILY_HAS_PORTA
    if (port == 'A' || port == 'a') tris = pic16f5x_sim_trisa;
#endif
#if PIC16F5X_FAMILY_HAS_PORTC
    if (port == 'C' || port == 'c') tris = pic16f5x_sim_trisc;
#endif
#if PIC16F5X_FAMILY_HAS_PORTD
    if (port == 'D' || port == 'd') tris = pic16f5x_sim_trisd;
#endif
#if PIC16F5X_FAMILY_HAS_PORTE
    if (port == 'E' || port == 'e') tris = pic16f5x_sim_trise;
#endif

    if (tris & mask)
    {
        return (sim_input_value[idx] & mask) ? 1U : 0U;
    }
    /* Output pin: report the latch. */
    uint16_t pa = port_reg(port);
    return (pic16f5x_sim_sfr[pa] & mask) ? 1U : 0U;
}
