/* Pure footprint probe for epic-cc: no HAL, no tick, no serial. */
#include "pid.h"

static epic_pid_t g_pid;

/** @brief Main. @return 0. */
int main(void)
{
    // Pure footprint: just reference pid struct to pull pid.c, no calls
    // that hit host math isel gaps (smax/smin) or mul.
    (void)g_pid.kp_q8;
    return 0;
}
