/**
 * Timer1 overflow fires an ISR that toggles RB0, the canonical "Timer1
 * counts and the ISR runs" smoke for pic16f193x-hal. The main loop just
 * lets time pass (pumping the sim on host, busy-spinning on target, via
 * core/epic_harness.h) and refreshes the WDT.
 *
 * Expected register image (host sim, after init, before main loop):
 *   T1CON  = 0x30                       (T1CKPS<5:4>=11, TMR1ON=0,
 *                                        before Start sets TMR1ON=1)
 *   PIE1   = 0x01                        (TMR1IE bit 0)
 *   INTCON = 0xC0                        (GIE=1, PEIE=1, TMR0IE=0)
 *   TRISB  = 0xFE                        (RB0 output, rest input)
 *   ANSELB = 0xFE                        (RB0 digital, rest analog)
 *   LATB   = 0x00                        (RB0 driving low at start)
 *   TRISA  = 0xFE                        (RA0 output, rest input;
 *                                        driven by HARNESS=sim
 *                                        harness, not by this ex.)
 *   ANSELA = 0xFE                        (RA0 digital, rest analog)
 *   LATA   = 0x00                        (RA0 start low; flipped by
 *                                        the harness's log() hook
 *                                        on epic_harness_report)
 * The ISR flips LATB<0> on every overflow; g_toggle_count counts them.
 *
 * SIM_CYCLES = 2_000_000 gives ~3.8 Timer1 overflows at the 1:8
 * prescaler + 16-bit counter, so the toggle count is around 3-4. Pass
 * condition: >= 2.
 */

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "peripherals/pic16f193x_gpio.h"
#include "peripherals/pic16f193x_timer1.h"
#include "core/pic16f193x_irq.h"
#include "core/pic16f193x_wdt_sleep.h"
#include "core/epic_harness.h"

/** Family-local harness extension, not part of core/epic_harness.h
 *  since only pic16f193x's RA0-marker mechanism needs it: no-op on
 *  the CMake host build (pic16f193x_harness_sim.c), infinite loop on
 *  the mdb-under-MPLAB-SIM build (pic16f193x_harness_sim_target.c) so
 *  the HARNESS=sim marker's RA0 stays set across the mdb `print PORTA`
 *  readback. */
extern void pic16f193x_harness_halt(void);

#ifndef FOSC_HZ
#define FOSC_HZ 32000000UL
#endif

/** Simulated run length (host only). 524288 cycles per overflow at
 *  1:8 prescaler + 16-bit counter; 2_000_000 gives ~3.8 overflows. */
#define SIM_CYCLES  2000000UL

/* Toggle count, the ISR is the only writer. */
static volatile uint32_t g_toggle_count = 0;

/* Timer1 overflow callback, runs in interrupt context (target) or
 * the sim IRQ callback (host). */
static void on_t1_overflow(void)
{
    EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
    g_toggle_count++;
}

int main(void)
{
    epic_harness_init(SIM_CYCLES);

    /* 1. RB0 as digital output, start low. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    /* 2. Timer1: internal Fosc/4, 1:8 prescaler, reload 0, toggle
     *    on each overflow. */
    TIMER1_HandleTypeDef h = TIMER1_HANDLE_DEFAULT;
    h.ClockSource       = TIMER1_CLOCK_INTERNAL;
    h.Prescaler         = TIMER1_PRESCALER_1_8;
    h.ReloadValue       = 0x0000U;
    h.OverflowCallback  = on_t1_overflow;
    EPIC_TIMER1_Init(&h);
    EPIC_TIMER1_Start(&h);

    /* 3. Arm the Timer1 interrupt (EPIC_TIMER1_Init set TMR1IE; now
     *    GIE). */
    EPIC_IRQ_Restore(1);

    /* 4. Let time pass. On the target this busy-spins forever,
     *    refreshing the WDT while the Timer1 ISR toggles RB0; on
     *    the host the harness bounds the loop to SIM_CYCLES and
     *    pumps the sim each iteration. EPIC_WDT_Refresh is a no-op
     *    on the host, so it is called unconditionally. */
    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
        EPIC_WDT_Refresh();
    }

    epic_harness_log("RB0 toggled %u times.\n", (unsigned)g_toggle_count);
    int rc = epic_harness_report(g_toggle_count >= 2U);
    /* Freeze here so the HARNESS=sim marker's RA0 stays set across the
     * mdb `print PORTA` readback: without this, XC8's `ljmp start`
     * epilogue would re-enter main() and epic_harness_init() would
     * drive RA0 low again. On the CMake host build this is a no-op and
     * falls through to `return rc`. */
    pic16f193x_harness_halt();
    return rc;
}
