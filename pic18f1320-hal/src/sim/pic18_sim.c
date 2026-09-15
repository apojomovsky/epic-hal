/*
 * PIC18F1320 host simulation backend (CMake host build only). Provides
 * `pic18_sim_sfr[]`, the 4096-byte register file the host SFR macros
 * dereference, and the hooks in `pic18f1320_sim.h`. Foundation phase
 * only: Timer0 stepping and GPIOA/GPIOB.
 */

#include "pic18f1320_sim.h"
#include "pic18f1320_sfr.h"
#include "pic18_platform.h"

#include <string.h>

/* 4096-byte memory-backed register file, referenced by
 * include/host/pic18_platform.h. Provisionally the full 12-bit data-memory
 * footprint; all SFRs the drivers use live in 0xF60-0xFFF. */
uint8_t pic18_sim_sfr[0x1000];

/* Per-pin input overrides set by the host application (A, B). */
static uint8_t sim_input_override[2] = {0};
static uint8_t sim_input_value   [2] = {0};

/* Optional ISR hook (the family dispatcher, registered by the harness). */
static pic18_sim_irq_cb_t sim_irq_cb = 0;

/**
 * @brief Advance the simulated Timer0 by one instruction cycle.
 */
static void sim_step_timer0(void);

/**
 * @brief Map a port letter (A, B, case-insensitive) to a 0-based index.
 *
 * Unknown letters map to index 0, matching port A.
 *
 * @param port the port letter to map
 * @return the 0-based port index (0..1)
 */
static uint8_t port_index(char port)
{
    switch (port)
    {
        case 'B': case 'b': return 1;
        default:            return 0;
    }
}

/**
 * @brief Return the register-file address of the LAT register for a port.
 *
 * @param port the port letter (A, B, case-insensitive)
 * @return the register-file index of the port's LAT register, or LATA for
 *         an unknown port
 */
static uint16_t lat_addr(char port)
{
    return (port == 'B' || port == 'b') ? PIC_REG_LATB : PIC_REG_LATA;
}

/**
 * @brief Return the register-file address of the TRIS register for a port.
 *
 * @param port the port letter (A, B, case-insensitive)
 * @return the register-file index of the port's TRIS register, or TRISA
 *         for an unknown port
 */
static uint16_t tris_addr(char port)
{
    return (port == 'B' || port == 'b') ? PIC_REG_TRISB : PIC_REG_TRISA;
}

/**
 * @brief Return the register-file address of the PORT register for a port.
 *
 * @param port the port letter (A, B, case-insensitive)
 * @return the register-file index of the port's PORT register, or PORTA
 *         for an unknown port
 */
static uint16_t port_addr(char port)
{
    return (port == 'B' || port == 'b') ? PIC_REG_PORTB : PIC_REG_PORTA;
}

/**
 * @brief Reset the simulated device to its power-on state.
 *
 * Loads the datasheet POR values into the register file, clears the input
 * overrides, and the IRQ callback.
 */
void pic18_sim_reset(void)
{
    memset(pic18_sim_sfr, 0, sizeof pic18_sim_sfr);

    /* Power-on reset values, DS39605F Table 5-1 + Register 4-1. */
    pic18_sim_sfr[PIC_REG_STATUS]   = PIC_STATUS_POR_VALUE;   /* 0x00 */
    pic18_sim_sfr[PIC_REG_BSR]      = PIC_BSR_POR_VALUE;      /* 0x00 */
    pic18_sim_sfr[PIC_REG_RCON]     = PIC_RCON_POR_VALUE;     /* 0x57 */
    pic18_sim_sfr[PIC_REG_INTCON]   = PIC_INTCON_POR_VALUE;   /* 0x00 */
    pic18_sim_sfr[PIC_REG_INTCON2]  = PIC_INTCON2_POR_VALUE;  /* 0xFB */
    pic18_sim_sfr[PIC_REG_INTCON3]  = PIC_INTCON3_POR_VALUE;  /* 0xC0 */
    pic18_sim_sfr[PIC_REG_PIR1]     = PIC_PIR1_POR_VALUE;     /* 0x00 */
    pic18_sim_sfr[PIC_REG_PIE1]     = PIC_PIE1_POR_VALUE;     /* 0x00 */
    pic18_sim_sfr[PIC_REG_IPR1]     = PIC_IPR1_POR_VALUE;     /* 0xFF */
    pic18_sim_sfr[PIC_REG_PIR2]     = PIC_PIR2_POR_VALUE;     /* 0x00 */
    pic18_sim_sfr[PIC_REG_PIE2]     = PIC_PIE2_POR_VALUE;     /* 0x00 */
    pic18_sim_sfr[PIC_REG_IPR2]     = PIC_IPR2_POR_VALUE;     /* 0xFF */
    pic18_sim_sfr[PIC_REG_T0CON]    = PIC_T0CON_POR_VALUE;    /* 0xFF */

    /* TRIS defaults: 1 = input. Both ports are full 8-bit on this part. */
    pic18_sim_sfr[PIC_REG_TRISA] = PIC_TRIS_POR_VALUE;
    pic18_sim_sfr[PIC_REG_TRISB] = PIC_TRIS_POR_VALUE;
    pic18_sim_sfr[PIC_REG_LATA]  = PIC_LAT_POR_VALUE;
    pic18_sim_sfr[PIC_REG_LATB]  = PIC_LAT_POR_VALUE;

    memset(sim_input_override, 0, sizeof sim_input_override);
    memset(sim_input_value,    0, sizeof sim_input_value);
    sim_irq_cb = 0;
}

