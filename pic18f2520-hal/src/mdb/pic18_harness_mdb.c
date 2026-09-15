/*
 * HARNESS=sim harness: epic_harness_log() drives RA0 on the
 * EPIC_HARNESS_RESULT marker so the mdb gate reads it via print in
 * MODE=gpio (no EUSART this phase; the pic16f193x pattern). init()
 * leaves RA0 low; running() bounds the run by `cycles`.
 */

#include "core/epic_harness.h"
#include "peripherals/pic18f2520_gpio.h"

#include <stdint.h>

static uint32_t g_cycles = 0U;

/**
 * @brief Harness start-up (sim target): stores the cycle bound and
 *        configures RA0 (PORTA bit 0) as a digital output driven low.
 * @param cycles bound on the run: simulated instruction cycles to pump
 *               before the run is reported over.
 */
void epic_harness_init(uint32_t cycles)
{

    g_cycles = cycles;

    /* RA0 as digital output. TRISA<0> = 0 (default is input per
     * DS39631E §10.0), LATA<0> starts at 0 (POR). The pass/fail marker
     * drives RA0 from log() below. */
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
 * @brief Loop-continuation test: 1 while the bounded run is in progress,
 *        so a run terminates on its own.
 * @param iteration the current loop index.
 * @return 1 while the run should continue, 0 when it is over.
 */
int epic_harness_running(uint32_t iteration)
{

    return (iteration < g_cycles) ? 1 : 0;
}

/**
 * @brief Log line: on the two pass/fail markers from
 *        epic_harness_report() drives RA0 from the meaning (PASS = high,
 *        FAIL = low); every other line is a no-op.
 * @param fmt printf-style format string; variadic arguments are ignored
 *            (the markers have none).
 */
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
        fmt[24] == 'S' && fmt[25] == '\n' && fmt[26] == '\0')
    {
        EPIC_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    }
    else if (fmt && fmt[0] == 'E' && fmt[1] == 'P' && fmt[2] == 'I' &&
        fmt[3] == 'C' && fmt[4] == '_' && fmt[5] == 'H' &&
        fmt[6] == 'A' && fmt[7] == 'R' && fmt[8] == 'N' &&
        fmt[9] == 'E' && fmt[10] == 'S' && fmt[11] == 'S' &&
        fmt[12] == '_' && fmt[13] == 'R' && fmt[14] == 'E' &&
        fmt[15] == 'S' && fmt[16] == 'U' && fmt[17] == 'L' &&
        fmt[18] == 'T' && fmt[19] == ':' && fmt[20] == ' ' &&
        fmt[21] == 'F' && fmt[22] == 'A' && fmt[23] == 'I' &&
        fmt[24] == 'L' && fmt[25] == '\n' && fmt[26] == '\0')
    {
        EPIC_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
    }
    /* variadic args ignored; the marker has none. */
}

/**
 * @brief Freeze here so RA0 stays at its post-report value: XC8's `ljmp
 *        start` epilogue would otherwise re-enter main() on return, and
 *        epic_harness_init() would drive RA0 low again, flickering
 *        PORTA<0> across the mdb `print PORTA` readback window.
 */
void pic18f2520_harness_halt(void)
{

    for (;;)
    {

        /* nothing */
    }
}
