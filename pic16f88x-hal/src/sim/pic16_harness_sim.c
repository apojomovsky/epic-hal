/* Host-simulation implementation of the test harness (core/epic_harness.h).
 * Linked by the CMake host build; the family-blind target twin is
 * epic_harness_target.c in epic-common. PIC16-specific only because it
 * pumps the PIC16 simulator. */

#include "core/epic_harness.h"   /* epic_dispatch_all_irqs is declared here */
#include "pic16f88x_sim.h"

#include <stdio.h>
#include <stdarg.h>

/** Bounded run length set by the last harness_init() call. */
static uint32_t g_cycles = 0U;

/**
 * @brief Initialize the test harness for the host simulation: record the
 *        run length, reset the simulator, and hook the IRQ dispatcher as
 *        the sim interrupt callback.
 * @param cycles the number of sim cycles the run is bounded by.
 */
void epic_harness_init(uint32_t cycles)
{
    g_cycles = cycles;
    pic16f88x_sim_reset();
    /* One sim callback that fans out to every peripheral handler, the
     * host analogue of the real target's single interrupt vector. */
    pic16f88x_sim_set_irq_callback(epic_dispatch_all_irqs);
}

/**
 * @brief Advance the simulated time by one instruction cycle.
 */
void epic_harness_tick(void)
{
    pic16f88x_sim_step(1);
}

/**
 * @brief Report whether the run should continue.
 * @param iteration the current 0-based iteration index.
 * @return 1 while `iteration` is below the configured cycle bound, else 0.
 */
int epic_harness_running(uint32_t iteration)
{
    return (iteration < g_cycles) ? 1 : 0;
}

/**
 * @brief Log a printf-style line to stdout.
 * @param fmt the printf format string.
 */
void epic_harness_log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    (void)vprintf(fmt, ap);
    va_end(ap);
}
