/*
 * PIC18F2455 family host simulation backend, linked by the CMake host
 * build only. Provides `pic18_sim_sfr[]`, the 4096-byte memory-backed
 * register file the host SFR macros (`include/host/pic18_platform.h`)
 * dereference, and the hooks declared in `pic18fxx5x_sim.h`. Every SFR
 * the drivers touch is in the Access Bank (0xF60-0xFFF), so it is just
 * an index into this array, no BSR translation needed.
 */

#include "pic18fxx5x_sim.h"
#include "pic18fxx5x_sfr.h"
#include "pic18_platform.h"

#include <string.h>

/* 4096-byte memory-backed register file, referenced by
 * include/host/pic18_platform.h. Provisionally the full 12-bit data-memory
 * footprint; all SFRs the drivers use live in 0xF60-0xFFF. */
uint8_t pic18_sim_sfr[0x1000];

/* Per-pin input overrides set by the host application (A..E). */
static uint8_t sim_input_override[5] = {0};
static uint8_t sim_input_value   [5] = {0};

/* Simulated data EEPROM cell storage (256 bytes, DS39632E §7.0). */
static uint8_t sim_eeprom[256] = {0};

/* Optional ISR hook (the family dispatcher, registered by the harness). */
static pic18_sim_irq_cb_t sim_irq_cb = 0;

/**
 * @brief Advance the simulated Timer0 by one instruction cycle.
 */
static void sim_step_timer0(void);
/**
 * @brief Advance the simulated Timer1 by one instruction cycle.
 */
static void sim_step_timer1(void);
/**
 * @brief Advance the simulated Timer2 by one instruction cycle.
 */
static void sim_step_timer2(void);
/**
 * @brief Advance the simulated Timer3 by one instruction cycle.
 */
static void sim_step_timer3(void);
/**
 * @brief Advance the simulated EUSART state machine by one instruction cycle.
 */
static void sim_step_usart(void);

/**
 * @brief Map a port letter (A..E, case-insensitive) to a 0-based index.
 *
 * Unknown letters map to index 0, matching port A.
 *
 * @param port the port letter to map
 * @return the 0-based port index (0..4)
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

/**
 * @brief Return the register-file address of the LAT register for a port.
 *
 * @param port the port letter (A..E, case-insensitive)
 * @return the register-file index of the port's LAT register, or LATA for
 *         unknown or unpopulated ports
 */
static uint16_t lat_addr(char port)
{
    switch (port) {
        case 'A': case 'a': return PIC_REG_LATA;
        case 'B': case 'b': return PIC_REG_LATB;
        case 'C': case 'c': return PIC_REG_LATC;
#if PIC18FXX5X_FAMILY_HAS_PORTD
        case 'D': case 'd': return PIC_REG_LATD;
#endif
#if PIC18FXX5X_FAMILY_HAS_PORTE
        case 'E': case 'e': return PIC_REG_LATE;
#endif
        default:             return PIC_REG_LATA;
    }
}

/**
 * @brief Return the register-file address of the TRIS register for a port.
 *
 * @param port the port letter (A..E, case-insensitive)
 * @return the register-file index of the port's TRIS register, or TRISA
 *         for unknown or unpopulated ports
 */
static uint16_t tris_addr(char port)
{
    switch (port) {
        case 'A': case 'a': return PIC_REG_TRISA;
        case 'B': case 'b': return PIC_REG_TRISB;
        case 'C': case 'c': return PIC_REG_TRISC;
#if PIC18FXX5X_FAMILY_HAS_PORTD
        case 'D': case 'd': return PIC_REG_TRISD;
#endif
#if PIC18FXX5X_FAMILY_HAS_PORTE
        case 'E': case 'e': return PIC_REG_TRISE;
#endif
        default:             return PIC_REG_TRISA;
    }
}

/**
 * @brief Return the register-file address of the PORT register for a port.
 *
 * @param port the port letter (A..E, case-insensitive)
 * @return the register-file index of the port's PORT register, or PORTA
 *         for unknown or unpopulated ports
 */
