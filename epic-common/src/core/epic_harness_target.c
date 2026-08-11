/**
 * Real-target implementation of the harness: no-ops, since the CPU
 * starts itself, time advances on its own, and there is no stdout.
 */

#include "core/epic_harness.h"

void epic_harness_init(uint32_t cycles)
{
    (void)cycles;   /* The target starts itself; cycles are a host concept. */
}

void epic_harness_tick(void)
{
    /* Real time advances on its own, nothing to pump. */
}

int epic_harness_running(uint32_t iteration)
{
    (void)iteration;
    return 1;       /* Firmware runs forever. */
}

void epic_harness_log(const char *fmt, ...)
{
    (void)fmt;      /* No stdout on the target. */
}
