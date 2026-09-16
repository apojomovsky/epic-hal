/* Sim-target implementation of the test harness (core/epic_harness.h):
 * real compiled firmware run under MPLAB SIM via mdb. This family has
 * no USART, so the PASS/FAIL marker goes out on RA0 (PORTA bit 0,
 * present on the 18-pin 16F54; MODE=gpio in scripts/sim-mdb-run.sh):
 * PASS drives RA0 high, FAIL leaves it low; epic_harness_log discards
 * every other line. running() is bounded by `cycles`. */

#include "core/epic_harness.h"
#include "peripherals/pic16f5x_gpio.h"
#include "pic16f5x_sfr.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 4000000UL
#endif

static uint32_t g_cycles = 0U;

/**
 * @brief Harness start-up (sim target): stores the cycle bound and
 *        arms RA0 as the marker output (driven low = not-yet-passed).
 * @param cycles the number of iterations the run is bounded by.
 */
void epic_harness_init(uint32_t cycles)
{
    g_cycles = cycles;
    EPIC_GPIO_Init(GPIOA, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
}

/**
 * @brief Advance simulated time. No-op: real time advances on its own
 *        under MPLAB SIM too, nothing to pump.
 */
void epic_harness_tick(void)
{
    /* Real time advances on its own under MPLAB SIM too, nothing to pump. */
}

/**
 * @brief Loop-continuation test: 1 while the bounded run is in
 *        progress, 0 after.
 * @param iteration the current 0-based iteration index.
 * @return 1 while `iteration` is below the configured cycle bound.
 */
int epic_harness_running(uint32_t iteration)
{
    return (iteration < g_cycles) ? 1 : 0;
}

/**
 * @brief Log line: the magic-string dispatch drives RA0 high on the
 *        PASS marker, low on the FAIL marker; every other line is a
 *        no-op.
 * @param fmt printf-style format string; variadic arguments are ignored
 *            (the markers have none).
 */
void epic_harness_log(const char *fmt, ...)
{
    /* Magic-string dispatch, trimmed for flash: the only callers are
     * epic_harness_report's two fixed literals, so the distinctive
     * bytes pin the verdict: "...RESULT: PASS\n\0" vs "...RESULT:
     * FAIL\n\0" differ at fmt[21] ('P'/'F') and fmt[24] ('S'/'L'). */
    if (fmt && fmt[0] == 'E' && fmt[19] == ':' && fmt[20] == ' ')
    {
        if (fmt[21] == 'P' && fmt[22] == 'A' && fmt[24] == 'S')
        {
            EPIC_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
        }
        else if (fmt[21] == 'F' && fmt[22] == 'A' && fmt[24] == 'L')
        {
            EPIC_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
        }
    }
    /* variadic args ignored; the marker has none. */
}

/**
 * @brief Freeze here so RA0 stays at its post-report value: XC8's `ljmp
 *        start` epilogue would otherwise re-enter main() on return, and
 *        epic_harness_init() would drive RA0 low again, flickering
 *        PORTA<0> across the mdb `print PORTA` readback window.
 */
void pic16f5x_harness_halt(void)
{
    for (;;)
    {
        /* nothing */
    }
}