static uint16_t port_addr(char port)
{
    switch (port) {
        case 'A': case 'a': return PIC_REG_PORTA;
        case 'B': case 'b': return PIC_REG_PORTB;
        case 'C': case 'c': return PIC_REG_PORTC;
#if PIC18FXX5X_FAMILY_HAS_PORTD
        case 'D': case 'd': return PIC_REG_PORTD;
#endif
#if PIC18FXX5X_FAMILY_HAS_PORTE
        case 'E': case 'e': return PIC_REG_PORTE;
#endif
        default:             return PIC_REG_PORTA;
    }
}

/**
 * @brief Reset the simulated device to its power-on state.
 *
 * Loads the datasheet POR values into the register file, clears the input
 * overrides, the data EEPROM cells, and the IRQ callback, and sets
 * PIR1<TXIF> as on real hardware.
 */
void pic18_sim_reset(void)
{
    memset(pic18_sim_sfr, 0, sizeof pic18_sim_sfr);

    /* Power-on reset values, DS39632E Table 5-1 + Register 4-1. */
    pic18_sim_sfr[PIC_REG_STATUS]   = PIC_STATUS_POR_VALUE;   /* 0x00 */
    pic18_sim_sfr[PIC_REG_BSR]      = PIC_BSR_POR_VALUE;      /* 0x00 */
    pic18_sim_sfr[PIC_REG_RCON]     = PIC_RCON_POR_VALUE;     /* 0x57 */
    pic18_sim_sfr[PIC_REG_INTCON]   = PIC_INTCON_POR_VALUE;   /* 0x00 */
    pic18_sim_sfr[PIC_REG_INTCON2]  = PIC_INTCON2_POR_VALUE;  /* 0xFB */
    pic18_sim_sfr[PIC_REG_INTCON3]  = PIC_INTCON3_POR_VALUE;  /* 0xC0 */
    pic18_sim_sfr[PIC_REG_PIR1]     = PIC_PIR1_POR_VALUE;     /* 0x00 */
    pic18_sim_sfr[PIC_REG_PIE1]     = PIC_PIE1_POR_VALUE;     /* 0x00 */
    pic18_sim_sfr[PIC_REG_IPR1]     = PIC_IPR1_POR_VALUE;     /* 0xFF */
    pic18_sim_sfr[PIC_REG_T0CON]    = PIC_T0CON_POR_VALUE;    /* 0xFF */
    pic18_sim_sfr[PIC_REG_T1CON]    = PIC_T1CON_POR_VALUE;    /* 0x00 */
    pic18_sim_sfr[PIC_REG_T2CON]    = PIC_T2CON_POR_VALUE;    /* 0x00 */
    pic18_sim_sfr[PIC_REG_T3CON]    = PIC_T3CON_POR_VALUE;    /* 0x00 */
    pic18_sim_sfr[PIC_REG_PR2]     = PIC_PR2_POR_VALUE;       /* 0xFF */
    pic18_sim_sfr[PIC_REG_PIR2]    = PIC_PIR2_POR_VALUE;      /* 0x00 */
    pic18_sim_sfr[PIC_REG_PIE2]    = PIC_PIE2_POR_VALUE;      /* 0x00 */
    pic18_sim_sfr[PIC_REG_IPR2]    = PIC_IPR2_POR_VALUE;      /* 0xFF */

    /* EUSART reset values (DS39632E Table 5-1). TXSTA resets to 0x02
     * (TRMT=1, TSR empty); the rest are clear. PIR1<TXIF> is a level, not
     * a latched flag: it reads 1 after POR because TXREG is empty
     * (§20.2.1), even though Table 5-1 lists PIR1 = 0x00. */
    pic18_sim_sfr[PIC_REG_BAUDCON] = PIC_BAUDCON_POR_VALUE;  /* 0x00 */
    pic18_sim_sfr[PIC_REG_RCSTA]   = PIC_RCSTA_POR_VALUE;    /* 0x00 */
    pic18_sim_sfr[PIC_REG_TXSTA]   = PIC_TXSTA_POR_VALUE;    /* 0x02 */
    pic18_sim_sfr[PIC_REG_SPBRG]   = PIC_SPBRG_POR_VALUE;    /* 0x00 */
    pic18_sim_sfr[PIC_REG_SPBRGH]  = PIC_SPBRGH_POR_VALUE;   /* 0x00 */

    /* Comparator: CMCON resets to 0x07 (comparators off, DS39632E Fig 22-1). */
    pic18_sim_sfr[PIC_REG_CMCON]   = PIC_CMCON_POR_VALUE;    /* 0x07 */

    /* A/D: ADCON0/1/2 reset to 0x00 (module off, DS39632E Table 5-1). */
    pic18_sim_sfr[PIC_REG_ADCON0]  = PIC_ADCON0_POR_VALUE;   /* 0x00 */
    pic18_sim_sfr[PIC_REG_ADCON1]  = PIC_ADCON1_POR_VALUE;   /* 0x00 */
    pic18_sim_sfr[PIC_REG_ADCON2]  = PIC_ADCON2_POR_VALUE;   /* 0x00 */
#if PIC18FXX5X_FAMILY_HAS_SPP
    /* SPP (40/44-pin only): all registers reset to 0x00. */
    pic18_sim_sfr[PIC_REG_SPPCON]  = PIC_SPPCON_POR_VALUE;
    pic18_sim_sfr[PIC_REG_SPPCFG]  = PIC_SPPCFG_POR_VALUE;
    pic18_sim_sfr[PIC_REG_SPPEPS]  = PIC_SPPEPS_POR_VALUE;
#endif

    /* PIR1<TXIF> reads 1 right after POR (TXREG empty, §20.2.1) even
     * though Table 5-1 lists PIR1 = 0x00; the PIC16 sim models it the
     * same way. */
    pic18_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_TXIF;

    /* TRIS defaults: 1 = input. PORTA is 6-bit, PORTE 3-bit. */
    pic18_sim_sfr[PIC_REG_TRISA] = 0x3FU;
    pic18_sim_sfr[PIC_REG_TRISB] = PIC_TRIS_POR_VALUE;
    pic18_sim_sfr[PIC_REG_TRISC] = PIC_TRIS_POR_VALUE;
#if PIC18FXX5X_FAMILY_HAS_PORTD
    pic18_sim_sfr[PIC_REG_TRISD] = PIC_TRIS_POR_VALUE;
#endif
#if PIC18FXX5X_FAMILY_HAS_PORTE
    pic18_sim_sfr[PIC_REG_TRISE] = 0x07U;
#endif
    pic18_sim_sfr[PIC_REG_LATA] = PIC_LAT_POR_VALUE;
    pic18_sim_sfr[PIC_REG_LATB] = PIC_LAT_POR_VALUE;
    pic18_sim_sfr[PIC_REG_LATC] = PIC_LAT_POR_VALUE;
#if PIC18FXX5X_FAMILY_HAS_PORTD
    pic18_sim_sfr[PIC_REG_LATD] = PIC_LAT_POR_VALUE;
#endif
#if PIC18FXX5X_FAMILY_HAS_PORTE
    pic18_sim_sfr[PIC_REG_LATE] = PIC_LAT_POR_VALUE;
#endif

    memset(sim_input_override, 0, sizeof sim_input_override);
    memset(sim_input_value,    0, sizeof sim_input_value);
    memset(sim_eeprom,         0, sizeof sim_eeprom);
    sim_irq_cb = 0;
}

