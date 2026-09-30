/*
 * HARNESS=sim dedicated IRQ-backend smoke test (epic-hal#178): enables
 * one Timer0 overflow interrupt and asserts it fires (RB0 toggles from
 * the ISR) with INTCON<TMR0IE/TMR0IF> and T0CON<TMR0ON> reading back
 * correctly (DS39605F §9.0). On the host build the bounded loop below
 * finishes and epic_harness_report() checks all of it. On real target
 * the gate is MODE=toggle (watches LATB across fixed `stepi` samples),
 * not MODE=gpio's `run`+`wait`+marker protocol: unreliable for a
 * bounded loop this size under MPLAB SIM (adding-a-device.md §4.6).
 */

#include "pic18f1320.h"
#include "pic18f1320_sfr.h"
#include "peripherals/pic18f1320_gpio.h"
#include "peripherals/pic18f1320_timer0.h"
#include "core/pic18_irq.h"
#include "core/epic_harness.h"

/** Bounded run length (host only; real target free-runs, watched via
 *  MODE=toggle instead). 1:32 prescaler overflows every 8192 cycles;
 *  deliberately not a smaller ratio whose period divides evenly into
 *  the mdb gate's fixed 200000-instruction `stepi` chunk (200000 mod
 *  8192 = 3392, so each chunk's starting phase drifts and the sampled
 *  toggle parity does too, rather than staying constant chunk to
 *  chunk). */
#define SIM_CYCLES  20000UL

static volatile uint32_t g_toggle_count = 0;

/** @brief Timer0 overflow callback: toggles RB0 and counts overflows. */
static void on_t0_overflow(void)
{
    EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
    g_toggle_count++;
}

/** @brief IRQ-backend dedicated smoke test: fire Timer0's overflow
 *         interrupt and confirm the flag/enable bits read back correctly.
 */
int main(void)
{
    epic_harness_init(SIM_CYCLES);

    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    /* 8-bit mode, internal Fosc/4, 1:32 prescaler (see SIM_CYCLES comment
     * for why not a smaller, power-of-two-friendly ratio). */
    TIMER0_HandleTypeDef h = TIMER0_HANDLE_DEFAULT;
    h.Mode              = TIMER0_BITMODE_8BIT;
    h.ClockSource       = TIMER0_CLOCK_INTERNAL;
    h.Prescaler         = TIMER0_PRESCALER_1_32;
    h.PrescalerAssigned = true;
    h.ReloadValue       = 0x00U;
    h.OverflowCallback  = on_t0_overflow;
    EPIC_TIMER0_Init(&h);
    EPIC_TIMER0_Start(&h);
    EPIC_IRQ_Restore(1);

    for (uint32_t i = 0; epic_harness_running(i); i++)
    {
        epic_harness_tick();
    }

    /* Flag/enable readback: TMR0IE must still be set (never disabled),
     * TMR0IF must read clear (the ISR's last EPIC_IRQ_ClearFlag), and
     * T0CON<TMR0ON> must still be set (the timer is still running). */
    uint8_t intcon = EPIC_REG8(PIC_REG_INTCON);
    uint8_t t0con  = EPIC_REG8(PIC_REG_T0CON);
    int regs_ok = (intcon & PIC_INTCON_TMR0IE) &&
                  !(intcon & PIC_INTCON_TMR0IF) &&
                  (t0con & PIC_T0CON_TMR0ON);

    epic_harness_log("Timer0 overflowed %u times, regs_ok=%d.\n",
                     (unsigned)g_toggle_count, regs_ok);
    return epic_harness_report(g_toggle_count >= 2U && regs_ok);
}
