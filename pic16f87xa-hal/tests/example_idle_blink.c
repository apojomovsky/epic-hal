/**
 * @file    example_idle_blink.c
 * @brief   Blink an LED on RB0 with the CPU asleep, fully peripheral
 *          driven, so the CPU is mostly idle.
 *
 * @details
 *   Hardware timebase, not a software delay loop: the CPU spends
 *   almost all its time in Sleep, a Timer1 overflow interrupt wakes it
 *   just long enough to toggle the LED. Timer1 runs on the 32.768 kHz
 *   watch crystal (T1OSC), asynchronous (the only mode that keeps
 *   counting in Sleep, DS39582B §6.5), reload 0x8000 for a 1 s period.
 *   Wiring: LED+resistor on RB0, 32.768 kHz crystal on T1OSO/T1OSI,
 *   20 MHz HS crystal still required for the CPU clock.
 */

#include "pic16f87xa.h"
#include "pic16f87xa_sfr.h"
#include "peripherals/pic16f87xa_gpio.h"
#include "peripherals/pic16f87xa_timer1.h"
#include "core/pic16_irq.h"
#include "core/pic16f87xa_wdt_sleep.h"
#include "core/epic_harness.h"

/**
 * @brief  Timer1 reload, 0x8000 (32768). On the T1OSC timebase that is
 *         exactly 1 s of the 32.768 kHz crystal; the ISR writes it back
 *         after each overflow to re-arm the 1 s period.
 */
#define T1_RELOAD  0x8000U

/** Simulated run length (host only). The sim advances T1OSC Timer1 one
 *  count per cycle, so 32768 cycles per overflow → ~6 toggles in 200k. */
#define SIM_CYCLES  200000UL

/* Toggle count, the ISR is the only writer. */
static volatile uint32_t g_toggle_count = 0;

/**
 * @brief  Timer1 overflow callback, re-arms the period, toggles the LED,
 *         and refreshes the WDT (a no-op on the host). Runs in interrupt
 *         context on the target and in the sim IRQ callback on the host.
 */
static void on_t1_overflow(void)
{
    EPIC_TIMER1_WriteCounter(T1_RELOAD);
    EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
    EPIC_WDT_Refresh();
    g_toggle_count++;
}

int main(void)
{
    epic_harness_init(SIM_CYCLES);

    /* 1. RB0 as output, start low. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    /* 2. Timer1 on the 32.768 kHz T1OSC crystal, asynchronous (keeps
     *    counting in Sleep), 1:1 prescaler, reload 0x8000 → 1 s overflow.
     *    The same configuration builds for the sim, which models the
     *    external clock at a simplified rate. */
    TIMER1_HandleTypeDef h = TIMER1_HANDLE_DEFAULT;
    h.ClockSource      = TIMER1_CLOCK_EXTERNAL;
    h.ClockSync        = TIMER1_ASYNC_EXTERNAL;
    h.Oscillator       = TIMER1_OSCILLATOR_ON;
    h.Prescaler        = TIMER1_PRESCALER_1_1;
    h.ReloadValue      = T1_RELOAD;
    h.OverflowCallback = on_t1_overflow;
    EPIC_TIMER1_Init(&h);
    EPIC_TIMER1_Start(&h);

    /* 3. Wait for the T1OSC crystal to start ticking before sleeping (a
     *    32 kHz crystal can take a few hundred ms), refreshing the WDT
     *    meanwhile. epic_harness_tick pumps the sim on the host (so
     *    the counter advances and the loop exits) and is a no-op on the
     *    target, where the real crystal advances the counter on its own. */
    while (EPIC_TIMER1_ReadCounter() <= T1_RELOAD) {
        EPIC_WDT_Refresh();
        epic_harness_tick();
    }

    /* 4. Enable global interrupts (EPIC_TIMER1_Init already set TMR1IE
     *    and PEIE). On the sim this is harmless; on the target it arms the
     *    wake-up. */
    EPIC_IRQ_Restore(1);

    /* 5. Idle loop, the heart of "CPU mostly idle". On the host the
     *    harness bounds the loop to SIM_CYCLES; on the target it runs
     *    forever. Each iteration pumps the sim (host) / is empty (target),
     *    then sleeps (a no-op on the host). Each Timer1 overflow wakes the
     *    CPU; the vector dispatcher calls TIMER1_IRQHandler, which clears
     *    the flag and runs on_t1_overflow (reload + toggle + WDT refresh),
     *    and the CPU sleeps again. */
    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
        EPIC_Sleep_Enter();
    }

    epic_harness_log("RB0 toggled %u times; CPU idle between overflows.\n",
                           (unsigned)g_toggle_count);
    return epic_harness_report(g_toggle_count >= 2U);
}
