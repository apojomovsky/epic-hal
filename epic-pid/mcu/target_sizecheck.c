/**
 * @file    target_sizecheck.c
 * @brief   Minimal on-target build proving pid.c cross-compiles for real
 *          XC8/PIC16/PIC18 silicon and reporting flash/RAM footprint; not
 *          a correctness test (see ../tests/test_pid.c for that).
 *
 * Measured footprint (XC8 -O2, full epic-math linked in, `pid_t` = 21 B):
 *
 *     Target       Program space              Data space (full)
 *     PIC16F877A   1787 words (21.8% of 8 KW)  142 B (38.6% of 368 B)
 *     PIC16F873A   1789 words (43.7% of 4 KW)  140 B (72.9% of 192 B)
 *     PIC18F4550   1734 B    (5.3%  of 32 KB)  163 B (8.0%  of 2 KB)
 */

#include "pid.h"

static pid_t g_pid;

int main(void)
{
    pid_init(&g_pid, (int16_t)0x0100, (int16_t)0x0001, (int16_t)0x0000,
             (int16_t)-1000, (int16_t)1000);
    for (;;) {
        (void)pid_update(&g_pid, (int16_t)0, (int16_t)0);
    }
}
