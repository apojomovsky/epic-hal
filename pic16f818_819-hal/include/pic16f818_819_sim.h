/* Public API for the PIC16F818/819 host simulation backend: drive input
 * pins, read output-pin levels, advance time via
 * pic16f818_819_sim_step(), and inject peripheral events. SFRs index the
 * host-side register file (include/host/pic16f818_819_platform.h); see
 * src/sim/pic16f818_819_sim.c for the peripheral models. */

#ifndef PIC16F818_819_SIM_H
#define PIC16F818_819_SIM_H

#include <stdint.h>

#include "pic16f818_819_hal.h"

/* The 512-byte memory-backed register file the host platform header
 * indexes, defined in src/sim/pic16f818_819_sim.c. */
extern uint8_t pic16f818_819_sim_sfr[0x200];

/**
 * @brief Initialize the simulation backend and reset every SFR to its
 *        power-on-reset value. Must be called before any HAL call.
 */
void pic16f818_819_sim_reset(void);

/**
 * @brief Advance the simulated peripherals by `ticks` instruction
 *        cycles. Drives Timer0, Timer1, Timer2 and the CCP1 compare
 *        model.
 * @param ticks the number of instruction cycles to advance.
 */
void pic16f818_819_sim_step(uint32_t ticks);

/**
 * @brief Drive a digital input pin from the test rig (when the pin is
 *        configured as input, the PORTx read reflects this level).
 * @param port the port letter, 'A' or 'B'.
 * @param pin the pin number, 0..7.
 * @param level 0 = low, 1 = high.
 */
void pic16f818_819_sim_drive_input(char port, uint8_t pin, uint8_t level);

/**
 * @brief Read the level currently driven onto an output pin: the PORTx
 *        latch bit for a pin configured as output, the last driven level
 *        for an input one.
 * @param port the port letter, 'A' or 'B'.
 * @param pin the pin number, 0..7.
 * @return the pin level, 0 or 1.
 */
uint8_t pic16f818_819_sim_read_output(char port, uint8_t pin);

/**
 * @brief Hook a user callback fired whenever the simulated CPU would
 *        take an interrupt. Lets tests assert "an interrupt should fire
 *        here".
 */
typedef void (*pic16f818_819_sim_irq_cb_t)(void);

/**
 * @brief Install or remove the interrupt callback.
 * @param cb the callback to fire on a simulated interrupt, or NULL to
 *        unregister.
 */
void pic16f818_819_sim_set_irq_callback(pic16f818_819_sim_irq_cb_t cb);

/**
 * @brief Inject a byte into the SSP receiver (SPI slave or I2C target).
 *        Stores the byte in SSPBUF and sets SSPSTAT<BF> and
 *        PIR1<SSPIF>. The next call to EPIC_SSP_ReadByte() returns it.
 * @param data the byte to inject.
 */
void pic16f818_819_sim_drive_ssp_rx(uint8_t data);

/**
 * @brief Drive an A/D conversion to completion with a given 10-bit
 *        result. Writes ADRESH:ADRESL in the justification ADCON1<ADFM>
 *        selects, clears GO/DONE, and sets PIR1<ADIF>. The next call to
 *        EPIC_ADC_Read() returns the result.
 * @param result the 10-bit conversion result, 0..1023.
 */
void pic16f818_819_sim_drive_adc_done(uint16_t result);

/**
 * @brief Place a byte in the simulated EEPROM. Subsequent calls to
 *        EPIC_EEPROM_ReadByte(addr) return it.
 * @param addr the EEPROM address, 0..255.
 * @param data the byte to store.
 */
void pic16f818_819_sim_drive_eeprom_byte(uint8_t addr, uint8_t data);

/**
 * @brief Simulate a completed EEPROM write at `addr` with `data` and
 *        set PIR2<EEIF>.
 * @param addr the EEPROM address that was written.
 * @param data the byte that was stored.
 */
void pic16f818_819_sim_drive_eeprom_done(uint8_t addr, uint8_t data);

/**
 * @brief Read a byte from the simulated EEPROM array (the shared host
 *        driver's read path).
 * @param addr the EEPROM address, 0..255.
 * @return the stored byte.
 */
uint8_t pic14_sim_eeprom_read(uint8_t addr);

#endif /* PIC16F818_819_SIM_H */
