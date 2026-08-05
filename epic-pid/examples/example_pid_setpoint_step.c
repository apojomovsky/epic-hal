/**
 * @file    example_pid_setpoint_step.c
 * @brief   Host-only setpoint-step + manual/auto transfer demo for epic-pid.
 *
 * @details
 *   Pure-host, no HAL/XC8 build; a first-order-lag integer "plant"
 *   (`measurement += (output - measurement) / 4`) driven by a
 *   deliberately tight output clamp so anti-windup visibly engages on a
 *   setpoint step, then a MANUAL -> AUTO handoff shows the resume is
 *   bumpless (matches the last MANUAL output exactly).
 */

#include "pid.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* Host-side Q8.8 conversion; the library itself takes pre-scaled gains. */
static int16_t q8(float x) { return (int16_t)(x * 256.0f); }

int main(void)
{
    /* Kp=2.0/Ki=0.5/Kd=0.0 and a tight output clamp are chosen to make
     * saturation and anti-windup clearly visible in the log. */
    const int16_t kp_q8 = q8(2.0f);
    const int16_t ki_q8 = q8(0.5f);
    const int16_t kd_q8 = q8(0.0f);
    const int16_t out_min = -200;
    const int16_t out_max = 200;

    pid_t pid;
    pid_init(&pid, kp_q8, ki_q8, kd_q8, out_min, out_max);

    int16_t measurement = 0;
    int16_t setpoint    = 0;

    /* Phase 1: setpoint step 0 -> 100; output should saturate briefly
     * then the plant catches up as anti-windup releases the integrator. */
    setpoint = 100;
    printf("== Setpoint step 0 -> 100 (AUTO) ==\n");
    printf("step | setpoint | measurement | output | integrator_q8 | mode\n");
    for (int step = 0; step < 30; step++) {
        int16_t output = pid_update(&pid, setpoint, measurement);
        /* Plant: integer first-order lag, no floats. */
        int16_t plant_delta = (int16_t)((output - measurement) / 4);
        measurement = (int16_t)(measurement + plant_delta);
        printf("%4d | %8d | %11d | %6d | %13d | %s\n",
               step, setpoint, measurement, output,
               pid.integrator_q8,
               pid.mode == PID_MODE_AUTO ? "AUTO" : "MANUAL");
    }

    /* Phase 2: switch to MANUAL; the operator drives the plant directly
     * via pid_set_manual_output while the integrator back-calculates. */
    printf("\n== Switch to MANUAL, operator takes over (target 50) ==\n");
    pid_set_mode(&pid, PID_MODE_MANUAL);
    pid_set_manual_output(&pid, 50);
    for (int step = 30; step < 40; step++) {
        int16_t output = pid_update(&pid, setpoint, measurement);
        int16_t plant_delta = (int16_t)((output - measurement) / 4);
        measurement = (int16_t)(measurement + plant_delta);
        printf("%4d | %8d | %11d | %6d | %13d | %s\n",
               step, setpoint, measurement, output,
               pid.integrator_q8,
               pid.mode == PID_MODE_AUTO ? "AUTO" : "MANUAL");
    }

    /* With the plant frozen, a MANUAL call followed by an AUTO call at
     * the same setpoint/measurement returns the exact same output. */
    int16_t frozen_setpoint = setpoint;
    int16_t frozen_measurement = measurement;
    int16_t new_manual = 75;
    printf("\n== Bumpless-equivalence demo (plant frozen) ==\n");
    printf("  setpoint=%d  measurement=%d\n", frozen_setpoint, frozen_measurement);
    pid_set_mode(&pid, PID_MODE_MANUAL);
    pid_set_manual_output(&pid, new_manual);
    int16_t held_out = pid_update(&pid, frozen_setpoint, frozen_measurement);
    printf("  MANUAL output = %d  (operator's target was %d)\n", held_out, new_manual);
    pid_set_mode(&pid, PID_MODE_AUTO);
    int16_t first_auto = pid_update(&pid, frozen_setpoint, frozen_measurement);
    printf("  first AUTO output = %d  %s\n", first_auto,
           (first_auto == held_out) ? "(matches MANUAL exactly -- bumpless)"
                                    : "(differs -- check the test suite's assert)");
    /* Second AUTO call: integrator resumes evolving, output diverges. */
    int16_t second_auto = pid_update(&pid, frozen_setpoint, frozen_measurement);
    printf("  second AUTO output = %d  (integrator resumes evolving)\n", second_auto);

    /* Phase 3: switch back to AUTO. Internal state is continuous (no
     * integrator jump), but the plant moved, so the output isn't pinned. */
    printf("\n== Switch back to AUTO (controller state is continuous, "
           "but the plant moved one step) ==\n");
    pid_set_mode(&pid, PID_MODE_AUTO);
    for (int step = 40; step < 70; step++) {
        int16_t output = pid_update(&pid, setpoint, measurement);
        int16_t plant_delta = (int16_t)((output - measurement) / 4);
        measurement = (int16_t)(measurement + plant_delta);
        printf("%4d | %8d | %11d | %6d | %13d | %s\n",
               step, setpoint, measurement, output,
               pid.integrator_q8,
               pid.mode == PID_MODE_AUTO ? "AUTO" : "MANUAL");
    }

    return 0;
}