/**
 * @brief Advance the simulated device by a number of instruction cycles.
 *
 * Each cycle steps Timer0.
 *
 * @param ticks the number of instruction cycles to simulate
 */
void pic18_sim_step(uint32_t ticks)
{
    for (uint32_t i = 0; i < ticks; i++)
    {
        sim_step_timer0();
    }
}

/**
 * @brief Step the simulated Timer0 by one instruction cycle.
 *
 * Applies the T0CON prescaler, increments the timer in 8- or 16-bit mode
 * per T08BIT, and raises TMR0IF on overflow.
 */
static void sim_step_timer0(void)
{
    /* T0CON layout (DS39605F Register 10-1):
     *   bit 7  TMR0ON
     *   bit 6  T08BIT (1 = 8-bit)
     *   bit 5  T0CS
     *   bit 4  T0SE
     *   bit 3  PSA   (1 = prescaler not assigned -> raw clock)
     *   bit 2..0 T0PS2:T0PS0
     */
    uint8_t t0con = pic18_sim_sfr[PIC_REG_T0CON];
    if (!(t0con & PIC_T0CON_TMR0ON)) return;

    uint8_t ps  = (uint8_t)(t0con & PIC_T0CON_T0PS_MASK);
    uint8_t psa = (t0con & PIC_T0CON_PSA) ? 1U : 0U;

    /* Prescaler ratio, DS39605F Table 10-1. PSA = 1 -> raw (1:1). uint16_t
     * so the 1:256 entry (256) is not truncated. */
    static const uint16_t ps_idx[8] = {2, 4, 8, 16, 32, 64, 128, 256};
    uint32_t rate = psa ? 1U : ps_idx[ps];

    static uint16_t t0_prescaler = 0U;
    t0_prescaler++;
    if (t0_prescaler < rate) return;
    t0_prescaler = 0U;

    if (t0con & PIC_T0CON_T08BIT)
    {
        /* 8-bit mode: increment TMR0L. */
        uint8_t t0 = (uint8_t)(pic18_sim_sfr[PIC_REG_TMR0L] + 1U);
        pic18_sim_sfr[PIC_REG_TMR0L] = t0;
        if (t0 == 0x00U)
        {
            pic18_sim_sfr[PIC_REG_INTCON] |= PIC_INTCON_TMR0IF;
            if (sim_irq_cb) sim_irq_cb();
        }
    }
    else
    {
        /* 16-bit mode: increment TMR0H:TMR0L. */
        uint16_t full = (uint16_t)(((uint16_t)pic18_sim_sfr[PIC_REG_TMR0H] << 8) |
                                   pic18_sim_sfr[PIC_REG_TMR0L]);
        full++;
        pic18_sim_sfr[PIC_REG_TMR0L] = (uint8_t)(full & 0xFFU);
        pic18_sim_sfr[PIC_REG_TMR0H] = (uint8_t)(full >> 8);
        if (full == 0U)
        {
            pic18_sim_sfr[PIC_REG_INTCON] |= PIC_INTCON_TMR0IF;
            if (sim_irq_cb) sim_irq_cb();
        }
    }
}

/**
 * @brief Drive a port pin to an external input level.
 *
 * Records the override so input-pin reads return the level, and updates
 * PORTx to match real hardware (PORT reads return pin state when TRIS=1).
 *
 * @param port the port letter (A, B)
 * @param pin the pin number (0..7); values above 7 are ignored
 * @param level the level to drive (nonzero = high, zero = low)
 */
void pic18_sim_drive_input(char port, uint8_t pin, uint8_t level)
{
    if (pin > 7U) return;
    uint8_t idx  = port_index(port);
    uint8_t mask = (uint8_t)(1U << pin);
    sim_input_override[idx] |= mask;
    if (level) sim_input_value[idx] |= mask;
    else       sim_input_value[idx] &= (uint8_t)~mask;

    /* Also update PORTx so EPIC_GPIO_ReadPin sees the external level on
     * input pins, matching real hardware (PORT reads return pin state
     * when TRIS=1). */
    uint16_t pa = port_addr(port);
    uint8_t portval = pic18_sim_sfr[pa];
    if (level) portval |= mask;
    else portval &= (uint8_t)~mask;
    pic18_sim_sfr[pa] = portval;
}

/**
 * @brief Read the current logic level of a port pin.
 *
 * Returns the externally driven level for pins configured as inputs and
 * the LATx bit for pins configured as outputs.
 *
 * @param port the port letter (A, B)
 * @param pin the pin number (0..7); values above 7 read as 0
 * @return 1 if the pin reads high, else 0
 */
uint8_t pic18_sim_read_output(char port, uint8_t pin)
{
    if (pin > 7U) return 0U;
    uint8_t idx  = port_index(port);
    uint8_t mask = (uint8_t)(1U << pin);
    uint8_t tris = pic18_sim_sfr[tris_addr(port)];

    if (tris & mask)
    {
        /* Input: return the externally driven level (0 if not driven). */
        return (sim_input_override[idx] & mask) ?
               ((sim_input_value[idx] & mask) ? 1U : 0U) : 0U;
    }
    /* Output: return the LATx bit (DS39605F §5.0/§6.0). */
    return (pic18_sim_sfr[lat_addr(port)] & mask) ? 1U : 0U;
}

/**
 * @brief Register the ISR hook invoked when a simulated interrupt fires.
 *
 * @param cb the callback to invoke on a simulated interrupt, or 0 for none
 */
void pic18_sim_set_irq_callback(pic18_sim_irq_cb_t cb)
{
    sim_irq_cb = cb;
}
