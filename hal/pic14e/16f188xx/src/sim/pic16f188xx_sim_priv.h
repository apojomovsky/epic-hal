/**
 * Private seam between the port/Timer0 model (pic16f188xx_sim.c) and the
 * EUSART, MSSP and PPS-lock model (pic16f188xx_sim_serial.c). Not part of
 * the sim API a probe sees.
 */
#ifndef PIC16F188XX_SIM_PRIV_H
#define PIC16F188XX_SIM_PRIV_H

/**
 * @brief Clear the serial peripheral state and load the POR values of its SFRs.
 */
void pic16f188xx_sim_serial_reset(void);

/**
 * @brief Advance the EUSART transmitter and the MSSP engine by one tick.
 */
void pic16f188xx_sim_serial_step(void);

#endif
