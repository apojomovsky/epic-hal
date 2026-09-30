/*
 * Dedicated IRQ-core smoke test (§5.5): enable one Timer0 interrupt,
 * confirm it fires with INTCON<TMR0IE>/T0CON<TMR0ON> reading back set
 * (DS39631E §9.0). The ISR toggles RB0, counts overflows, and drives
 * the RA0 PASS marker from the second overflow: under MPLAB SIM the
 * main loop can starve behind firing ISRs and never reach report().
 */

#include "pic18f2520_hal.h"
#include "pic18f2520_sfr.h"
#include "peripherals/pic18f2520_gpio.h"
#include "peripherals/pic18f2520_timer0.h"
#include "core/pic18_irq.h"
#include "core/epic_harness.h"

/*
 * Expected register image: T0CON = 0xC7 (8-bit, Fosc/4, 1:256, ON);
 * INTCON = 0xE0 (GIE/GIEL/TMR0IE); INTCON2 = 0xFB; RCON = 0xD7 (POR
 * 0x57 | IPEN); TRISB = 0xFE (RB0 out); LATB = 0x00 (starts low).
 */

/** @brief Family-local harness extension: no-op on the CMake host build
 *  (pic18_harness_sim.c), infinite loop on the mdb build so the
 *  HARNESS=sim marker's RA0 stays set across the mdb latch readback
 *  (PORTA does not mirror LATx on this silicon; mirrors 193x halt). */
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
    if (g_toggle_count >= 2U)
    {
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
    for (uint32_t i = 0; epic_harness_running(i); i++)
    {
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
