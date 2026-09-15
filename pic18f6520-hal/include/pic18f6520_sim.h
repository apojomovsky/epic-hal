/*
 * Host simulation backend API: drive input pins, read output levels,
 * advance peripherals by N cycles, register an IRQ callback. Host SFR
 * accesses index a memory-backed register file (include/host/). Names
 * mirror pic16f87xa_sim_*; covers Timer0 plus GPIO this phase.
 */

#ifndef PIC18F6520_SIM_H
#define PIC18F6520_SIM_H

#include <stdint.h>
#include "pic18f6520_hal.h"

/**
 * @brief Initialize the simulation backend and reset every SFR to its
 *        power-on-reset value (DS39609B Table 3-3/4-3). Must be called
 *        before any HAL call.
 */
void pic18_sim_reset(void);

/**
 * @brief Advance the simulated peripherals by `ticks` instruction cycles.
 *        Drives Timer0 (and raises TMR0IF + the IRQ callback on overflow).
 * @param ticks the number of instruction cycles to simulate.
 */
void pic18_sim_step(uint32_t ticks);

/**
 * @brief Drive a digital input pin from the test rig (when the pin is
 *        configured as input, TRIS bit = 1).
 *
 * @param port   One of 'A'..'G'.
 * @param pin    Pin number 0..7.
 * @param level  0 = low, 1 = high.
 */
void pic18_sim_drive_input(char port, uint8_t pin, uint8_t level);

/**
 * @brief Read the level currently driven onto an output pin (what an
 *        external load would see). For pins configured as outputs, this
 *        returns the latched value in LATx (DS39609B §10.0). For inputs,
 *        this returns the last value driven via @ref pic18_sim_drive_input.
 * @param port the port letter, 'A'..'G'.
 * @param pin the pin number, 0..7.
 * @return the level on the pin: 0 = low, 1 = high.
 */
uint8_t pic18_sim_read_output(char port, uint8_t pin);

/**
 * @brief Hook a user callback fired whenever the simulated CPU would take
 *        an interrupt. The host harness registers the family dispatcher
 *        here.
 */
typedef void (*pic18_sim_irq_cb_t)(void);

/**
 * @brief Hook a user callback fired whenever the simulated CPU would take
 *        an interrupt. The host harness registers the family dispatcher
 *        here.
 * @param cb the callback to invoke on every simulated interrupt, or NULL
 *        to unregister.
 */
void pic18_sim_set_irq_callback(pic18_sim_irq_cb_t cb);

#endif /* PIC18F6520_SIM_H */
