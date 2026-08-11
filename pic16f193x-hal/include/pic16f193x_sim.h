/**
 * Public API for the PIC16F193X host simulation backend. On the host
 * build every SFR access indexes a host-side register file
 * (include/host/pic16f193x_platform.h), one byte per 12-bit data-memory
 * address (DS41364B §2.2). These hooks let test code drive input pins,
 * read output-pin levels, advance simulated time via
 * pic16f193x_sim_step(), and hook the interrupt callback.
 */

#ifndef PIC16F193X_SIM_H
#define PIC16F193X_SIM_H

#include <stdint.h>
#include <stdbool.h>
#include "pic16f193x.h"

/**
 * @brief Initialize the simulation backend and reset every SFR to its
 *        power-on-reset value. Must be called before any HAL call.
 */
void pic16f193x_sim_reset(void);

/**
 * @brief Advance the simulated peripherals by `ticks` instruction cycles.
 *        Drives Timer0 and refreshes the GPIO pin-level model.
 */
void pic16f193x_sim_step(uint32_t ticks);

/**
 * @brief Drive a digital input pin from the test rig (when the pin is
 *        configured as input, TRIS bit = 1).
 *
 * @param port   One of 'A'..'E'.
 * @param pin    Pin number 0..7.
 * @param level  0 = low, 1 = high.
 */
void pic16f193x_sim_drive_input(char port, uint8_t pin, uint8_t level);

/**
 * @brief Read the level currently driven onto an output pin (what an
 *        external load would see). For pins configured as outputs, this
 *        returns the latched value in LATx. For inputs, this returns the
 *        last value driven via @ref pic16f193x_sim_drive_input.
 */
uint8_t pic16f193x_sim_read_output(char port, uint8_t pin);

/**
 * @brief Hook a user callback fired whenever the simulated CPU would take
 *        an interrupt. Lets tests assert "an interrupt should fire here".
 */
typedef void (*pic16f193x_sim_irq_cb_t)(void);
void pic16f193x_sim_set_irq_callback(pic16f193x_sim_irq_cb_t cb);

#endif /* PIC16F193X_SIM_H */
