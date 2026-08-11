/**
 * Host simulation backend for the PIC16F193X HAL: provides
 * `pic16f193x_sim_sfr[]`, the 4096-byte memory-backed register file (one
 * byte per 12-bit data-memory address, DS41364B §2.2) that the host SFR
 * macros (include/host/pic16f193x_platform.h) dereference. Models Timer0
 * (DS41364B §15.0), the GPIO pin-level model (PORT/TRIS/LAT/ANSEL, §6.0)
 * and the PORTB interrupt-on-change (§7.0).
 */

#include "pic16f193x_sim.h"
#include "pic16f193x_sfr.h"
#include <string.h>

/* register file */

/** SFR backing store. Indices are the physical 12-bit data-memory
 *  addresses (DS41364B §2.2, Table 2-4/2-5). Covers all 32 banks x 128. */
uint8_t pic16f193x_sim_sfr[0x1000];

/* Per-pin input overrides set by the host application (per port A..E). */
static uint8_t sim_input_override[5] = {0};
static uint8_t sim_input_value   [5] = {0};

/* Last-seen PORTB input level, for interrupt-on-change edge detection. */
static uint8_t sim_last_portb = 0xFFU;

/* Optional ISR hook. */
static pic16f193x_sim_irq_cb_t sim_irq_cb = 0;

static void sim_step_timer0(void);
static void sim_step_timer1(void);
static void sim_step_timer246(void);
static void sim_step_usart(void);
static void sim_step_eeprom(void);
static void sim_refresh_ports(void);
static void sim_step_ioc(void);

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

/* public API */

void pic16f193x_sim_reset(void)
{
    memset(pic16f193x_sim_sfr, 0, sizeof pic16f193x_sim_sfr);

    /* Power-on reset values, DS41364B §3 + Table 2-4 register summary. */
    pic16f193x_sim_sfr[PIC_REG_STATUS]  = PIC_STATUS_POR_VALUE;
    pic16f193x_sim_sfr[PIC_REG_PCON]    = PIC_PCON_POR_VALUE;
    pic16f193x_sim_sfr[PIC_REG_INTCON]  = PIC_INTCON_POR_VALUE;
    pic16f193x_sim_sfr[PIC_REG_OPTION] = PIC_OPTION_POR_VALUE;   /* 0xFF. */
    pic16f193x_sim_sfr[PIC_REG_PIR1]    = PIC_PIR1_POR_VALUE;
    pic16f193x_sim_sfr[PIC_REG_PIR2]    = PIC_PIR2_POR_VALUE;
    pic16f193x_sim_sfr[PIC_REG_PIR3]    = PIC_PIR3_POR_VALUE;
    pic16f193x_sim_sfr[PIC_REG_PIE1]    = PIC_PIE1_POR_VALUE;
    pic16f193x_sim_sfr[PIC_REG_PIE2]    = PIC_PIE2_POR_VALUE;
    pic16f193x_sim_sfr[PIC_REG_PIE3]    = PIC_PIE3_POR_VALUE;

    /* PR2/PR4/PR6 reset to 0xFF, not 0x00 (DS41364B §17.0, Register
     * 17-1 POR column); the surrounding memset zeroes everything, so
     * these three need an explicit override like PIR1/PIE1 above. */
    pic16f193x_sim_sfr[PIC_REG_PR2] = 0xFFU;
    pic16f193x_sim_sfr[PIC_REG_PR4] = 0xFFU;
    pic16f193x_sim_sfr[PIC_REG_PR6] = 0xFFU;

    /* TXSTA POR = 0x02 (TRMT=1), BAUDCON POR = 0x40 (RCIDL=1, read-only),
     * DS41364B Register 23-3/23-4 POR columns. */
    pic16f193x_sim_sfr[PIC_REG_TXSTA] = PIC_TXSTA_POR_VALUE;
    pic16f193x_sim_sfr[PIC_REG_BAUDCON] = PIC_BAUDCON_POR_VALUE;

    /* PIR1<TXIF> resets to 1 (TXREG empty after POR, DS41364B §20.0). */
    pic16f193x_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_TXIF;

    /* TRIS defaults: 1 = input for every implemented pin (DS41364B §6.0). */
    pic16f193x_sim_sfr[PIC_REG_TRISA] = 0xFFU;
    pic16f193x_sim_sfr[PIC_REG_TRISB] = 0xFFU;
    pic16f193x_sim_sfr[PIC_REG_TRISC] = 0xFFU;
#if PIC16F193X_FAMILY_HAS_PORTD
    pic16f193x_sim_sfr[PIC_REG_TRISD] = 0xFFU;
#endif
#if PIC16F193X_FAMILY_HAS_PORTE
    pic16f193x_sim_sfr[PIC_REG_TRISE] = 0x0FU;   /* PORTE is 4-bit (RE0-RE3). */
#endif

    /* ANSEL defaults: analog pins read ANSEL=1 at POR (DS41364B §6.0); the
     * GPIO driver clears ANSEL when configuring a pin as digital. */
    pic16f193x_sim_sfr[PIC_REG_ANSELA] = 0xFFU;
    pic16f193x_sim_sfr[PIC_REG_ANSELB] = 0xFFU;
#if PIC16F193X_FAMILY_HAS_PORTD
    pic16f193x_sim_sfr[PIC_REG_ANSELD] = 0xFFU;
#endif
#if PIC16F193X_FAMILY_HAS_PORTE
    pic16f193x_sim_sfr[PIC_REG_ANSELE] = 0x0FU;
#endif

    /* Latches and ports read 0 at POR. */
    pic16f193x_sim_sfr[PIC_REG_LATA]  = 0x00U;
    pic16f193x_sim_sfr[PIC_REG_LATB]  = 0x00U;
    pic16f193x_sim_sfr[PIC_REG_LATC]  = 0x00U;
#if PIC16F193X_FAMILY_HAS_PORTD
    pic16f193x_sim_sfr[PIC_REG_LATD] = 0x00U;
#endif
#if PIC16F193X_FAMILY_HAS_PORTE
    pic16f193x_sim_sfr[PIC_REG_LATE]  = 0x00U;
#endif
    pic16f193x_sim_sfr[PIC_REG_PORTA] = 0x00U;
    pic16f193x_sim_sfr[PIC_REG_PORTB] = 0x00U;
    pic16f193x_sim_sfr[PIC_REG_PORTC] = 0x00U;
#if PIC16F193X_FAMILY_HAS_PORTD
    pic16f193x_sim_sfr[PIC_REG_PORTD] = 0x00U;
#endif
#if PIC16F193X_FAMILY_HAS_PORTE
    pic16f193x_sim_sfr[PIC_REG_PORTE] = 0x00U;
#endif

    memset(sim_input_override, 0, sizeof sim_input_override);
    memset(sim_input_value,    0, sizeof sim_input_value);
    sim_last_portb = 0xFFU;
}

