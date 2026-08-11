/* Blink an LED on RB0 with the CPU asleep, fully peripheral driven.
 * Timer1 on the 32.768 kHz T1OSC crystal, asynchronous (the only mode
 * that keeps counting in Sleep, DS39582B §6.5), reload 0x8000 for a
 * 1 s period; the overflow interrupt wakes the CPU just long enough to
 * toggle the LED. Wiring: LED+resistor on RB0, 32.768 kHz crystal on
 * T1OSO/T1OSI, 20 MHz HS crystal for the CPU clock. */

#include "pic16f87xa.h"
#include "pic16f87xa_sfr.h"
#include "peripherals/pic16f87xa_gpio.h"
#include "peripherals/pic16f87xa_timer1.h"
#include "core/pic16_irq.h"
#include "core/pic16f87xa_wdt_sleep.h"
#include "core/epic_harness.h"

/* Timer1 reload, 0x8000 (32768): exactly 1 s on the 32.768 kHz T1OSC
 * timebase; the ISR writes it back after each overflow to re-arm. */
#define T1_RELOAD  0x8000U

/** Simulated run length (host only). The sim advances T1OSC Timer1 one
 *  count per cycle, so 32768 cycles per overflow → ~6 toggles in 200k. */
#define SIM_CYCLES  200000UL

/* Toggle count, the ISR is the only writer. */
static volatile uint32_t g_toggle_count = 0;

/* Timer1 overflow callback: re-arms the period, toggles the LED, and
 * refreshes the WDT (a no-op on the host). Runs in interrupt context
 * on the target and in the sim IRQ callback on the host. */
/**
 * @brief Re-arm the 1 s period, toggle RB0 and refresh the WDT.
 */
static void on_t1_overflow(void)
{
    EPIC_TIMER1_WriteCounter(T1_RELOAD);
    EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
    EPIC_WDT_Refresh();
    g_toggle_count++;
}

/**
 * @brief Blink RB0 from a Timer1 overflow while the CPU sleeps between
 *        overflows.
 */
int main(void)
{
    epic_harness_init(SIM_CYCLES);

    /* 1. RB0 as output, start low. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    /* 2. Timer1 on the 32.768 kHz T1OSC crystal, asynchronous (keeps
     *    counting in Sleep), 1:1 prescaler, reload 0x8000 -> 1 s
     *    overflow. The sim models the external clock at a simplified
     *    rate. */
    TIMER1_HandleTypeDef h = TIMER1_HANDLE_DEFAULT;
    h.ClockSource      = TIMER1_CLOCK_EXTERNAL;
    h.ClockSync        = TIMER1_ASYNC_EXTERNAL;
    h.Oscillator       = TIMER1_OSCILLATOR_ON;
    h.Prescaler        = TIMER1_PRESCALER_1_1;
    h.ReloadValue      = T1_RELOAD;
    h.OverflowCallback = on_t1_overflow;
    EPIC_TIMER1_Init(&h);
    EPIC_TIMER1_Start(&h);

    /* 3. Wait for the T1OSC crystal to start ticking before sleeping
     *    (a 32 kHz crystal can take a few hundred ms), refreshing the
     *    WDT meanwhile. On the host the sim advances the counter so
     *    the loop exits. */
    while (EPIC_TIMER1_ReadCounter() <= T1_RELOAD) {
        EPIC_WDT_Refresh();
        epic_harness_tick();
    }

    /* 4. Enable global interrupts (TMR1IE and PEIE already set by
     *    Init). On the sim this is harmless; on the target it arms the
     *    wake-up. */
    EPIC_IRQ_Restore(1);

    /* 5. Idle loop: each iteration pumps the sim (host) or is empty
     *    (target), then sleeps. Each Timer1 overflow wakes the CPU;
     *    the dispatcher runs the callback and the CPU sleeps again.
     *    The harness bounds the loop on the host. */
    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
        EPIC_Sleep_Enter();
    }

    epic_harness_log("RB0 toggled %u times; CPU idle between overflows.\n",
                           (unsigned)g_toggle_count);
    return epic_harness_report(g_toggle_count >= 2U);
}
