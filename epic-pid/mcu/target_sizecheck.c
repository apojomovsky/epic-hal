/**
 * Minimal on-target build proving pid.c cross-compiles for real
 * XC8/PIC16/PIC18 silicon and reports flash/RAM footprint; not a
 * correctness test (see ../tests/test_pid.c).
 */

#include "pid.h"

static pid_t g_pid;

/**
 * @brief On-target build proof and footprint report.
 *
 * Runs the control loop forever on real silicon; never returns.
 *
 * @return never returns on target
 */
int main(void)
{
    pid_init(&g_pid, (int16_t)0x0100, (int16_t)0x0001, (int16_t)0x0000,
             (int16_t)-1000, (int16_t)1000);
    for (;;) {
        (void)pid_update(&g_pid, (int16_t)0, (int16_t)0);
    }
}