void pic16f193x_sim_step(uint32_t ticks)
{
    for (uint32_t i = 0; i < ticks; i++) {
        sim_step_timer0();
        sim_step_timer1();
        sim_step_timer246();
        sim_step_usart();
        sim_step_eeprom();
        sim_refresh_ports();
        sim_step_ioc();
    }
}

/* Timer0 step */

static void sim_step_timer0(void)
{
    /* OPTION_REG layout (DS41364B Register 2-2): T0CS(5), T0SE(4),
     * PSA(3), PS<2:0>(2:0). Same prescaler ratios as classic PIC16
     * (Table 15-1). The sim only models the internal-clock (Fosc/4) path;
     * T0CS=1 (T0CKI external) halts the model since there is no external
     * signal to count. */
    uint8_t option = pic16f193x_sim_sfr[PIC_REG_OPTION];
    uint8_t t0cs   = (option & PIC_OPTION_T0CS) ? 1U : 0U;
    if (t0cs) return;   /* External clock: not modeled. */

    uint8_t ps  = option & PIC_OPTION_PS_MASK;
    uint8_t psa = (option & PIC_OPTION_PSA) ? 1U : 0U;

    /* PSA=1 assigns the prescaler to the WDT, so TMR0 runs at 1:1. */
    static const uint16_t ps_ratio[8] = { 2, 4, 8, 16, 32, 64, 128, 256 };
    static uint16_t t0_prescaler = 0U;
    uint32_t rate = psa ? 1U : (uint32_t)ps_ratio[ps & 0x07U];
    (void)psa;

    t0_prescaler++;
    if (t0_prescaler < rate) return;
    t0_prescaler = 0U;

    uint8_t t0 = pic16f193x_sim_sfr[PIC_REG_TMR0];
    t0++;
    if (t0 == 0x00U) {
        pic16f193x_sim_sfr[PIC_REG_INTCON] |= PIC_INTCON_TMR0IF;
        if (sim_irq_cb) sim_irq_cb();
    }
    pic16f193x_sim_sfr[PIC_REG_TMR0] = t0;
}

