/* Fixed-point PID loop on epic-pid, logged over the UART: a repeating
 * setpoint-step scenario (AUTO step, operator MANUAL takeover, then
 * bumpless AUTO resume) runs at a fixed 10 ms control period from
 * epic-tick against a small first-order software plant, printing one
 * line per step at 115200 baud. Wire a USB-serial adapter to the USART
 * TX/RX pins to watch the log. */

#include <stdint.h>
#include <stdio.h>

#include "pid.h"
#include "epic_tick.h"
#include "epic_serial.h"
#include "epic_hal.h"

#define CONTROL_HZ   100u         /* control-loop rate; Ts = 10 ms */
#define STEP_MS      (1000u / CONTROL_HZ)
#define SCENARIO_LEN 70u          /* steps per repeating scenario */
#define STEP_SETPOINT 1u          /* step 1: setpoint 0 -> 100 */
#define STEP_MANUAL   30u         /* steps 30..39: operator in MANUAL */
#define STEP_RESUME   40u         /* step 40: back to AUTO */

/**
 * @brief Run the repeating setpoint-step scenario forever, logging each step.
 */
int main(void)
{
    epic_serial_init(FOSC_HZ, 115200u);
    epic_tick_init(FOSC_HZ);
    EPIC_IRQ_Restore(1);

    /* Kp=2.0 (512), Ki=50/s = 0.5 per 10 ms step (128), Kd=0 and a tight
     * +/-200 clamp are chosen so the step saturates the output, showing
     * anti-windup release and a clean bumpless MANUAL to AUTO handoff in
     * the log. Gains are integer Q8.8 literals, so no FP runtime is
     * linked on 8-bit targets. */
    epic_pid_t pid;
    epic_pid_init(&pid, 512, 128, 0, (int16_t)-200, (int16_t)200);

    int16_t setpoint = 0;
    int16_t measurement = 0;
    uint16_t step = 0;
    uint8_t auto_mode = 1;
    uint32_t last = epic_tick_get();

    for (;;) {
        if (epic_tick_elapsed_since(last) < STEP_MS) {
            continue;
        }
        last = epic_tick_get();

        uint16_t s = step % SCENARIO_LEN;
        if (s == 0u) {
            printf(" step   set   meas   out  mode\n");
            setpoint = 0;
            measurement = 0;
            epic_pid_reset(&pid); /* re-arm the loop for the next run */
        }
        if (s == STEP_SETPOINT) {
            setpoint = 100;
        }
        if (s == STEP_MANUAL) {
            epic_pid_set_manual_output(&pid, 50);
            epic_pid_set_mode(&pid, EPIC_PID_MODE_MANUAL);
            auto_mode = 0;
        }
        if (s == STEP_RESUME) {
            epic_pid_set_mode(&pid, EPIC_PID_MODE_AUTO);
            auto_mode = 1;
        }

        int16_t output = epic_pid_update(&pid, setpoint, measurement);
        /* First-order stand-in for the real process: the actuator output
         * drives the measurement a quarter of the way each step. */
        measurement += (int16_t)((output - measurement) / 4);

        printf("%4u %5d %5d %5d %s\n", (unsigned)step, setpoint, measurement,
               output, auto_mode ? "AUTO" : "MANUAL");
        step++;
    }
}
