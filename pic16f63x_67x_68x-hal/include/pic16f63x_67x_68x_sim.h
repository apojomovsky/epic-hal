/* Public API for the PIC16F63x/67x/68x host simulation backend: drive
 * input pins, read output-pin levels, advance time via
 * pic16f63x_67x_68x_sim_step(), and inject peripheral events. SFRs
 * index the host-side register file
 * (include/host/pic16f63x_67x_68x_platform.h); see
 * src/sim/pic16f63x_67x_68x_sim.c for the peripheral models. */

#ifndef PIC16F63X_67X_68X_SIM_H
#define PIC16F63X_67X_68X_SIM_H

#include <stdint.h>
#include <stdbool.h>
#include "pic16f63x_67x_68x_hal.h"

/**
 * @brief Initialize the simulation backend and reset every SFR to its
 *        power-on-reset value. Must be called before any HAL call.
 */
void pic16f63x_67x_68x_sim_reset(void);

/**
 * @brief Advance the simulated peripherals by `ticks` instruction cycles.
 *        Drives Timer0 and Timer1.
 * @param ticks the number of instruction cycles to advance.
 */
void pic16f63x_67x_68x_sim_step(uint32_t ticks);

/**
 * @brief Drive a digital input pin from the test rig (when the pin is
 *        an input). PORTA is RA0..RA5, PORTB is RB4..RB7, PORTC is
 *        RC0..RC7.
 * @param port the port letter, 'A', 'B' or 'C'.
 * @param pin the pin number, 0..7.
 * @param level 0 = low, 1 = high.
 */
void pic16f63x_67x_68x_sim_drive_input(char port, uint8_t pin, uint8_t level);

/**
 * @brief Read the level currently driven onto an output pin (what an
 *        LED or scope would see).
 * @param port the port letter, 'A', 'B' or 'C'.
 * @param pin the pin number, 0..7.
 * @return the pin level, 0 or 1.
 */
uint8_t pic16f63x_67x_68x_sim_read_output(char port, uint8_t pin);

/**
 * @brief Hook a user callback fired whenever the simulated CPU would take
 *        an interrupt. Lets tests assert "an interrupt should fire here".
 */
typedef void (*pic16f63x_67x_68x_sim_irq_cb_t)(void);

/**
 * @brief Install or remove the interrupt callback.
 * @param cb the callback to fire on a simulated interrupt, or NULL to
 *        unregister.
 */
void pic16f63x_67x_68x_sim_set_irq_callback(pic16f63x_67x_68x_sim_irq_cb_t cb);

/**
 * @brief Drive a comparator output: set CxOUT and raise the matching
 *        change flag as if the analog input had crossed the reference.
 * @param comp 1 for C1, 2 for C2.
 * @param level 0 = output low, 1 = output high.
 */
void pic16f63x_67x_68x_sim_drive_comparator(uint8_t comp, uint8_t level);

/**
 * @brief Place a byte in the simulated EEPROM. Subsequent reads of the
 *        same address return it. The table holds 256 bytes (the 631
 *        implements 128, the 677 all 256).
 * @param addr the EEPROM address, 0..127.
 * @param data the byte to store.
 */
void pic16f63x_67x_68x_sim_drive_eeprom_byte(uint8_t addr, uint8_t data);

/**
 * @brief Simulate a completed EEPROM write at `addr` with `data` and
 *        set PIR2<EEIF>.
 * @param addr the EEPROM address that was written.
 * @param data the byte that was stored.
 */
void pic16f63x_67x_68x_sim_drive_eeprom_done(uint8_t addr, uint8_t data);

#endif /* PIC16F63X_67X_68X_SIM_H */
