/**
 * Host simulator API for the PIC16F1508/1509 ports, IOC, CLC cells, and
 * timers. Host-only: never in an XC8 build.
 */
#ifndef PIC16F1508_SIM_H
#define PIC16F1508_SIM_H

#include <stdint.h>

typedef void (*pic16f1508_sim_irq_cb_t)(void);

/**
 * @brief Reset all modelled SFRs to power-up values and clear inputs.
 */
void pic16f1508_sim_reset(void);

/**
 * @brief Run the timers for a tick count, then sample pins, IOC and CLC once.
 * @param ticks Instruction cycles the timers advance; pins, IOC and CLC still sample once per call.
 */
void pic16f1508_sim_step(uint32_t ticks);

/**
 * @brief Inject one T1CKI rising edge into the TMR1 counter path.
 * @details Counts only while TMR1ON is set with the external source selected;
 * any other setup ignores the edge.
 */
void pic16f1508_sim_timer1_external_pulse(void);

/**
 * @brief Drive one Dx level into a CLC cell's gate logic.
 * @param cell Cell index 0..3 (CLC1 is 0). Out of range is ignored.
 * @param input Data input 1..4. Any other value is ignored.
 * @param level Non-zero drives high, zero drives low.
 */
void pic16f1508_sim_drive_clc_input(uint8_t cell, uint8_t input, uint8_t level);

/**
 * @brief Drive an external level onto an input pin.
 * @param port 'A', 'B', or 'C'.
 * @param pin Bit number 0..7.
 * @param level Non-zero for high.
 */
void pic16f1508_sim_drive_input(char port, uint8_t pin, uint8_t level);

/**
 * @brief Read the PORTx level of a pin as the simulator currently holds it.
 * @param port 'A', 'B', or 'C'.
 * @param pin Bit number 0..7.
 * @return 0 or 1; 0 for an invalid port or pin.
 */
uint8_t pic16f1508_sim_read_output(char port, uint8_t pin);

/**
 * @brief Register the callback the simulator fires when an interrupt is pending.
 * @param cb Callback, or NULL to clear.
 */
void pic16f1508_sim_set_irq_callback(pic16f1508_sim_irq_cb_t cb);

#endif
