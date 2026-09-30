/*
 * PIC18F2455-family host-simulation implementation of the test harness
 * (see core/epic_harness.h), linked by the CMake host build. The target
 * implementation is the family-blind `epic_harness_target.c` in
 * `epic-common`, so neither this file nor the examples need `#ifdef`;
 * this file is PIC18-specific only because it pumps the PIC18 simulator.
 */

#include "core/epic_harness.h"   /* epic_dispatch_all_irqs is declared here */
#include "pic18fxx5x_sim.h"

#include <stdio.h>
#include <stdarg.h>

/** Bounded run length set by the last harness_init() call. */
static uint32_t g_cycles = 0U;

/**
 * @brief  Harness start-up (host-sim build): resets the simulated CPU,
 *         wires the sim IRQ callback to the family dispatcher and stores
 *         `cycles` as the bound for the upcoming run.
 * @param cycles bound on the run: simulated instruction cycles to pump
 *               before the harness reports the run over.
 */
void epic_harness_init(uint32_t cycles)
{
    g_cycles = cycles;
    pic18_sim_reset();
    /* One sim callback that fans out to every peripheral handler, the
     * host analogue of the real target's two interrupt vectors. */
    pic18_sim_set_irq_callback(epic_dispatch_all_irqs);
}

/**
 * @brief  Advance simulated time by one instruction cycle (host-sim
 *         build): pumps the simulator.
 */
void epic_harness_tick(void)
{
    pic18_sim_step(1);
}

/**
 * @brief  Loop-continuation test (host-sim build): returns 1 while the
 *         bounded run is in progress, 0 when it is over.
 * @param iteration the current loop index.
 * @return 1 while the run should continue, 0 when the host run is over.
 */
int epic_harness_running(uint32_t iteration)
{
    return (iteration < g_cycles) ? 1 : 0;
}

/**
 * @brief  printf-style log line (host-sim build): prints to stdout.
 * @param fmt printf-style format string; remaining arguments follow.
 */
void epic_harness_log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    (void)vprintf(fmt, ap);
    va_end(ap);
}