/* Timer1 step */

static void sim_step_timer1(void)
{
    /* T1CON layout (DS41364B Register 16-1):
     *   bit 0    TMR1ON
     *   bit 1    unimplemented
     *   bit 2    T1SYNC
     *   bit 3    T1OSCEN
     *   bit 5:4  T1CKPS1:T1CKPS0
     *   bit 7:6  TMR1CS1:TMR1CS0
     * T1SYNC/T1OSCEN are 0 in this phase; EPIC_TIMER1_Start() rejects
     * any ClockSource other than TIMER1_CLOCK_INTERNAL, so TMR1CS
     * bits 7:6 are always 00 (FOSC/4) coming from the driver. */
    uint8_t t1con = pic16f193x_sim_sfr[PIC_REG_T1CON];
    if (!(t1con & PIC_T1CON_TMR1ON)) return;   /* TMR1ON = 0 -> stopped. */

    /* TMR1CS<1:0> != 00 (external clock / T1OSC / CAPOSC): the sim
     * does not model those sources, so the counter halts. Same
     * limitation as pic16f87xa_sim.c. Unreachable via the current
     * HAL (see above), kept as a direct-SFR-poke safety net. */
    if (t1con & 0xC0U) return;

    /* Prescaler ratio from T1CKPS<1:0>. */
    uint8_t ckps = (uint8_t)((t1con >> 4) & 0x3U);
    static const uint16_t ps_ratio[4] = { 1, 2, 4, 8 };
    static uint16_t t1_prescaler = 0U;
    t1_prescaler++;
    if (t1_prescaler < ps_ratio[ckps]) return;
    t1_prescaler = 0U;

    /* Increment TMR1H:TMR1L (16-bit counter). */
    uint8_t lo = pic16f193x_sim_sfr[PIC_REG_TMR1L];
    uint8_t hi = pic16f193x_sim_sfr[PIC_REG_TMR1H];
    uint16_t full = (uint16_t)(((uint16_t)hi << 8) | lo);
    full++;
    pic16f193x_sim_sfr[PIC_REG_TMR1L] = (uint8_t)(full & 0xFFU);
    pic16f193x_sim_sfr[PIC_REG_TMR1H] = (uint8_t)(full >> 8);

    /* On overflow, set PIR1<TMR1IF> and fire the IRQ callback. */
    if (full == 0x0000U) {
        pic16f193x_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_TMR1IF;
        if (sim_irq_cb) sim_irq_cb();
    }
}

/* Timer2/4/6 step */

