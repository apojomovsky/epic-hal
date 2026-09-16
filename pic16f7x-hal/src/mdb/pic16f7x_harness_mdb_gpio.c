/* Sim-target harness for the 16F72: this die has no USART, so the
 * PASS/FAIL marker goes out on RA0 (MODE=gpio), not over UART. PASS
 * drives RA0 high and leaves FAIL low; non-marker log lines are ignored.
 * running() is bounded by `cycles`, matching the UART harness. */

#include "core/epic_harness.h"
#include "peripherals/pic16f7x_gpio.h"
#include "target/pic16f7x_platform.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

static uint32_t g_cycles = 0U;

/**
 * @brief Harness start-up (sim target): store the cycle bound and arm
 *        RA0 as the marker output, driven low until a PASS report.
 * @param cycles the number of iterations the run is bounded by.
 */
void epic_harness_init(uint32_t cycles)
{
    g_cycles = cycles;
    /* RA0 boots under ADC control, so TRIS alone cannot drive the
     * marker pin; PCFG = 0b111 releases every analog input to digital.
     */
    EPIC_BANK1_WRITE8(ADCON1, 0x07u);
    EPIC_GPIO_Init(GPIOA, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
}

/**
 * @brief Advance time. No-op: real time advances on its own under MPLAB
 *        SIM, nothing to pump.
 */
void epic_harness_tick(void)
{
    /* Real time advances on its own under MPLAB SIM. */
}

/**
 * @brief Loop-continuation test: 1 while the bounded run is in progress.
 * @param iteration the current 0-based iteration index.
 * @return 1 while the run should continue, 0 when it is over.
 */
int epic_harness_running(uint32_t iteration)
{
    return (iteration < g_cycles) ? 1 : 0;
}

/**
 * @brief Log line: the trimmed PASS/FAIL marker dispatch drives RA0.
 * @param fmt printf-style format string; variadic arguments are ignored.
 */
void epic_harness_log(const char *fmt, ...)
{
    /* The only callers are epic_harness_report's two fixed literals, so
     * the distinctive bytes pin the verdict: "...RESULT: PASS\n" and
     * "...RESULT: FAIL\n". The fmt[23:24] pair ('SS' vs 'IL') rejects
     * near-marker prefixes.
     */
    if (fmt && fmt[0] == 'E' && fmt[19] == ':' && fmt[20] == ' ')
    {
        /* RA0 reverts to analog when the probe resets ADCON1, so re-arm
         * PCFG = 0b111 on the marker itself before driving the pin.
         */
        EPIC_BANK1_WRITE8(ADCON1, 0x07u);
        if (fmt[21] == 'P' && fmt[22] == 'A' && fmt[23] == 'S' &&
            fmt[24] == 'S')
        {
            EPIC_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
        }
        else if (fmt[21] == 'F' && fmt[22] == 'A' && fmt[23] == 'I' &&
                 fmt[24] == 'L')
        {
            EPIC_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
        }
    }
    /* Variadic arguments are ignored; the markers have none. */
}
