/**
 * @file    epic_harness_target.c
 * @brief   Real-target implementation of the test harness (@ref
 *          core/epic_harness.h): four no-ops, since the CPU starts
 *          itself, time advances on its own, and there is no stdout.
 *          Family-blind, so the same object links against every family.
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
