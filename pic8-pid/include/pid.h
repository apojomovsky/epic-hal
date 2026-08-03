/**
 * @file    pid.h
 * @brief   Vendor-agnostic, single-loop, fixed-point (Q8.8) PID controller
 *          with anti-windup, derivative-on-measurement, and bumpless
 *          auto/manual transfer.
 *
 * @details
 *   One caller-owned `pid_t` per control loop; call `pid_update()` once per
 *   fixed period `Ts`. Pure arithmetic, no HAL dependency, compiles
 *   unchanged for host/PIC16/PIC18 (uses `pic8-math` for the 16x16->32
 *   multiply). Gains are caller-pre-scaled by `Ts` so `pid_update` needs no
 *   division; see `docs/ARCHITECTURE.md` for the full design rationale and
 *   `docs/API.md` for the Kp/Ki/Kd/Ts -> kp_q8/ki_q8/kd_q8 conversion.
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
 * @brief  One PID control loop, caller-owned storage.
 *
 * Fields are written by `pid_init` / `pid_set_*` and by `pid_update` (the
 * integrator, the D-term history, and the mode's back-calculation). The
 * caller reads them through the API; reading them directly is unsupported
 * but harmless.
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
 * @brief  Initialize a PID instance: stores gains and clamp range, sets
 *         AUTO mode, zeroes the integrator and D-term history.
 *
 * @param  pid       the instance (caller-owned storage).
 * @param  kp_q8     Q8.8 proportional gain (= round(Kp * 256)).
 * @param  ki_q8     Q8.8 integral gain, pre-multiplied by sample period Ts
 *                   (= round(Ki * Ts * 256)).
 * @param  kd_q8     Q8.8 derivative gain, pre-divided by Ts
 *                   (= round(Kd / Ts * 256)).
 * @param  out_min   minimum output (clamp rail; out_min <= out_max).
 * @param  out_max   maximum output (clamp rail).
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
 * @brief  Set the target output used while `mode == PID_MODE_MANUAL`.
 *         Only consulted by `pid_update()` while in MANUAL; ignored in AUTO.
 *         Call every cycle the operator/supervisor wants a new manual
 *         output in effect.
 */
void pid_set_manual_output(pid_t *pid, int16_t value);

/**
 * @brief  Step the controller once per fixed control-loop period, in
 *         either mode; the single per-cycle entry point.
 *
 * AUTO: `clamp((P+I+D) >> 8, out_min, out_max)`, D from
 * `-d(measurement)/dt`, integrator clamped to `[out_min, out_max] << 8`
 * for anti-windup. MANUAL: `clamp(manual_output, out_min, out_max)`, and
 * back-calculates the integrator so resuming AUTO is bumpless. Precondition
 * (not runtime-checked): `|setpoint - measurement| <= 32767`.
 *
 * @return  the clamped output, always in `[out_min, out_max]`.
 */
int16_t pid_update(pid_t *pid, int16_t setpoint, int16_t measurement);

#endif /* PID_H */
