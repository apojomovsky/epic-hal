/**
 * Host simulator API for the PIC16F188xx ports, interrupt-on-change and
 * Timer0..Timer2. Host-only: never in an XC8 build.
 */
#ifndef PIC16F188XX_SIM_H
#define PIC16F188XX_SIM_H

#include <stdint.h>

typedef void (*pic16f188xx_sim_irq_cb_t)(void);

/**
 * @brief Reset all modelled SFRs to power-up values and clear inputs.
 */
void pic16f188xx_sim_reset(void);

/**
 * @brief Sample pins and IOC once per tick, advancing the timers each tick.
 * @param ticks Instruction ticks to advance.
 */
void pic16f188xx_sim_step(uint32_t ticks);

/**
 * @brief Drive an external level onto an input pin.
 * @param port 'A', 'B', or 'C' ('D'/'E' on 40-pin parts).
 * @param pin Bit number 0..7.
 * @param level Non-zero for high.
 */
void pic16f188xx_sim_drive_input(char port, uint8_t pin, uint8_t level);

/**
 * @brief Read the PORTx level of a pin as the simulator currently holds it.
 * @param port 'A', 'B', or 'C' ('D'/'E' on 40-pin parts).
 * @param pin Bit number 0..7.
 * @return 0 or 1; 0 for an invalid port or pin.
 */
uint8_t pic16f188xx_sim_read_output(char port, uint8_t pin);

/**
 * @brief Register the callback the simulator fires when an interrupt is pending.
 * @param cb Callback, or NULL to clear.
 */
void pic16f188xx_sim_set_irq_callback(pic16f188xx_sim_irq_cb_t cb);

#endif
