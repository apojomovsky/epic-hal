/**
 * Private seam between the port/Timer0..Timer2 model (pic16f188xx_sim.c),
 * the Timer4 model (pic16f188xx_sim_timer4.c), the EUSART, MSSP and
 * PPS-lock model (pic16f188xx_sim_serial.c), the EEPROM, NCO and WDT/Sleep
 * model (pic16f188xx_sim_sys.c) and the CCP and PWM model
 * (pic16f188xx_sim_ccp.c). Not part of the sim API a probe sees.
 */
#ifndef PIC16F188XX_SIM_PRIV_H
#define PIC16F188XX_SIM_PRIV_H

#include <stdint.h>

/**
 * PWM time base a timer hands to the CCP and PWM model each tick.
 */
typedef struct {
    uint8_t wrap;   /**< Non-zero on the tick TMRx matched PRx and cleared. */
    uint8_t live;   /**< Non-zero while TMRx is on and clocked from FOSC/4. */
    uint16_t pos;   /**< TMRx x 4 plus the prescaler fraction (the 10-bit time base). */
} pic16f188xx_sim_pwm_base_t;

/**
 * @brief Clear the Timer4 state and load the POR value of PR4.
 */
void pic16f188xx_sim_timer4_reset(void);

/**
 * @brief Advance Timer4 by one instruction tick.
 * @details Sets PIR4.TMR4IF on the postscaled period match. Call once per
 * tick before pic16f188xx_sim_timer4_base and the CCP step.
 */
void pic16f188xx_sim_timer4_step(void);

/**
 * @brief Timer4 view for the CCP and PWM model after this tick's step.
 * @param base Out: the PWM time base of Timer4.
 */
void pic16f188xx_sim_timer4_base(pic16f188xx_sim_pwm_base_t *base);

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

/**
 * @brief Clear the CCP and PWM state and load the POR values of its SFRs.
 */
void pic16f188xx_sim_ccp_reset(void);

/**
 * @brief Advance every CCP and PWM module by one tick, after the timers.
 * @param base PWM time bases of this tick: [0] Timer2 (couple value 01),
 *        [1] Timer4 (couple value 10).
 */
void pic16f188xx_sim_ccp_step(const pic16f188xx_sim_pwm_base_t base[2]);

/**
 * @brief Apply a write to a CCPxCON or PWMxCON register.
 * @details OUT is read-only and keeps its hardware value; a CCPxCON write
 * that changes EN or MODE resets the module's output latch and capture
 * prescaler at once.
 * @param addr SFR address written.
 * @param value Value written.
 * @return 1 when the address is a CCP or PWM control register and the
 *         write was applied, 0 for any other address (a plain store).
 */
uint8_t pic16f188xx_sim_ccp_write(uint16_t addr, uint8_t value);

#endif
