/**
 * Host simulator API for the PIC16F188xx ports, interrupt-on-change,
 * Timer0..Timer2, EUSART, MSSP1, PPS, data EEPROM, NCO1 and the WDT/Sleep
 * helpers. Host-only: never in an XC8 build.
 */
#ifndef PIC16F188XX_SIM_H
#define PIC16F188XX_SIM_H

#include <stdint.h>

typedef void (*pic16f188xx_sim_irq_cb_t)(void);

/** I2C bus trace event codes: the high byte is the kind, the low byte the payload. */
#define PIC16F188XX_SIM_I2C_START    0x0100U  /**< START condition. */
#define PIC16F188XX_SIM_I2C_RESTART  0x0101U  /**< Repeated START. */
#define PIC16F188XX_SIM_I2C_STOP     0x0102U  /**< STOP condition. */
#define PIC16F188XX_SIM_I2C_ADDRESS  0x0200U  /**< Address byte (7-bit address << 1 | R/W). */
#define PIC16F188XX_SIM_I2C_DATA     0x0300U  /**< Data byte the master wrote. */
#define PIC16F188XX_SIM_I2C_READ     0x0400U  /**< Data byte the slave returned. */
#define PIC16F188XX_SIM_I2C_ACK      0x0500U  /**< Master ACK (0) or NACK (1) bit. */

/**
 * @brief Reset all modelled SFRs to power-up values and clear inputs.
 */
void pic16f188xx_sim_reset(void);

/**
 * @brief Sample pins and IOC once per tick, advancing the timers, the
 *        EUSART and MSSP1 each tick.
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

/**
 * @brief Deliver a frame to the EUSART receiver as if it arrived on RX.
 * @param frame Data byte; bit 8 is the ninth bit in 9-bit mode.
 * @return 1 when accepted, 0 when the receiver is off, in overrun, filtered
 *         by address detect, or the two-deep FIFO is full (which also
 *         raises OERR).
 */
uint8_t pic16f188xx_sim_usart_rx_inject(uint16_t frame);

/**
 * @brief Count the frames the EUSART transmitter has shifted out.
 * @return Frames in the TX log (at most 64).
 */
uint16_t pic16f188xx_sim_usart_tx_count(void);

/**
 * @brief Read one shifted-out frame from the TX log.
 * @param index Frame index, 0 is the oldest.
 * @return The frame (bit 8 is TX9D), or 0 for an index past the count.
 */
uint16_t pic16f188xx_sim_usart_tx_frame(uint16_t index);

/**
 * @brief Empty the TX log.
 */
void pic16f188xx_sim_usart_tx_clear(void);

/**
 * @brief Set the byte an SPI transfer shifts in when loopback is off.
 * @param byte MISO byte; the default is 0.
 */
void pic16f188xx_sim_spi_set_miso(uint8_t byte);

/**
 * @brief Tie SDI to SDO: each transfer reads back the byte it sent.
 * @param on Nonzero enables loopback, zero returns to the MISO byte.
 */
void pic16f188xx_sim_spi_set_loopback(uint8_t on);

/**
 * @brief Count the SPI bytes the master has shifted out.
 * @return Bytes in the MOSI log (at most 64).
 */
uint16_t pic16f188xx_sim_spi_count(void);

/**
 * @brief Read one shifted-out SPI byte from the MOSI log.
 * @param index Byte index, 0 is the oldest.
 * @return The byte, or 0 for an index past the count.
 */
uint8_t pic16f188xx_sim_spi_mosi(uint16_t index);

/**
 * @brief Attach a scripted I2C slave to the bus.
 * @param addr7 Seven-bit slave address.
 * @param tx Bytes the slave returns to the master, or NULL for none; the
 *           array must outlive the simulation run.
 * @param tx_len Number of bytes in tx; reads past it return 0xFF.
 */
void pic16f188xx_sim_i2c_set_slave(uint8_t addr7, const uint8_t *tx, uint8_t tx_len);

/**
 * @brief Empty the I2C bus trace.
 */
void pic16f188xx_sim_i2c_clear_trace(void);

/**
 * @brief Count the events in the I2C bus trace.
 * @return Events recorded (at most 64).
 */
uint16_t pic16f188xx_sim_i2c_trace_count(void);

/**
 * @brief Read one event from the I2C bus trace.
 * @param index Event index, 0 is the oldest.
 * @return The event code (PIC16F188XX_SIM_I2C_*), or 0 past the count.
 */
uint16_t pic16f188xx_sim_i2c_trace(uint16_t index);

/**
 * @brief Set how long a data EEPROM write takes in the simulator.
 * @param ticks Ticks from WR set to WR clear. 0 (the default, restored by
 *              reset) completes the write inside the unlock sequence.
 */
void pic16f188xx_sim_eeprom_set_write_ticks(uint32_t ticks);

/**
 * @brief Read the data EEPROM backing store, bypassing NVM.
 * @param addr Byte address 0..255.
 * @return The stored byte; a blank part reads 0xFF.
 */
uint8_t pic16f188xx_sim_eeprom_peek(uint8_t addr);

/**
 * @brief Pin the WDT period the way CONFIG3 WDTCPS does.
 * @param wdtps Period code 0..18 the WDTPS bits take and keep, or -1 (the
 *              default, restored by reset) to leave them writable.
 */
void pic16f188xx_sim_wdt_set_fixed_period(int8_t wdtps);

/**
 * @brief Count the EPIC_WDT_Refresh calls since reset.
 * @return Refresh count.
 */
uint32_t pic16f188xx_sim_wdt_refresh_count(void);

/**
 * @brief Count the EPIC_Sleep_Enter calls since reset, Idle entries included.
 * @return Sleep entry count.
 */
uint32_t pic16f188xx_sim_sleep_count(void);

/**
 * @brief Count the sleeps entered while CPUDOZE<IDLEN> was set since reset.
 * @return Idle entry count.
 */
uint32_t pic16f188xx_sim_idle_count(void);

#endif