static void sim_step_timer246(void)
{
    /* T*CON layout (DS41364B §17.0), identical for T2CON/T4CON/T6CON:
     *   bits 6:3  T*OUTPS<3:0>  postscaler select, 1:(N+1)
     *   bit 2     TMR*ON        timer enable
     *   bits 1:0  T*CKPS<1:0>   prescaler select: 00=1:1,01=1:4,1x=1:16
     *
     * Unlike sim_step_timer0/sim_step_timer1 (increment then check for
     * wraparound), this counts up to PRx and resets to 0 on the cycle
     * it would exceed PRx: comparing tmr == pr BEFORE incrementing,
     * not after, because an 8-bit counter wrapping 0xFF -> 0x00 on its
     * own can't be told apart from a PR=0xFF match using a
     * post-increment check. */
    static const uint16_t con_addr[3] = { PIC_REG_T2CON, PIC_REG_T4CON, PIC_REG_T6CON };
    static const uint16_t tmr_addr[3] = { PIC_REG_TMR2,  PIC_REG_TMR4,  PIC_REG_TMR6  };
    static const uint16_t pr_addr[3]  = { PIC_REG_PR2,   PIC_REG_PR4,   PIC_REG_PR6   };
    static const uint16_t pir_addr[3] = { PIC_REG_PIR1,  PIC_REG_PIR3,  PIC_REG_PIR3  };
    static const uint8_t  pir_bit[3]  = { PIC_PIR1_TMR2IF, PIC_PIR3_TMR4IF, PIC_PIR3_TMR6IF };
    static const uint8_t  pre_ratio[4] = { 1, 4, 16, 16 };

    static uint16_t prescale_ctr[3]  = { 0, 0, 0 };
    static uint8_t  postscale_ctr[3] = { 0, 0, 0 };

    for (uint8_t i = 0; i < 3u; i++) {
        uint8_t con = pic16f193x_sim_sfr[con_addr[i]];
        if (!(con & 0x04U)) continue;   /* TMR*ON = 0 -> stopped. */

        uint8_t ckps = con & 0x03U;
        prescale_ctr[i]++;
        if (prescale_ctr[i] < pre_ratio[ckps]) continue;
        prescale_ctr[i] = 0U;

        uint8_t tmr = pic16f193x_sim_sfr[tmr_addr[i]];
        uint8_t pr  = pic16f193x_sim_sfr[pr_addr[i]];

        if (tmr == pr) {
            /* PR match: reset to 0, bump the postscaler. */
            tmr = 0U;
            uint8_t outps = (uint8_t)((con >> 3) & 0x0FU);
            postscale_ctr[i]++;
            if (postscale_ctr[i] > outps) {
                postscale_ctr[i] = 0U;
                pic16f193x_sim_sfr[pir_addr[i]] |= pir_bit[i];
                if (sim_irq_cb) sim_irq_cb();
            }
        } else {
            tmr++;
        }
        pic16f193x_sim_sfr[tmr_addr[i]] = tmr;
    }
}

/* USART step */

static void sim_step_usart(void)
{
    /* TXSTA: TRMT is a hardware-read-only bit (DS41364B §23.0) that
     * reads 1 when the shift register is empty. The sim models
     * transmission as instantaneous (no real UART wire), so TRMT stays
     * 1 whenever TXEN is set. The driver writes TXSTA without TRMT
     * (it is read-only on real silicon), so the sim sets it here. */
    uint8_t txsta = pic16f193x_sim_sfr[PIC_REG_TXSTA];
    if (txsta & PIC_TXSTA_TXEN) {
        pic16f193x_sim_sfr[PIC_REG_TXSTA] = (uint8_t)(txsta | PIC_TXSTA_TRMT);
    }
    /* BAUDCON: RCIDL (bit 6) is read-only, stays 1 when receiver idle. */
    pic16f193x_sim_sfr[PIC_REG_BAUDCON] |= PIC_BAUDCON_RCIDL;
}

/* EEPROM step */

static uint8_t s_eeprom_data[256];

static void sim_step_eeprom(void)
{
    uint8_t econ1 = pic16f193x_sim_sfr[PIC_REG_EECON1];
    if (econ1 & PIC_EECON1_RD) {
        uint8_t addr = pic16f193x_sim_sfr[PIC_REG_EEADRL];
        pic16f193x_sim_sfr[PIC_REG_EEDATL] = s_eeprom_data[addr];
        pic16f193x_sim_sfr[PIC_REG_EECON1] &= (uint8_t)~PIC_EECON1_RD;
    } else if (econ1 & PIC_EECON1_WR) {
        uint8_t addr = pic16f193x_sim_sfr[PIC_REG_EEADRL];
        s_eeprom_data[addr] = pic16f193x_sim_sfr[PIC_REG_EEDATL];
        pic16f193x_sim_sfr[PIC_REG_EECON1] &= (uint8_t)~PIC_EECON1_WR;
    }
}

/* GPIO pin-level refresh */

/* Keep PORTx fresh for EPIC_GPIO_ReadPin (which reads PORTx): for output
 * pins mirror LATx, for input pins mirror the driven override. The
 * canonical blink example ticks between writing and reading, so this
 * per-step refresh is sufficient on the host. */
