/**
 * Host-sim harness: drives the simulator one step per tick and routes the
 * interrupt callback into the shared dispatcher.
 */
#include <stdarg.h>
#include <stdio.h>
#include "core/epic_harness.h"
#include "core/pic16f1508_irq.h"
#include "pic16f1508_sim.h"

static uint32_t g_cycles;

/**
 * @brief Reset the simulator and bind the interrupt callback.
 * @param cycles Run bound for epic_harness_running.
 */
void epic_harness_init(uint32_t cycles)
{
    g_cycles = cycles;
    pic16f1508_sim_reset();
    pic16f1508_sim_set_irq_callback(epic_dispatch_all_irqs);
}

/**
 * @brief Advance the simulator by one step.
 */
void epic_harness_tick(void)
{
    pic16f1508_sim_step(1U);
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
 * @brief Write a formatted line to stdout.
 * @param fmt printf-style format string.
 */
void epic_harness_log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

/**
 * @brief No-op on host: the process exits when main returns.
 */
void pic16f1508_harness_halt(void)
{
}
