/* Ultra Low-Power Wake-up (ULPWU) driver. Source: DS40001291H §3.2.2.
 * ULPWU lets RA0/AN0/ULPWU/C12IN0- detect a slow falling voltage
 * (typically a capacitor discharging through the internal 200 nA sink)
 * and generate a wake-up interrupt, without an external comparator.
 * The sequence is: charge RA0 by driving it high, set ULPWUE
 * (PCON<5>) to start the discharge, arm ULPWUIE (PIE2<2>), then SLEEP;
 * the interrupt fires when RA0 drops below VIL. */

#ifndef PIC16F88X_ULPWU_H
#define PIC16F88X_ULPWU_H

#include "pic16f88x.h"
#include "pic16f88x_sfr.h"

/**
 * @brief  Initialize the ULPWU module: clear the wake-up flag and arm
 *         the interrupt if a callback is given.
 * @param callback optional wake-up callback (fires on ULPWUIF), or
 *        NULL for polling mode.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_ULPWU_Init(void (*callback)(void));

/**
 * @brief  De-initialize the ULPWU module: disable the interrupt, clear
 *         the flag and stop the discharge.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_ULPWU_DeInit(void);

/**
 * @brief  Start the ULPWU discharge (set PCON<ULPWUE>). Call after
 *         charging RA0 high and switching it to input.
 */
void EPIC_ULPWU_Start(void);

/**
 * @brief  Stop the ULPWU discharge (clear PCON<ULPWUE>).
 */
void EPIC_ULPWU_Stop(void);

/**
 * @brief  Returns 1 if a wake-up condition has occurred (PIR2<ULPWUIF>).
 * @return 1 if the wake-up flag is set, 0 otherwise.
 */
uint8_t EPIC_ULPWU_IsWakeup(void);

/**
 * @brief Clear the ULPWU wake-up flag.
 */
void EPIC_ULPWU_ClearFlag(void);

/**
 * @brief Weak ULPWU ISR, override in user code.
 */
void ULPWU_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F88X_ULPWU_H */