static void sim_refresh_ports(void)
{
    static const uint16_t port_addr[5] = { PIC_REG_PORTA, PIC_REG_PORTB,
                                           PIC_REG_PORTC, PIC_REG_PORTD,
                                           PIC_REG_PORTE };
    static const uint16_t tris_addr[5] = { PIC_REG_TRISA, PIC_REG_TRISB,
                                           PIC_REG_TRISC, PIC_REG_TRISD,
                                           PIC_REG_TRISE };
    static const uint16_t lat_addr[5]  = { PIC_REG_LATA,  PIC_REG_LATB,
                                           PIC_REG_LATC,  PIC_REG_LATD,
                                           PIC_REG_LATE };
    for (uint8_t p = 0; p < 5u; p++) {
        uint8_t tris = pic16f193x_sim_sfr[tris_addr[p]];
        uint8_t lat  = pic16f193x_sim_sfr[lat_addr[p]];
        uint8_t in   = sim_input_value[p];
        uint8_t port = 0U;
        for (uint8_t b = 0; b < 8u; b++) {
            uint8_t m = (uint8_t)(1U << b);
            if (tris & m) {
                if (sim_input_override[p] & m) port |= (uint8_t)(in & m);
                /* input with no override reads 0. */
            } else {
                port |= (uint8_t)(lat & m);
            }
        }
        pic16f193x_sim_sfr[port_addr[p]] = port;
    }
}

/* PORTB interrupt-on-change */

static void sim_step_ioc(void)
{
    /* DS41364B §7.0: IOCBP enables positive-edge detection per pin,
     * IOCBN negative-edge. IOCBF is the per-pin flag; IOCIF (INTCON<0>)
     * is set when any IOCBF bit is set. */
    uint8_t cur    = pic16f193x_sim_sfr[PIC_REG_PORTB];
    uint8_t iocbp  = pic16f193x_sim_sfr[PIC_REG_IOCBP];
    uint8_t iocbn  = pic16f193x_sim_sfr[PIC_REG_IOCBN];
    uint8_t changed = (uint8_t)(cur ^ sim_last_portb);
    uint8_t iocbf   = pic16f193x_sim_sfr[PIC_REG_IOCBF];

    for (uint8_t b = 0; b < 8u; b++) {
        uint8_t m = (uint8_t)(1U << b);
        if (!(changed & m)) continue;
        uint8_t rising  = (cur & m) && !(sim_last_portb & m);
        uint8_t falling = !(cur & m) && (sim_last_portb & m);
        if ((rising  && (iocbp & m)) || (falling && (iocbn & m))) {
            iocbf |= m;
        }
    }
    sim_last_portb = cur;
    if (iocbf != pic16f193x_sim_sfr[PIC_REG_IOCBF]) {
        pic16f193x_sim_sfr[PIC_REG_IOCBF] = iocbf;
        if (iocbf) {
            pic16f193x_sim_sfr[PIC_REG_INTCON] |= PIC_INTCON_IOCIF;
            if (sim_irq_cb) sim_irq_cb();
        }
    }
}

/* input / output */

void pic16f193x_sim_drive_input(char port, uint8_t pin, uint8_t level)
{
    if (pin > 7U) return;
    uint8_t idx = port_index(port);
    uint8_t mask = (uint8_t)(1U << pin);
    sim_input_override[idx] |= mask;
    if (level) sim_input_value[idx] |= mask;
    else       sim_input_value[idx] &= (uint8_t)~mask;
}

uint8_t pic16f193x_sim_read_output(char port, uint8_t pin)
{
    if (pin > 7U) return 0U;
    uint8_t idx  = port_index(port);
    uint8_t mask = (uint8_t)(1U << pin);
    static const uint16_t tris_addr[5] = { PIC_REG_TRISA, PIC_REG_TRISB,
                                           PIC_REG_TRISC, PIC_REG_TRISD,
                                           PIC_REG_TRISE };
    static const uint16_t lat_addr[5]  = { PIC_REG_LATA,  PIC_REG_LATB,
                                           PIC_REG_LATC,  PIC_REG_LATD,
                                           PIC_REG_LATE };
    uint8_t tris = pic16f193x_sim_sfr[tris_addr[idx]];
    if (tris & mask) {
        /* Input: return the externally driven level (0 if not driven). */
        return (sim_input_override[idx] & mask) ?
               ((sim_input_value[idx] & mask) ? 1U : 0U) : 0U;
    }
    /* Output: return the latch bit. */
    return (pic16f193x_sim_sfr[lat_addr[idx]] & mask) ? 1U : 0U;
}

void pic16f193x_sim_set_irq_callback(pic16f193x_sim_irq_cb_t cb)
{
    sim_irq_cb = cb;
}
