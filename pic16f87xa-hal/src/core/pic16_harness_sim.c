/**
 * @file    pic16_harness_sim.c
 * @brief   PIC16F87XA host-simulation implementation of the test harness
 *          (see core/epic_harness.h).
 *
 * @details
 *   Linked by the CMake host build. The companion target implementation
 *   is the family-blind epic_harness_target.c in epic-common; the build
 *   picks one, so neither this file nor the examples need `#ifdef`. This
 *   file is PIC16-specific only because it pumps the PIC16 simulator; the
 *   harness contract it implements is shared by every family.
 */

#include "core/epic_harness.h"   /* epic_dispatch_all_irqs is declared here */
#include "pic16f87xa_sim.h"

#include <stdio.h>
#include <stdarg.h>

/** Bounded run length set by the last harness_init() call. */
static uint32_t g_cycles = 0U;

void epic_harness_init(uint32_t cycles)
{
    g_cycles = cycles;
    pic16f87xa_sim_reset();
    /* One sim callback that fans out to every peripheral handler, the
     * host analogue of the real target's single interrupt vector. */
    pic16f87xa_sim_set_irq_callback(epic_dispatch_all_irqs);
}

void epic_harness_tick(void)
{
    pic16f87xa_sim_step(1);
}

int epic_harness_running(uint32_t iteration)
{
    return (iteration < g_cycles) ? 1 : 0;
}

void epic_harness_log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    (void)vprintf(fmt, ap);
    va_end(ap);
}
