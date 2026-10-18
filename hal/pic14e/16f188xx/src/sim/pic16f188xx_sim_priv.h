/**
 * Private seam between the port/Timer0 model (pic16f188xx_sim.c), the
 * EUSART, MSSP and PPS-lock model (pic16f188xx_sim_serial.c) and the
 * EEPROM, NCO and WDT/Sleep model (pic16f188xx_sim_sys.c). Not part of
 * the sim API a probe sees.
 */
#ifndef PIC16F188XX_SIM_PRIV_H
#define PIC16F188XX_SIM_PRIV_H

#include <stdint.h>

/**
 * @brief Clear the serial peripheral state and load the POR values of its SFRs.
 */
void pic16f188xx_sim_serial_reset(void);

/**
 * @brief Advance the EUSART transmitter and the MSSP engine by one tick.
 */
void pic16f188xx_sim_serial_step(void);

/**
 * @brief Load the POR values of the NVM, NCO, WDT and power SFRs and clear
 *        the model state (the EEPROM erases to 0xFF).
 */
void pic16f188xx_sim_sys_reset(void);

/**
 * @brief Advance the EEPROM write countdown and NCO1 by one tick.
 */
void pic16f188xx_sim_sys_step(void);

/**
 * @brief Apply the write side effects of the NVM and NCO registers.
 * @param addr SFR address.
 * @param value Value written.
 * @return 1 when handled and the cell updated, 0 for a plain store.
 */
uint8_t pic16f188xx_sim_sys_write(uint16_t addr, uint8_t value);

/**
 * @brief Count one EPIC_WDT_Refresh call.
 */
void pic16f188xx_sim_wdt_note_refresh(void);

/**
 * @brief Count one EPIC_Sleep_Enter call (and an Idle entry when IDLEN is set).
 */
void pic16f188xx_sim_sleep_note(void);

#endif
