/*
 * Public API for the PIC18F1320 host simulation backend: drive input
 * pins, read output levels, advance simulated peripherals, and
 * register an interrupt callback. Foundation phase only: Timer0 and
 * GPIOA/GPIOB.
 */

#ifndef PIC18F1320_SIM_H
#define PIC18F1320_SIM_H

#include <stdint.h>
#include "pic18f1320.h"

/**
 * @brief Initialize the simulation backend and reset every SFR to its
 *        power-on-reset value (DS39605F Table 5-1). Must be called before
 *        any HAL call.
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
 * @param port   'A' or 'B'.
 * @param pin    Pin number 0..7.
 * @param level  0 = low, 1 = high.
 */
void pic18_sim_drive_input(char port, uint8_t pin, uint8_t level);

/**
 * @brief Read the level currently driven onto an output pin (what an
 *        external load would see). For pins configured as outputs, this
 *        returns the latched value in LATx (DS39605F §5.0/§6.0). For
 *        inputs, this returns the last value driven via @ref
 *        pic18_sim_drive_input.
 * @param port the port letter, 'A' or 'B'.
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

/**
 * @brief Deliver a received EUSART byte to the simulated hardware. Places
 *        the byte in RCREG, sets PIR1<RCIF>, and raises the IRQ callback.
 * @param data the byte received on the USART.
 */
void pic18_sim_drive_usart_rx(uint8_t data);

#endif /* PIC18F1320_SIM_H */
