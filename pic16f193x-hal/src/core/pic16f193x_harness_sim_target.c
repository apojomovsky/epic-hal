/**
 * PIC16F193X sim-target implementation of the test harness
 * (core/epic_harness.h): epic_harness_log() inspects its format string
 * and, on the EPIC_HARNESS_RESULT marker, drives RA0 (PORTA bit 0) so
 * the mdb wrapper can read the result via 'print PORTA' in MODE=gpio.
 * The marker strings are passed verbatim from epic_harness_report()
 * (static inline in epic_harness.h) and are the only call sites in the
 * codebase that ever pass these literals. init() leaves RA0 driving low;
 * the toggle happens on log() (marker line drives RA0 to ok's value,
 * every other log line is a no-op). running() is bounded by `cycles` so
 * a run terminates on its own. Not in epic-common: the marker line needs
 * GPIO pin access, unlike epic_harness_target.c's four no-ops.
 */

#include "core/epic_harness.h"
#include "peripherals/pic16f193x_gpio.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 32000000UL
#endif

static uint32_t g_cycles = 0U;

void epic_harness_init(uint32_t cycles)
{
    g_cycles = cycles;

    /* RA0 as digital output. ANSELA<0> = 0 (analog default at POR,
     * DS41364B §6.0), TRISA<0> = 0 (input default), LATA<0> starts
     * at 0 (POR). The pass/fail marker drives RA0 from log() below. */
    EPIC_GPIO_Init(GPIOA, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
}

void epic_harness_tick(void)
{
    /* Real time advances on its own under MPLAB SIM too, nothing to pump. */
}

int epic_harness_running(uint32_t iteration)
{
    return (iteration < g_cycles) ? 1 : 0;
}

void epic_harness_log(const char *fmt, ...)
{
    /* Magic-string dispatch: on the two pass/fail markers from
     * epic_harness_report() drive RA0 from the meaning (PASS = high,
     * FAIL = low); every other log line is a no-op. */
    if (fmt && fmt[0] == 'E' && fmt[1] == 'P' && fmt[2] == 'I' &&
        fmt[3] == 'C' && fmt[4] == '_' && fmt[5] == 'H' &&
        fmt[6] == 'A' && fmt[7] == 'R' && fmt[8] == 'N' &&
        fmt[9] == 'E' && fmt[10] == 'S' && fmt[11] == 'S' &&
        fmt[12] == '_' && fmt[13] == 'R' && fmt[14] == 'E' &&
        fmt[15] == 'S' && fmt[16] == 'U' && fmt[17] == 'L' &&
        fmt[18] == 'T' && fmt[19] == ':' && fmt[20] == ' ' &&
        fmt[21] == 'P' && fmt[22] == 'A' && fmt[23] == 'S' &&
        fmt[24] == 'S' && fmt[25] == '\n' && fmt[26] == '\0') {
        EPIC_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    } else if (fmt && fmt[0] == 'E' && fmt[1] == 'P' && fmt[2] == 'I' &&
        fmt[3] == 'C' && fmt[4] == '_' && fmt[5] == 'H' &&
        fmt[6] == 'A' && fmt[7] == 'R' && fmt[8] == 'N' &&
        fmt[9] == 'E' && fmt[10] == 'S' && fmt[11] == 'S' &&
        fmt[12] == '_' && fmt[13] == 'R' && fmt[14] == 'E' &&
        fmt[15] == 'S' && fmt[16] == 'U' && fmt[17] == 'L' &&
        fmt[18] == 'T' && fmt[19] == ':' && fmt[20] == ' ' &&
        fmt[21] == 'F' && fmt[22] == 'A' && fmt[23] == 'I' &&
        fmt[24] == 'L' && fmt[25] == '\n' && fmt[26] == '\0') {
        EPIC_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
    }
    /* variadic args ignored; the marker has none. */
}

/* Freeze here so RA0 stays at its post-report value: XC8's `ljmp
 * start` epilogue would otherwise re-enter main() on return, and
 * epic_harness_init() would drive RA0 low again, flickering
 * PORTA<0> across the mdb `print PORTA` readback window. */
void pic16f193x_harness_halt(void)
{
    for (;;) {
        /* nothing */
    }
}
