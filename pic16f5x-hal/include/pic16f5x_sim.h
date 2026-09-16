/* Public API for the PIC16F5x host simulation backend: drive input
 * pins, read output-pin levels, advance time via
 * pic16f5x_sim_step(), and observe the control shadow registers. SFRs
 * index the host-side register file (include/host/pic16f5x_platform.h);
 * see src/sim/pic16f5x_sim.c for the peripheral models. */

#ifndef PIC16F5X_SIM_H
#define PIC16F5X_SIM_H

#include <stdint.h>
#include <stdbool.h>
#include "pic16f5x_hal.h"

/**
 * @brief Initialize the simulation backend and reset every SFR to its
 *        power-on-reset value. Must be called before any HAL call.
 */
void pic16f5x_sim_reset(void);

/**
 * @brief Advance the simulated peripherals by `ticks` instruction
 *        cycles. Drives Timer0.
 * @param ticks the number of instruction cycles to advance.
 */
void pic16f5x_sim_step(uint32_t ticks);

/**
 * @brief Drive a digital input pin from the test rig (when the pin is
 *        an input).
 * @param port the port letter, 'A' or 'B'.
 * @param pin the pin number, 0..7.
 * @param level 0 = low, 1 = high.
 */
void pic16f5x_sim_drive_input(char port, uint8_t pin, uint8_t level);

/**
 * @brief Read the level currently driven onto an output pin (what an
 *        LED or scope would see).
 * @param port the port letter, 'A' or 'B'.
 * @param pin the pin number, 0..7.
 * @return the pin level, 0 or 1.
 */
uint8_t pic16f5x_sim_read_output(char port, uint8_t pin);

/* The control shadow registers (the host platform header writes them
 * through EPIC_TRIS_WRITE / EPIC_OPTION_WRITE); exposed read-only so
 * tests can assert the direction/prescaler the driver programmed.
 * Reset to 0xFF (all inputs, option POR value). */
extern uint8_t pic16f5x_sim_trisa;
extern uint8_t pic16f5x_sim_trisb;
extern uint8_t pic16f5x_sim_option;

#endif /* PIC16F5X_SIM_H */
