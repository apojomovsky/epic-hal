/**
 * @file    pic8_harness_target.c
 * @brief   Real-target implementation of the test harness (@ref
 *          core/pic8_harness.h): four no-ops, since the CPU starts
 *          itself, time advances on its own, and there is no stdout.
 *          Family-blind, so the same object links against every family.
 */

#include "core/pic8_harness.h"

void pic8_harness_init(uint32_t cycles)
{
    (void)cycles;   /* The target starts itself; cycles are a host concept. */
}

void pic8_harness_tick(void)
{
    /* Real time advances on its own, nothing to pump. */
}

int pic8_harness_running(uint32_t iteration)
{
    (void)iteration;
    return 1;       /* Firmware runs forever. */
}

void pic8_harness_log(const char *fmt, ...)
{
    (void)fmt;      /* No stdout on the target. */
}
