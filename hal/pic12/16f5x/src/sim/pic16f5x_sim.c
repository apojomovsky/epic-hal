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

/* Input overrides per port (A, B, and C where the die has it; the
 * 16F506's C2 inputs live on RC0/RC1). */
#if PIC16F5X_FAMILY_HAS_PORTC
static uint8_t sim_input_override[3] = {0};
static uint8_t sim_input_value   [3] = {0};
#else
static uint8_t sim_input_override[2] = {0};
static uint8_t sim_input_value   [2] = {0};
#endif

#if PIC16F5X_FAMILY_HAS_COMP_ADC
/* Driven analog levels per CHS channel (AN0..AN2, 0.6V slot) and
 * the conversion countdown in instruction cycles. */
static uint8_t sim_analog[4] = {0};
static uint8_t sim_adc_converting = 0U;
static uint8_t sim_adc_ticks_left = 0U;
#endif

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
 * @brief Map a port letter to the override-array index.
 * @param port the port letter, 'A'..'C'.
 * @return the index 0..2 (1 for an invalid port).
 */
static uint8_t port_index(char port)
{
    switch (port)
    {
#if PIC16F5X_FAMILY_HAS_PORTA
        case 'A': case 'a': return 0;
#endif
        case 'B': case 'b': return 1;
#if PIC16F5X_FAMILY_HAS_PORTC
        case 'C': case 'c': return 2;
#endif
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
#if PIC16F5X_FAMILY_HAS_COMP_ADC
    /* 16F506 analog block POR (DS41268D Table 4-2); VRCON 0x00 is
     * the EDC + MPLAB SIM value (see pic16f5x_sfr.h). */
    pic16f5x_sim_sfr[PIC_REG_CM1CON0] = PIC_CM1CON0_POR_VALUE;
    pic16f5x_sim_sfr[PIC_REG_CM2CON0] = PIC_CM2CON0_POR_VALUE;
    pic16f5x_sim_sfr[PIC_REG_ADCON0]  = PIC_ADCON0_POR_VALUE;
    pic16f5x_sim_sfr[PIC_REG_VRCON]   = PIC_VRCON_POR_VALUE;
    sim_adc_converting = 0U;
    sim_adc_ticks_left = 0U;
    memset(sim_analog, 0, sizeof sim_analog);
#endif
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

#if PIC16F5X_FAMILY_HAS_COMP_ADC
/**
 * @brief Recompute C1OUT/C2OUT from the driven pin levels. C1
 *        compares RB0 (+) against RB1 (-), C2 RC0 (+) against RC1
 *        (the POR mux selects the pins; the 0.6V/CVREF selects are
 *        not modeled). Disabled forces 0 (DS41268D Figure 7-1/7-2
 *        note 1); CxPOL=1 is not inverted (Register 7-2/7-3).
 */
static void sim_step_comp(void)
{
    uint8_t portb = pic16f5x_sim_sfr[PIC_REG_PORTB];
    uint8_t portc = pic16f5x_sim_sfr[PIC_REG_PORTC];
    uint8_t cm1 = pic16f5x_sim_sfr[PIC_REG_CM1CON0];
    uint8_t cm2 = pic16f5x_sim_sfr[PIC_REG_CM2CON0];
    uint8_t raw1 = (((portb >> 0) & 1U) > ((portb >> 1) & 1U)) ? 1U : 0U;
    uint8_t raw2 = (((portc >> 0) & 1U) > ((portc >> 1) & 1U)) ? 1U : 0U;
    uint8_t out1 = 0U;
    uint8_t out2 = 0U;
    if (cm1 & PIC_CM1CON0_C1ON)
    {
        out1 = raw1;
        if ((cm1 & PIC_CM1CON0_C1POL) == 0U) out1 ^= 1U;
    }
    if (cm2 & PIC_CM2CON0_C2ON)
    {
        out2 = raw2;
        if ((cm2 & PIC_CM2CON0_C2POL) == 0U) out2 ^= 1U;
    }
    if (out1) cm1 |= PIC_CM1CON0_C1OUT;
    else      cm1 &= (uint8_t)~PIC_CM1CON0_C1OUT;
    if (out2) cm2 |= PIC_CM2CON0_C2OUT;
    else      cm2 &= (uint8_t)~PIC_CM2CON0_C2OUT;
    pic16f5x_sim_sfr[PIC_REG_CM1CON0] = cm1;
    pic16f5x_sim_sfr[PIC_REG_CM2CON0] = cm2;
}

/**
 * @brief Advance the ADC by one instruction cycle. A GO rising edge
 *        arms a 13-TAD countdown (TAD in instruction cycles from
 *        ADCS: FOSC/16 takes 4, FOSC/8 takes 2, the /4 pair takes 1;
 *        §9.1); expiry latches the driven level into ADRES and
 *        clears GO. GO cleared early or ADON off aborts, ADRES kept.
 */
static void sim_step_adc(void)
{
    static const uint8_t tad_ticks[4] = {4U, 2U, 1U, 1U};
    uint8_t adcon0 = pic16f5x_sim_sfr[PIC_REG_ADCON0];
    if ((adcon0 & PIC_ADCON0_ADON) == 0U)
    {
        sim_adc_converting = 0U;
        return;
    }
    if ((adcon0 & PIC_ADCON0_GO) == 0U)
    {
        sim_adc_converting = 0U;
        return;
    }
    if (sim_adc_converting == 0U)
    {
        uint8_t adcs = (uint8_t)((adcon0 & PIC_ADCON0_ADCS_MASK) >> PIC_ADCON0_ADCS_POS);
        sim_adc_converting = 1U;
        sim_adc_ticks_left = (uint8_t)(13U * tad_ticks[adcs]);
    }
    sim_adc_ticks_left--;
    if (sim_adc_ticks_left != 0U) return;
    sim_adc_converting = 0U;
    pic16f5x_sim_sfr[PIC_REG_ADRES] =
        sim_analog[(adcon0 & PIC_ADCON0_CHS_MASK) >> PIC_ADCON0_CHS_POS];
    pic16f5x_sim_sfr[PIC_REG_ADCON0] = (uint8_t)(adcon0 & (uint8_t)~PIC_ADCON0_GO);
}
#endif

#if PIC16F5X_FAMILY_HAS_COMP_ADC
/**
 * @brief Drive an analog channel level from the test rig.
 * @param channel the CHS channel, 0..3 (AN0..AN2, 0.6V slot).
 * @param level the 8-bit level the next conversion latches.
 */
void pic16f5x_sim_drive_analog(uint8_t channel, uint8_t level)
{
    if (channel > 3U) return;
    sim_analog[channel] = level;
}
#endif

/**
 * @brief Advance the simulation by `ticks` instruction cycles.
 * @param ticks the number of instruction cycles to advance.
 */
void pic16f5x_sim_step(uint32_t ticks)
{
    for (uint32_t i = 0; i < ticks; i++)
    {
        sim_step_timer0();
#if PIC16F5X_FAMILY_HAS_COMP_ADC
        sim_step_comp();
        sim_step_adc();
#endif
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
     * silicon (DS41213D/DS41236C/DS41268D port tables). */
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
#if PIC16F5X_FAMILY_HAS_COMP_ADC
    sim_step_comp();
#endif
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
