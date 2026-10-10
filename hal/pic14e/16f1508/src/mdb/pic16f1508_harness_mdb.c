/**
 * mdb gate harness. Reports through RA0: driven low at init, set on
 * "EPIC_HARNESS_RESULT: PASS", cleared on FAIL. The gate halts the target
 * and reads PORTA bit 0.
 */
#include <stdarg.h>
#include "epic_hal.h"
#include "peripherals/pic16f1508_gpio.h"
#include "core/epic_harness.h"

static uint32_t g_cycles;

/**
 * @brief Check whether a log format string equals a marker.
 * @param fmt Format string as passed to epic_harness_log.
 * @param marker Expected string, including its trailing newline.
 * @return 1 on an exact match, 0 otherwise.
 */
static int marker_is(const char *fmt, const char *marker)
{
    while (*marker != '\0')
    {
        if (*fmt != *marker)
        {
            return 0;
        }
        fmt++;
        marker++;
    }
    return (*fmt == '\0');
}

/**
 * @brief Arm the harness and drive the RA0 result pin low.
 * @param cycles Run bound; the mdb gate does not use it.
 */
void epic_harness_init(uint32_t cycles)
{
    g_cycles = cycles;
    EPIC_GPIO_Init(GPIOA, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
}

/**
 * @brief Per-iteration hook; nothing to pump on target.
 */
void epic_harness_tick(void)
{
}

/**
 * @brief Loop bound check for the harness run loop.
 * @param iteration Current iteration index.
 * @return Non-zero while iteration is below the cycle bound.
 */
int epic_harness_running(uint32_t iteration)
{
    return iteration < g_cycles;
}

/**
 * @brief Harness log sink: sets RA0 on the PASS marker and clears it on FAIL.
 * @param fmt Format string; only the exact result markers have an effect.
 */
void epic_harness_log(const char *fmt, ...)
{
    if (marker_is(fmt, "EPIC_HARNESS_RESULT: PASS\n"))
    {
        EPIC_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    }
    else if (marker_is(fmt, "EPIC_HARNESS_RESULT: FAIL\n"))
    {
        EPIC_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
    }
}

/**
 * @brief Park the target after the result is reported.
 * @details Loops forever so RA0 keeps the reported value. Returning from main
 * would let the XC8 startup code jump back to start and drive RA0 low again.
 */
void pic16f1508_harness_halt(void)
{
    for (;;)
    {
    }
}
