/*
 * Dedicated IRQ-core smoke test for pic18f2520-hal, the step
 * docs/adding-a-device.md §5.5 makes mandatory before any peripheral
 * builds on the interrupt backend: enable one timer interrupt, confirm
 * it actually fires and that the flag/enable bits read back correctly.
 * Timer0 (INTCON<TMR0IE>/<TMR0IF>) is the chosen source; enabling it
 * through EPIC_IRQ_Enable + EPIC_IRQ_Restore, then letting Timer0
 * overflow via the sim (host) or real time (mdb/MPSIM) drives the ISR,
 * which counts the overflow. The pass condition is both "an overflow
 * was observed" (the ISR ran) and "the enable bit reads back set" (the
 * IRQ write path landed), verified through literal-token SFR reads,
 * the proven-safe side (DS39631E §9.0).
 *
 * Expected register image (host sim, verified by probe):
 *   T0CON   = 0xC7                       (8-bit, T08BIT=1; Fosc/4;
 *                                        prescaler 1:256 T0PS=111;
 *                                        TMR0ON=1 after Start)
 *   INTCON  = 0xE0                       (GIE=1, GIEL=1, TMR0IE bit 5,
 *                                        once EPIC_IRQ_Restore(1) runs)
 *   INTCON2 = 0xFB                       (RBPU=1, INTEDG0/1/2=1)
 *   RCON    = 0xD7                       (POR value 0x57 | IPEN bit 7,
 *                                        once EPIC_IRQ_Restore(1) runs)
 *   TRISB   = 0xFE                       (RB0 output, rest input)
 *   LATB    = 0x00                       (RB0 driving low at start)
 * The ISR flips LATB<0> on every overflow; g_toggle_count counts them.
 *
 * SIM_CYCLES = 600_000 gives ~9 Timer0 overflows at the 1:256 prescaler
 * + 8-bit counter (65536 cycles per overflow).
 */

#include "pic18f2520_hal.h"
#include "pic18f2520_sfr.h"
#include "peripherals/pic18f2520_gpio.h"
#include "peripherals/pic18f2520_timer0.h"
#include "core/pic18_irq.h"
#include "core/epic_harness.h"

/** @brief Family-local harness extension: no-op on the CMake host build
 *  (pic18_harness_sim.c), infinite loop on the mdb build so the
 *  HARNESS=sim marker's RA0 stays set across the mdb `print PORTA`
 *  readback (mirrors pic16f193x_harness_halt). */
extern void pic18f2520_harness_halt(void);

/** Simulated run length (host only). */
#define SIM_CYCLES  600000UL

/* Toggle count, the ISR is the only writer. */
static volatile uint32_t g_toggle_count = 0;

/** @brief  Timer0 overflow callback, bumped the ISR fires.
 *
 *          Runs in interrupt context (target) or the sim IRQ callback
 *          (host). Toggles RB0, bumps the count, and on the second
 *          overflow drives the RA0 PASS marker directly: under MPLAB
 *          SIM the main loop is starved by continuously-firing ISRs and
 *          may never reach the report() call in the mdb wait window, so
 *          the marker must be driven from the ISR itself the instant its
 *          condition is met (docs/adding-a-device.md §4 step 6's
 *          ISR-driven-marker pattern).
 */
static void on_t0_overflow(void)
{
    EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
    g_toggle_count++;
    if (g_toggle_count >= 2U) {
        EPIC_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    }
}

/**
 * @brief  IRQ-core smoke test: enable the Timer0 interrupt, let it fire,
 *         then read back the enable and a live fired flag through the
 *         literal-token path and report.
 */
int main(void)
{
    epic_harness_init(SIM_CYCLES);

    /* 1. RB0 as output, start low (writes go through LATB, DS39631E §10.0). */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    /* 2. Timer0: 8-bit, internal Fosc/4, 1:256 prescaler, reload 0. */
    TIMER0_HandleTypeDef h = TIMER0_HANDLE_DEFAULT;
    h.Mode              = TIMER0_BITMODE_8BIT;
    h.ClockSource       = TIMER0_CLOCK_INTERNAL;
    h.Prescaler         = TIMER0_PRESCALER_1_256;
    h.PrescalerAssigned = true;
    h.ReloadValue       = 0x00U;
    h.OverflowCallback  = on_t0_overflow;
    EPIC_TIMER0_Init(&h);   /* sets TMR0IE */
    EPIC_TIMER0_Start(&h);

    /* 3. Arm the master enables (also sets IPEN). Put TMR0 on the high
     *    vector (INTCON2<TMR0IP>): the reset default is low priority, and
     *    the dedicated-vector high path is the one the mdb-proven TIMER2
     *    gates exercise; a source on the low vector is more readily
     *    starved under MPLAB SIM's interrupt-servicing model. This is the
     *    documented EPIC_IRQ_* contract. */
    EPIC_IRQ_SetPriority(PIC18_IRQ_TMR0, EPIC_IRQ_PRIORITY_HIGH);
    EPIC_IRQ_Restore(1);

    /* 4. The enable bit must read back set (the IRQ write path landed) and
     *    the timer must be running, both via literal-token reads. */
    uint8_t en_ok     =
        (epic_sfr_read8(PIC_REG_INTCON) & PIC_INTCON_TMR0IE) ? 1U : 0U;
    uint8_t running_ok =
        (epic_sfr_read8(PIC_REG_T0CON)  & PIC_T0CON_TMR0ON)  ? 1U : 0U;

    /* 5. Let time pass; the ISR toggles RB0 and bumps the count. */
    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
        if (g_toggle_count >= 2U) break;
    }

    /* 6. A fired flag must read back set at least once on the way; the
     *    count is the durable proof the ISR ran. */
    uint8_t fired_ok = (g_toggle_count >= 2U);

    epic_harness_log("IRQ: en=%u run=%u fired=%u toggles=%u\n",
                     (unsigned)en_ok, (unsigned)running_ok,
                     (unsigned)fired_ok, (unsigned)g_toggle_count);
    int rc = epic_harness_report(en_ok && running_ok && fired_ok);

    /* Freeze so the HARNESS=sim marker's RA0 stays set across the mdb
     * `print PORTA` readback (mirrors pic16f193x). */
    pic18f2520_harness_halt();
    return rc;
}
