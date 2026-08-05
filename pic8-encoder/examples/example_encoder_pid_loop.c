/**
 * @file    example_encoder_pid_loop.c
 * @brief   One encoder feeding encoder_get_position() into pid_update() as
 *          the measurement each control cycle.
 *
 * @details
 *   A simulated servo: each cycle reads the encoder, steps the PID, applies
 *   the output to a first-order-lag plant, then drives the encoder one
 *   quadrature edge at a time to match, so `encoder_get_position() ==
 *   motor_angle` exactly. Host-only, no floats.
 */

#include "encoder.h"
#include "pid.h"
#include "pic8_tick.h"
#include "pic8_hal.h"
#include "core/pic8_harness.h"

#include <stdio.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define PIN_A   4u
#define PIN_B   5u
#define PLANT_N 4       /* first-order-lag time constant in control steps */

/* Q8.8 helper (host-side convenience; the library takes pre-scaled gains). */
static int16_t q8(float x) { return (int16_t)(x * 256.0f); }

/* Gray state for a count, in the table's positive direction
 * (count: 0->00, 1->10, 2->11, 3->01, 4->00, ...). */
static uint8_t gray_state(int32_t count)
{
    static const uint8_t g[4] = { 0, 2, 3, 1 };
    return g[(uint32_t)count & 3U];
}

static uint8_t port_byte(uint8_t state)
{
    uint8_t v = 0U;
    if (state & 0x2U) v |= (uint8_t)(1U << PIN_A);
    if (state & 0x1U) v |= (uint8_t)(1U << PIN_B);
    return v;
}

/* Drive the encoder to `target` counts by feeding one Gray edge at a time,
 * so the decoder sees every single-bit transition (never a diagonal jump)
 * and encoder_get_position() == target exactly. */
static int32_t g_driven = 0;
static void drive_encoder_to(encoder_t *enc, int32_t target)
{
    while (g_driven < target) {
        g_driven++;
        encoder_update(enc, port_byte(gray_state(g_driven)));
    }
    while (g_driven > target) {
        g_driven--;
        encoder_update(enc, port_byte(gray_state(g_driven)));
    }
}

int main(void)
{
    pic8_harness_init(4000000UL);
    pic8_tick_init(FOSC_HZ);

    /* A PID tuned for a visible saturating step response (mirrors
     * example_pid_setpoint_step's choices). Output clamp tight enough that
     * anti-windup engages on the setpoint step. */
    pid_t pid;
    pid_init(&pid, q8(2.0f), q8(0.5f), q8(0.0f), -200, 200);

    /* Encoder on RB4/RB5, glitch gate off (the simulated motor is clean). */
    encoder_t enc;
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_4 | GPIO_PIN_5, GPIO_MODE_INPUT);
    PIC8_REG8(PIC_REG_PORTB) = port_byte(gray_state(0));
    encoder_init(&enc, PIN_A, PIN_B, 0, PIC8_REG8(PIC_REG_PORTB));

    int32_t motor_angle = 0;
    int16_t setpoint    = 100;

    printf("== Servo: PID closes on encoder_get_position(), setpoint %d ==\n",
           (int)setpoint);
    printf("step | setpoint | encoder_meas |   output  | motor_angle | integrator_q8\n");
    /* Convergence = the encoder reading settles within 1 count of the
     * setpoint for several consecutive cycles (the PID output need not
     * reach 0, only the measurement matters). */
    int settled = 0;
    int converged = 0;
    int last_step_logged = 0;
    for (int step = 0; step < 80; step++) {
        int32_t meas = encoder_get_position(&enc);     /* the sensor reading */
        int16_t output = pid_update(&pid, setpoint, (int16_t)meas);

        /* Plant: integer first-order lag, no floats. */
        int32_t delta = (output - motor_angle) / PLANT_N;
        motor_angle += delta;

        /* The motor's shaft turns the encoder; drive it to the new angle. */
        drive_encoder_to(&enc, motor_angle);

        if (step < 20 || step % 5 == 0) {   /* keep the log readable once settled */
            printf("%4d | %8d | %12ld | %8d | %11ld | %13ld\n",
                   step, (int)setpoint, (long)meas, (int)output,
                   (long)motor_angle, (long)pid.integrator_q8);
            last_step_logged = step;
        }

        int32_t now = encoder_get_position(&enc);
        int32_t err = now - setpoint;
        if (err < 0) err = -err;
        if (err <= 1) {
            if (++settled >= 5) { converged = 1; break; }
        } else {
            settled = 0;
        }
    }

    int32_t final_meas = encoder_get_position(&enc);
    printf("\nfinal encoder reading = %ld (setpoint %d)\n",
           (long)final_meas, (int)setpoint);
    printf("encoder errors = %u, glitches = %u\n",
           (unsigned)encoder_get_error_count(&enc),
           (unsigned)encoder_get_glitch_count(&enc));

    int32_t err = final_meas - setpoint;
    if (err < 0) err = -err;
    int ok = converged && (err <= 1) &&
             (encoder_get_error_count(&enc) == 0) &&
             (encoder_get_glitch_count(&enc) == 0);
    return pic8_harness_report(ok);
}
