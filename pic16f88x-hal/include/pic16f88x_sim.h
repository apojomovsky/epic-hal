/* Public API for the PIC16F88X host simulation backend: drive input
 * pins, read output-pin levels, advance time via pic16f88x_sim_step(),
 * and inject peripheral events. SFRs index the host-side register file
 * (include/host/pic16f88x_platform.h); see src/sim/pic16f88x_sim.c
 * for the peripheral models. */

#ifndef PIC16F88X_SIM_H
#define PIC16F88X_SIM_H

#include <stdint.h>

#include "pic16f88x.h"

/**
 * @brief Initialize the simulation backend and reset every SFR to its
 *        power-on-reset value. Must be called before any HAL call.
 */
void pic16f88x_sim_reset(void);

/**
 * @brief Advance the simulated peripherals by `ticks` instruction cycles.
 *        Drives Timer0, Timer1, Timer2, and the EUSART TXIF re-assert.
 * @param ticks the number of instruction cycles to advance.
 */
void pic16f88x_sim_step(uint32_t ticks);

/**
 * @brief Drive a digital input pin from the test rig (when the pin is
 *        configured as input, the PORTx read reflects this level).
 * @param port the port letter, 'A'..'E'.
 * @param pin the pin number, 0..7.
 * @param level 0 or 1.
 */
void pic16f88x_sim_drive_input(char port, uint8_t pin, uint8_t level);

/**
 * @brief Read the level currently driven onto an output pin (what an
 *        external observer would see on the pin).
 * @param port the port letter, 'A'..'E'.
 * @param pin the pin number, 0..7.
 * @return the level the chip is driving, 0 or 1.
 */
uint8_t pic16f88x_sim_read_output(char port, uint8_t pin);

/**
 * @brief Hook a user callback fired whenever the simulated CPU would take
 *        an interrupt. Lets tests assert "an interrupt should fire here".
 */
typedef void (*pic16f88x_sim_irq_cb_t)(void);

/**
 * @brief Install or remove the interrupt callback.
 * @param cb the callback to fire on a simulated interrupt, or NULL to
 *        unregister.
 */
void pic16f88x_sim_set_irq_callback(pic16f88x_sim_irq_cb_t cb);

/**
 * @brief Inject a byte into the EUSART receiver as if it had just been
 *        received: store it in RCREG and set PIR1<RCIF>.
 * @param data the byte to inject.
 */
void pic16f88x_sim_drive_usart_rx(uint8_t data);

/**
 * @brief Inject a byte into the SSP receiver (SPI slave or I²C target):
 *        store it in SSPBUF, set SSPSTAT<BF> and PIR1<SSPIF>.
 * @param data the byte to inject.
 */
void pic16f88x_sim_drive_ssp_rx(uint8_t data);

/**
 * @brief Drive an A/D conversion to completion with a given 10-bit
 *        result: clear GO/DONE, store the result right-justified in
 *        ADRESH:ADRESL and set PIR1<ADIF>.
 * @param result the 10-bit conversion result, 0..1023.
 */
void pic16f88x_sim_drive_adc_done(uint16_t result);

/**
 * @brief Place a byte in the simulated EEPROM. Subsequent calls to
 *        EPIC_EEPROM_ReadByte return it (the sim models EEPROM reads
 *        against this array).
 * @param addr the EEPROM address, 0..255.
 * @param data the byte to store.
 */
void pic16f88x_sim_drive_eeprom_byte(uint8_t addr, uint8_t data);

/**
 * @brief Simulate a completed EEPROM write at `addr` with `data` and
 *        set PIR2<EEIF>.
 * @param addr the EEPROM address, 0..255.
 * @param data the byte written.
 */
void pic16f88x_sim_drive_eeprom_done(uint8_t addr, uint8_t data);

/**
 * @brief Read a byte from the simulated EEPROM array.
 * @param addr the EEPROM address, 0..255.
 * @return the stored byte.
 */
uint8_t pic16f88x_sim_eeprom_read(uint8_t addr);

/**
 * @brief Simulate a comparator output transition: set/clear C1OUT or
 *        C2OUT and the matching PIR2 flag. Lets host tests exercise the
 *        comparator ISR path without hardware.
 * @param cmp the comparator index, 1 or 2.
 * @param out the new output level, 0 or 1.
 */
void pic16f88x_sim_drive_comparator(uint8_t cmp, uint8_t out);

/**
 * @brief Trigger a simulated oscillator-fail event: set PIR2<OSFIF>.
 *        Lets host tests exercise the OSF ISR path.
 */
void pic16f88x_sim_drive_osc_fail(void);

#endif /* PIC16F88X_SIM_H */
