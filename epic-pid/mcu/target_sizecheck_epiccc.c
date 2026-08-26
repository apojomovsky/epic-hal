/* Real-driver epic-cc build and PORTB execution gate: links the real
 * pid.c (the host epic_math mul is supplied by epic_build.py) and runs
 * one pure-P step whose clamped output lands on PORTB, so mdb-hex can
 * assert the control loop really executed under epic-cc. */
#include "pid.h"
#include "epic_hal.h"

/* No manifest config words on this path; keep the WDT off so the
 * MPLAB SIM gate is not reset mid-run. */
EPIC_CONFIG("osc=hs, wdt=off, xtal_hz=20000000");

/* PORTB as output, value written below (the mdb-hex execution gate). */
#define TRISB_REG PIC_REG_TRISB
#define PORTB_REG PIC_REG_PORTB

static epic_pid_t g_pid;

/** @brief Main. @return 0. */
int main(void)
{
    /* kp = 1.0 Q8.8, error = 100 -> output 100 (0x64), the pure-P
     * oracle from tests/test_pid.c. */
    epic_pid_init(&g_pid, (int16_t)0x0100, (int16_t)0, (int16_t)0,
                  (int16_t)-1000, (int16_t)1000);
    int16_t out = epic_pid_update(&g_pid, (int16_t)100, (int16_t)0);
    EPIC_REG8(TRISB_REG) = (uint8_t)0x00u;
    EPIC_REG8(PORTB_REG) = (uint8_t)out;
    for (;;) {
    }
}