/**
 * @brief Advance the simulated device by a number of instruction cycles.
 *
 * Each cycle steps the enabled timers and the EUSART state machine.
 *
 * @param ticks the number of instruction cycles to simulate
 */
void pic18_sim_step(uint32_t ticks)
{
    for (uint32_t i = 0; i < ticks; i++) {
        sim_step_timer0();
        sim_step_timer1();
        sim_step_timer2();
        sim_step_timer3();
        sim_step_usart();
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
    /* T0CON layout (DS39632E Register 11-1):
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

    /* Prescaler ratio, DS39632E Table 11-1. PSA = 1 -> raw (1:1). uint16_t
     * so the 1:256 entry (256) is not truncated. */
    static const uint16_t ps_idx[8] = {2, 4, 8, 16, 32, 64, 128, 256};
    uint32_t rate = psa ? 1U : ps_idx[ps];

    static uint16_t t0_prescaler = 0U;
    t0_prescaler++;
    if (t0_prescaler < rate) return;
    t0_prescaler = 0U;

    if (t0con & PIC_T0CON_T08BIT) {
        /* 8-bit mode: increment TMR0L. */
        uint8_t t0 = (uint8_t)(pic18_sim_sfr[PIC_REG_TMR0L] + 1U);
        pic18_sim_sfr[PIC_REG_TMR0L] = t0;
        if (t0 == 0x00U) {
            pic18_sim_sfr[PIC_REG_INTCON] |= PIC_INTCON_TMR0IF;
            if (sim_irq_cb) sim_irq_cb();
        }
    } else {
        /* 16-bit mode: increment TMR0H:TMR0L. */
        uint16_t full = (uint16_t)(((uint16_t)pic18_sim_sfr[PIC_REG_TMR0H] << 8) |
                                   pic18_sim_sfr[PIC_REG_TMR0L]);
        full++;
        pic18_sim_sfr[PIC_REG_TMR0L] = (uint8_t)(full & 0xFFU);
        pic18_sim_sfr[PIC_REG_TMR0H] = (uint8_t)(full >> 8);
        if (full == 0U) {
            pic18_sim_sfr[PIC_REG_INTCON] |= PIC_INTCON_TMR0IF;
            if (sim_irq_cb) sim_irq_cb();
        }
    }
}


/**
 * @brief Step the simulated Timer1 by one instruction cycle.
 *
 * Applies the T1CON prescaler, increments the 16-bit timer value, and
 * raises TMR1IF on overflow.
 */
static void sim_step_timer1(void)
{
    /* T1CON layout (DS39632E Register 12-1):
     *   bit 7  RD16
     *   bit 6  T1RUN (status, RO)
     *   bit 5..4 T1CKPS1:T1CKPS0
     *   bit 3  T1OSCEN
     *   bit 2  T1SYNC
     *   bit 1  TMR1CS
     *   bit 0  TMR1ON
     */
    uint8_t t1con = pic18_sim_sfr[PIC_REG_T1CON];
    if (!(t1con & PIC_T1CON_TMR1ON)) return;

    /* Prescaler 1:1/1:2/1:4/1:8 (T1CKPS1:T1CKPS0). TMR1CS = 1 (external/T1OSC):
     * the sim does not model a real external signal, so it advances at the
     * configured prescaler rate per instruction cycle (lets T1OSC-based
     * firmware run on the host with the same Timer1 config a real target
     * uses; the 32 kHz crystal's actual rate is not reproduced). */
    static const uint8_t ps_idx[4] = {1, 2, 4, 8};
    uint32_t rate = ps_idx[(t1con >> 4) & 0x3U];

    static uint8_t t1_prescaler = 0U;
    t1_prescaler++;
    if (t1_prescaler < rate) return;
    t1_prescaler = 0U;

    /* 16-bit increment (the sim ignores RD16 latching; it reads both bytes
     * atomically). */
    uint16_t full = (uint16_t)(((uint16_t)pic18_sim_sfr[PIC_REG_TMR1H] << 8) |
                               pic18_sim_sfr[PIC_REG_TMR1L]);
    full++;
    pic18_sim_sfr[PIC_REG_TMR1L] = (uint8_t)(full & 0xFFU);
    pic18_sim_sfr[PIC_REG_TMR1H] = (uint8_t)(full >> 8);
    if (full == 0U) {
        pic18_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_TMR1IF;
        if (sim_irq_cb) sim_irq_cb();
    }
}


/**
 * @brief Step the simulated Timer2 by one instruction cycle.
 *
 * Applies the T2CON prescaler, increments TMR2 until it matches PR2, then
 * resets and raises TMR2IF after the postscaler.
 */
static void sim_step_timer2(void)
{
    /* T2CON layout (DS39632E Register 12-2):
     *   bit 6..3 T2OUTPS3:T2OUTPS0
     *   bit 2  TMR2ON
     *   bit 1..0 T2CKPS1:T2CKPS0
     */
    uint8_t t2con = pic18_sim_sfr[PIC_REG_T2CON];
    if (!(t2con & PIC_T2CON_TMR2ON)) return;

    /* T2CKPS1:T2CKPS0 -> 1:1, 1:4, 1:16, 1:16. */
    static const uint8_t pre_idx[4] = {1, 4, 16, 16};
    uint32_t pre = pre_idx[t2con & PIC_T2CON_T2CKPS_MASK];
    /* TOUTPS3:TOUTPS0 -> 1:(N+1). */
    uint8_t  post = (uint8_t)(((t2con & PIC_T2CON_TOUTPS_MASK) >> 3) + 1U);

    uint8_t pr2 = pic18_sim_sfr[PIC_REG_PR2];

    static uint16_t t2_prescaler = 0U;
    static uint8_t  t2_post      = 0U;

    t2_prescaler++;
    if (t2_prescaler < pre) return;
    t2_prescaler = 0U;

    /* TMR2 increments until it matches PR2, then resets (DS39632E §12.0);
     * TMR2IF fires after the postscaler, once per (PR2+1) prescaled cycles. */
    uint8_t t2 = (uint8_t)(pic18_sim_sfr[PIC_REG_TMR2] + 1U);
    if (t2 > pr2) {
        t2 = 0U;
        t2_post++;
        if (t2_post >= post) {
            t2_post = 0U;
            pic18_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_TMR2IF;
            if (sim_irq_cb) sim_irq_cb();
        }
    }
    pic18_sim_sfr[PIC_REG_TMR2] = t2;
}


/**
 * @brief Step the simulated Timer3 by one instruction cycle.
 *
 * Applies the T3CON prescaler, increments the 16-bit timer value, and
 * raises TMR3IF on overflow.
 */
static void sim_step_timer3(void)
{
    /* T3CON layout (DS39632E Register 14-1):
     *   bit 7  RD16
     *   bit 6  T3CCP2
     *   bit 5..4 T3CKPS1:T3CKPS0
     *   bit 3  T3CCP1
     *   bit 2  T3SYNC
     *   bit 1  TMR3CS
     *   bit 0  TMR3ON
     */
    uint8_t t3con = pic18_sim_sfr[PIC_REG_T3CON];
    if (!(t3con & PIC_T3CON_TMR3ON)) return;

    static const uint8_t ps_idx[4] = {1, 2, 4, 8};
    uint32_t rate = ps_idx[(t3con >> 4) & 0x3U];

    static uint8_t t3_prescaler = 0U;
    t3_prescaler++;
    if (t3_prescaler < rate) return;
    t3_prescaler = 0U;

    uint16_t full = (uint16_t)(((uint16_t)pic18_sim_sfr[PIC_REG_TMR3H] << 8) |
                               pic18_sim_sfr[PIC_REG_TMR3L]);
    full++;
    pic18_sim_sfr[PIC_REG_TMR3L] = (uint8_t)(full & 0xFFU);
    pic18_sim_sfr[PIC_REG_TMR3H] = (uint8_t)(full >> 8);
    if (full == 0U) {
        pic18_sim_sfr[PIC_REG_PIR2] |= PIC_PIR2_TMR3IF;
        if (sim_irq_cb) sim_irq_cb();
    }
}


/**
 * @brief Drive a port pin to an external input level.
 *
 * Records the override so input-pin reads return the level, and updates
 * PORTx to match real hardware (PORT reads return pin state when TRIS=1).
 *
 * @param port the port letter (A..E)
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
 * @param port the port letter (A..E)
 * @param pin the pin number (0..7); values above 7 read as 0
 * @return 1 if the pin reads high, else 0
 */
uint8_t pic18_sim_read_output(char port, uint8_t pin)
{
    if (pin > 7U) return 0U;
    uint8_t idx  = port_index(port);
    uint8_t mask = (uint8_t)(1U << pin);
    uint8_t tris = pic18_sim_sfr[tris_addr(port)];

    if (tris & mask) {
        /* Input: return the externally driven level (0 if not driven). */
        return (sim_input_override[idx] & mask) ?
               ((sim_input_value[idx] & mask) ? 1U : 0U) : 0U;
    }
    /* Output: return the LATx bit (DS39632E §10.0). */
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

/**
 * @brief Deliver a received SSP byte to the simulated hardware.
 *
 * Places the byte in SSPBUF, sets SSPSTAT<BF> and PIR1<SSPIF>, and raises
 * the IRQ hook.
 *
 * @param data the byte received on the SSP bus
 */
void pic18_sim_drive_ssp_rx(uint8_t data)
{
    /* Place the byte in SSPBUF, set SSPSTAT<BF> + PIR1<SSPIF>. */
    pic18_sim_sfr[PIC_REG_SSPBUF] = data;
    uint8_t stat = (uint8_t)(pic18_sim_sfr[PIC_REG_SSPSTAT] | PIC_SSPSTAT_BF);
    pic18_sim_sfr[PIC_REG_SSPSTAT] = stat;
    pic18_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_SSPIF;
    if (sim_irq_cb) sim_irq_cb();
}


/**
 * @brief Step the simulated EUSART state machine by one instruction cycle.
 *
 * Re-asserts PIR1<TXIF> every cycle while TXEN is set to model the
 * instantaneous transmit completion.
 */
static void sim_step_usart(void)
{
    /* Re-assert TXIF every cycle when TXEN is set. TXIF is cleared by the
     * user writing TXREG (see EPIC_USART_Transmit); this step brings it
     * back high to model the instantaneous transmit completion (mirrors
     * the PIC16 sim). RCIF is set by the host application through
     * pic18_sim_drive_usart_rx(). */
    uint8_t txsta = pic18_sim_sfr[PIC_REG_TXSTA];
    if (txsta & PIC_TXSTA_TXEN) {
        pic18_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_TXIF;
    }
}

/**
 * @brief Deliver a received USART byte to the simulated hardware.
 *
 * Places the byte in RCREG, sets PIR1<RCIF>, and raises the IRQ hook.
 *
 * @param data the byte received on the USART
 */
void pic18_sim_drive_usart_rx(uint8_t data)
{
    /* Place the byte in RCREG (DS39632E §20.2.2), set PIR1<RCIF>. */
    pic18_sim_sfr[PIC_REG_RCREG] = data;
    pic18_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_RCIF;
    if (sim_irq_cb) sim_irq_cb();
}


/**
 * @brief Drive the comparator output levels and raise the comparator IRQ.
 *
 * Sets CMCON<C1OUT>/<C2OUT> (read-only in real hardware) and PIR2<CMIF>
 * to model an output change.
 *
 * @param c1out nonzero to set C1OUT high, zero for low
 * @param c2out nonzero to set C2OUT high, zero for low
 */
void pic18_sim_drive_comp(uint8_t c1out, uint8_t c2out)
{
    /* Set CMCON<C1OUT>/<C2OUT> (the comparator output levels, read-only in
     * real hardware) and raise CMIF (PIR2<6>) to model an output change. */
    uint8_t v = pic18_sim_sfr[PIC_REG_CMCON] & (uint8_t)~(PIC_CMCON_C1OUT | PIC_CMCON_C2OUT);
    if (c1out) v |= PIC_CMCON_C1OUT;
    if (c2out) v |= PIC_CMCON_C2OUT;
    pic18_sim_sfr[PIC_REG_CMCON] = v;
    pic18_sim_sfr[PIC_REG_PIR2] |= PIC_PIR2_CMIF;
    if (sim_irq_cb) sim_irq_cb();
}


/**
 * @brief Write a byte to the simulated data EEPROM.
 *
 * @param addr the EEPROM address (0..255)
 * @param data the byte to store
 */
void pic18_sim_drive_eeprom_byte(uint8_t addr, uint8_t data)
{
    sim_eeprom[addr] = data;
}

/**
 * @brief Complete a simulated data EEPROM write cycle.
 *
 * Stores the byte and sets PIR2<EEIF> to model the write cycle finishing.
 *
 * @param addr the EEPROM address (0..255)
 * @param data the byte to store
 */
void pic18_sim_drive_eeprom_done(uint8_t addr, uint8_t data)
{
    sim_eeprom[addr] = data;
    /* Set PIR2<EEIF> (bit 4) to model the write cycle completing. */
    pic18_sim_sfr[PIC_REG_PIR2] |= PIC_PIR2_EEIF;
    if (sim_irq_cb) sim_irq_cb();
}

/**
 * @brief Read a byte from the simulated data EEPROM.
 *
 * @param addr the EEPROM address (0..255)
 * @return the byte stored at that address
 */
uint8_t pic18_sim_eeprom_read(uint8_t addr)
{
    return sim_eeprom[addr];
}


/**
 * @brief Complete a simulated A/D conversion.
 *
 * Clears ADCON0<GO/DONE>, stores the 10-bit result in ADRESH:ADRESL per
 * ADFM, and sets PIR1<ADIF>.
 *
 * @param result the 10-bit conversion result (0..1023)
 */
void pic18_sim_drive_adc_done(uint16_t result)
{
    /* Clear GO/DONE in ADCON0. */
    uint8_t adcon0 = (uint8_t)(pic18_sim_sfr[PIC_REG_ADCON0] & (uint8_t)~PIC_ADCON0_GO_DONE);
    pic18_sim_sfr[PIC_REG_ADCON0] = adcon0;

    /* Store the 10-bit result in ADRESH:ADRESL per ADFM (ADCON2<7>).
     *   Right (ADFM=1): ADRESH[1:0] = result[9:8], ADRESL = result[7:0].
     *   Left  (ADFM=0): ADRESH[7:2] = result[9:2], ADRESL[7:6] = result[1:0]. */
    uint8_t adfm = (uint8_t)(pic18_sim_sfr[PIC_REG_ADCON2] & PIC_ADCON2_ADFM);
    uint16_t r = (uint16_t)(result & 0x03FFU);
    if (adfm) {
        pic18_sim_sfr[PIC_REG_ADRESH] = (uint8_t)((r >> 8) & 0x03U);
        pic18_sim_sfr[PIC_REG_ADRESL] = (uint8_t)(r & 0xFFU);
    } else {
        pic18_sim_sfr[PIC_REG_ADRESH] = (uint8_t)(r >> 2);
        pic18_sim_sfr[PIC_REG_ADRESL] = (uint8_t)((r & 0x03U) << 6);
    }

    /* Set PIR1<ADIF> (bit 6). */
    pic18_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_ADIF;
    if (sim_irq_cb) sim_irq_cb();
}

#if PIC18FXX5X_FAMILY_HAS_SPP

/**
 * @brief Model an SPP transfer event.
 *
 * Sets SPPEPS<WRSPP>/<RDSPP> to the given levels and raises PIR1<SPPIF>.
 * SPPBUSY is left for a dedicated hook.
 *
 * @param wrspp nonzero to set the WRSPP status bit, zero for clear
 * @param rdspp nonzero to set the RDSPP status bit, zero for clear
 */
void pic18_sim_drive_spp(uint8_t wrspp, uint8_t rdspp)
{
    /* Set the SPPEPS<WRSPP>/<RDSPP> status bits to model a transfer event,
     * and raise SPPIF (PIR1<7>). SPPBUSY is left for a dedicated hook. */
    uint8_t eps = pic18_sim_sfr[PIC_REG_SPPEPS] & (uint8_t)~(PIC_SPPEPS_WRSPP | PIC_SPPEPS_RDSPP);
    if (wrspp) eps |= PIC_SPPEPS_WRSPP;
    if (rdspp) eps |= PIC_SPPEPS_RDSPP;
    pic18_sim_sfr[PIC_REG_SPPEPS] = eps;
    pic18_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_SPPIF;
    if (sim_irq_cb) sim_irq_cb();
}
#endif
