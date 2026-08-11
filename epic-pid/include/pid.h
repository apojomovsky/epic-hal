/**
 * Vendor-agnostic, single-loop, fixed-point (Q8.8) PID controller with
 * anti-windup, derivative-on-measurement, and bumpless auto/manual
 * transfer. Pure arithmetic, no HAL dependency; gains are pre-scaled by
 * Ts so pid_update needs no division. See docs/API.md for the gain
 * conversion.
 */

#ifndef PID_H
#define PID_H

#include <stdint.h>
#include <stdbool.h>

/** Auto / manual mode selector (see @ref pid_set_mode, @ref pid_update). */
typedef enum {
    PID_MODE_MANUAL = 0,
    PID_MODE_AUTO,
} pid_mode_t;

/**
 * One PID control loop, caller-owned storage. Fields are written by
 * pid_init / pid_set_* and by pid_update; the caller reads them through
 * the API.
 */
typedef struct {
    int16_t    kp_q8, ki_q8, kd_q8;   /* Q8.8 gains; ki_q8 includes *Ts, kd_q8 includes /Ts */
    int16_t    out_min, out_max;      /* actuator output clamp; out_min <= out_max */

    int32_t    integrator_q8;         /* Q8.8 integral term, clamped to [out_min,out_max]<<8 */
    int16_t    prev_measurement;      /* for derivative-on-measurement */
    bool       have_prev_measurement; /* false until first pid_update() since init/reset;
                                        * gates the D term to avoid a first-call kick */
    bool       skip_next_i_increment; /* set after a MANUAL call back-calculates the
                                        * integrator; the next AUTO call skips the I
                                        * increment (bumpless transfer), then clears this */
    pid_mode_t mode;
    int16_t    manual_output;         /* caller-set target output while mode == MANUAL */
} pid_t;

/**
 * Initialize a PID instance: stores gains and clamp range, sets AUTO
 * mode, zeroes the integrator and D-term history.
 *
 * @param kp_q8  Q8.8 proportional gain (= round(Kp * 256)).
 * @param ki_q8  Q8.8 integral gain, pre-multiplied by Ts (= round(Ki * Ts * 256)).
 * @param kd_q8  Q8.8 derivative gain, pre-divided by Ts (= round(Kd / Ts * 256)).
 * @param out_min / out_max  actuator clamp rails (out_min <= out_max).
 */
void pid_init(pid_t *pid, int16_t kp_q8, int16_t ki_q8, int16_t kd_q8,
              int16_t out_min, int16_t out_max);

/**
 * @brief  Zero the integrator and clear the D-term history, without losing
 *         tuning (gains/clamp/mode untouched), for recovering from an
 *         external fault (e-stop, sensor dropout).
 */
void pid_reset(pid_t *pid);

/**
 * @brief  Replace the three gains, leaving the integrator, D-term history,
 *         and mode untouched.
 */
void pid_set_gains(pid_t *pid, int16_t kp_q8, int16_t ki_q8, int16_t kd_q8);

/**
 * @brief  Switch between AUTO and MANUAL. Does NOT reset the integrator
 *         or D-term history; switching mode is not a fault, and bumpless
 *         transfer depends on integrator state carrying across the switch.
 */
void pid_set_mode(pid_t *pid, pid_mode_t mode);

/**
 * Set the target output used while mode == PID_MODE_MANUAL; only
 * consulted by pid_update() in MANUAL, ignored in AUTO. Call every cycle
 * the operator wants a new manual output in effect.
 */
void pid_set_manual_output(pid_t *pid, int16_t value);

/**
 * Step the controller once per fixed control-loop period; the single
 * per-cycle entry point. AUTO: `clamp((P+I+D) >> 8, out_min, out_max)`
 * with D from `-d(measurement)/dt` and the integrator clamped to
 * `[out_min, out_max] << 8` (anti-windup). MANUAL: `clamp(manual_output,
 * out_min, out_max)`, back-calculating the integrator so resuming AUTO is
 * bumpless. Precondition (not runtime-checked):
 * `|setpoint - measurement| <= 32767`.
 *
 * @return  the clamped output, always in `[out_min, out_max]`.
 */
int16_t pid_update(pid_t *pid, int16_t setpoint, int16_t measurement);

#endif /* PID_H */
