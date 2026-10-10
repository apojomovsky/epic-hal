/**
 * Host simulator API for the PIC16F1508/1509 port and IOC model. Host-only:
 * never in an XC8 build.
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
 * @brief Sample pins once and evaluate interrupt-on-change.
 * @param ticks Accepted for API parity with the other families; not used (no time base).
 */
void pic16f1508_sim_step(uint32_t ticks);

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
