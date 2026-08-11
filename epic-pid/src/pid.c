/**
 * Fixed-point PID controller (see pid.h). The Q8.8 `sum_q8 >> 8`
 * truncation relies on signed `>>` being arithmetic (sign-extending),
 * true on every target this ships on (host gcc/clang, XC8 PIC16/PIC18).
 */

#include "pid.h"
#include "pic_math.h"

void pid_init(pid_t *pid, int16_t kp_q8, int16_t ki_q8, int16_t kd_q8,
              int16_t out_min, int16_t out_max)
{
    pid->kp_q8   = kp_q8;
    pid->ki_q8   = ki_q8;
    pid->kd_q8   = kd_q8;
    pid->out_min = out_min;
    pid->out_max = out_max;

    pid->integrator_q8         = 0;
    pid->prev_measurement      = 0;
    pid->have_prev_measurement = false;
    pid->skip_next_i_increment = false;
    pid->mode                  = PID_MODE_AUTO;
    pid->manual_output         = 0;
}

void pid_reset(pid_t *pid)
{
    /* Fault-recovery reset: zero integrator/D-history/skip-flag, keep
     * gains, clamp, and mode untouched. */
    pid->integrator_q8         = 0;
    pid->have_prev_measurement = false;
    pid->skip_next_i_increment = false;
}

void pid_set_gains(pid_t *pid, int16_t kp_q8, int16_t ki_q8, int16_t kd_q8)
{
    pid->kp_q8 = kp_q8;
    pid->ki_q8 = ki_q8;
    pid->kd_q8 = kd_q8;
}

void pid_set_mode(pid_t *pid, pid_mode_t mode)
{
    pid->mode = mode;
}

void pid_set_manual_output(pid_t *pid, int16_t value)
{
    pid->manual_output = value;
}

int16_t pid_update(pid_t *pid, int16_t setpoint, int16_t measurement)
{
    /* P term: Kp * error in Q8.8; fits int32_t without an overflow guard
     * (max product ~1.07e9, well under INT32_MAX). */
    int16_t error = (int16_t)(setpoint - measurement);
    int32_t p_q8  = pic_math_mul_s16(pid->kp_q8, error);

    /* D term: -d(measurement)/dt, not d(error)/dt, to avoid setpoint-step
     * kick; zero on the first call (no previous measurement yet). */
    int16_t dmeas;
    if (!pid->have_prev_measurement) {
        dmeas = 0;
        pid->have_prev_measurement = true;
    } else {
        dmeas = (int16_t)(measurement - pid->prev_measurement);
    }
    pid->prev_measurement = measurement;
    int32_t d_q8 = -pic_math_mul_s16(pid->kd_q8, dmeas);

    /* Integrator clamp rails: this is the anti-windup mechanism. */
    int32_t out_min_q8 = (int32_t)pid->out_min << 8;
    int32_t out_max_q8 = (int32_t)pid->out_max << 8;

    if (pid->mode == PID_MODE_MANUAL) {
        /* MANUAL: clamp manual_output, then back-calculate the (also
         * clamped) integrator so the next AUTO call reproduces this exact
         * output; skip_next_i_increment suppresses that call's I term so
         * the back-calculation isn't double-counted. */
        int16_t output = pid->manual_output;
        if (output < pid->out_min) { output = pid->out_min; }
        if (output > pid->out_max) { output = pid->out_max; }
        pid->integrator_q8 = ((int32_t)output << 8) - p_q8 - d_q8;
        if (pid->integrator_q8 < out_min_q8) { pid->integrator_q8 = out_min_q8; }
        if (pid->integrator_q8 > out_max_q8) { pid->integrator_q8 = out_max_q8; }
        pid->skip_next_i_increment = true;
        return output;
    }

    /* AUTO: accumulate Ki*error unless the prior MANUAL call asked us to
     * skip it (bumpless handoff), then sum P+I+D and clamp. */
    if (!pid->skip_next_i_increment) {
        pid->integrator_q8 += pic_math_mul_s16(pid->ki_q8, error);
    }
    pid->skip_next_i_increment = false;  /* single-shot: consumed */

    if (pid->integrator_q8 < out_min_q8) { pid->integrator_q8 = out_min_q8; }
    if (pid->integrator_q8 > out_max_q8) { pid->integrator_q8 = out_max_q8; }

    int32_t sum_q8 = p_q8 + pid->integrator_q8 + d_q8;
    int16_t output = (int16_t)(sum_q8 >> 8);
    if (output < pid->out_min) { output = pid->out_min; }
    if (output > pid->out_max) { output = pid->out_max; }
    return output;
}
