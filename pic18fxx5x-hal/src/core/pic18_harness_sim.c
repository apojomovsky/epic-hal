/**
 * @file    pic18_harness_sim.c
 * @brief   PIC18F2455 family host-simulation implementation of the test
 *          harness (see core/pic8_harness.h).
 *
 * @details
 *   Linked by the CMake host build; the companion target implementation is
 *   the family-blind `pic8_harness_target.c` in `pic8-common`, so neither
 *   this file nor the examples need `#ifdef`. PIC18-specific only because
 *   it pumps the PIC18 simulator; the harness contract itself is shared.
 */

#include "core/pic8_harness.h"   /* pic8_dispatch_all_irqs is declared here */
#include "pic18fxx5x_sim.h"

#include <stdio.h>
#include <stdarg.h>

/** Bounded run length set by the last harness_init() call. */
static uint32_t g_cycles = 0U;

void pic8_harness_init(uint32_t cycles)
{
    g_cycles = cycles;
    pic18_sim_reset();
    /* One sim callback that fans out to every peripheral handler, the
     * host analogue of the real target's two interrupt vectors. */
    pic18_sim_set_irq_callback(pic8_dispatch_all_irqs);
}

void pic8_harness_tick(void)
{
    pic18_sim_step(1);
}

int pic8_harness_running(uint32_t iteration)
{
    return (iteration < g_cycles) ? 1 : 0;
}

void pic8_harness_log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    (void)vprintf(fmt, ap);
    va_end(ap);
}
