/* Public API for the PIC16F628A host simulation backend: drive input
 * pins, read output-pin levels, advance time via pic16f628a_sim_step(),
 * and inject peripheral events. SFRs index the host-side register file
 * (include/host/pic16f628a_platform.h); see src/sim/pic16f628a_sim.c
 * for the peripheral models. */

#ifndef PIC16F628A_SIM_H
#define PIC16F628A_SIM_H

#include <stdint.h>
#include <stdbool.h>
#include "pic16f628a.h"

/**
 * @brief Initialize the simulation backend and reset every SFR to its
 *        power-on-reset value. Must be called before any HAL call.
 */
void pic16f628a_sim_reset(void);

/**
 * @brief Advance the simulated peripherals by `ticks` instruction cycles.
 *        Drives Timer0/1/2 and USART.
 * @param ticks the number of instruction cycles to advance.
 */
void pic16f628a_sim_step(uint32_t ticks);

/**
 * @brief Drive a digital input pin from the test rig (when the pin is
 *        an input). Only PORTA and PORTB exist on this part.
 * @param port the port letter, 'A' or 'B'.
 * @param pin the pin number, 0..7.
 * @param level 0 = low, 1 = high.
 */
void pic16f628a_sim_drive_input(char port, uint8_t pin, uint8_t level);

/**
 * @brief Read the level currently driven onto an output pin (what an
 *        LED or scope would see).
 * @param port the port letter, 'A' or 'B'.
 * @param pin the pin number, 0..7.
 * @return the pin level, 0 or 1.
 */
uint8_t pic16f628a_sim_read_output(char port, uint8_t pin);

/**
 * @brief Hook a user callback fired whenever the simulated CPU would take
 *        an interrupt. Lets tests assert "an interrupt should fire here".
 */
typedef void (*pic16f628a_sim_irq_cb_t)(void);

/**
 * @brief Install or remove the interrupt callback.
 * @param cb the callback to fire on a simulated interrupt, or NULL to
 *        unregister.
 */
void pic16f628a_sim_set_irq_callback(pic16f628a_sim_irq_cb_t cb);

/**
 * @brief Inject a byte into the USART receiver as if it had just been
 *        received.
 * @param data the byte to inject.
 */
void pic16f628a_sim_drive_usart_rx(uint8_t data);

/**
 * @brief Place a byte in the simulated EEPROM. Subsequent reads of the
 *        same address return it. The part has 128 bytes; the upper half
 *        of the 256-byte table is ignored.
 * @param addr the EEPROM address, 0..127.
 * @param data the byte to store.
 */
void pic16f628a_sim_drive_eeprom_byte(uint8_t addr, uint8_t data);

/**
 * @brief Simulate a completed EEPROM write at `addr` with `data` and
 *        set PIR1<EEIF>.
 * @param addr the EEPROM address that was written.
 * @param data the byte that was stored.
 */
void pic16f628a_sim_drive_eeprom_done(uint8_t addr, uint8_t data);

#endif /* PIC16F628A_SIM_H */
