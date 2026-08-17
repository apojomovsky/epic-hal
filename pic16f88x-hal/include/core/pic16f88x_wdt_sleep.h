/* CPU-level helpers: Watchdog Timer, Brown-out Reset, Sleep. Source:
 * DS40001291H §14.2 (PCON), §14.5 (WDT), §14.6 (Sleep). Config bits
 * are left to the user's MPLAB X/XC8 setup; EPIC_Sleep_Enter is a
 * no-op on host. */

#ifndef PIC16F88X_WDT_SLEEP_H
#define PIC16F88X_WDT_SLEEP_H

#include "pic16f88x.h"
#include "pic16f88x_sfr.h"

/**
 * @brief  Refresh the Watchdog Timer (`clrwdt`, no-op on host). Must be
 *         called more often than the WDT period on a real target.
 */
void EPIC_WDT_Refresh(void);

/**
 * @brief  Enter Sleep (`sleep` asm on target, no-op on host; callers
 *         should keep driving pic16f88x_sim_step()). Real target
 *         halts until any enabled interrupt wakes it (§14.6).
 */
void EPIC_Sleep_Enter(void);

/**
 * @brief  Returns 1 if the last reset was a Brown-out Reset
 *         (PCON<BOR>).  Clear after reading via @ref EPIC_BOR_ClearFlag.
 * @return 1 if the last reset was a Brown-out Reset, 0 otherwise.
 */
uint8_t EPIC_BOR_GetStatus(void);

/**
 * @brief Clear PCON<BOR>. The POR bit is write-1-to-clear.
 */
void EPIC_BOR_ClearFlag(void);

/**
 * @brief  Returns 1 if the device just powered on (PCON<POR>).
 *         Set only on Power-on Reset.
 * @return 1 if the last reset was a Power-on Reset, 0 otherwise.
 */
uint8_t EPIC_POR_GetStatus(void);

/**
 * @brief Clear PCON<POR>.
 */
void EPIC_POR_ClearFlag(void);

/**
 * @brief  Enable or disable the software Watchdog Timer (WDTCON<SWDTEN>).
 *         Only effective when the WDTE configuration bit is 0
 *         (DS40001291H §14.5, Register 14-3).
 * @param enable 1 to turn the WDT on, 0 to turn it off.
 */
void EPIC_WDT_SetSoftwareEnable(uint8_t enable);

/**
 * @brief  Set the WDT period prescaler (WDTCON<WDTPS3:WDTPS0>,
 *         DS40001291H Register 14-3). The WDT time base is the
 *         31 kHz LFINTOSC; the nominal time-out is 4 ms times the
 *         prescale ratio (1:32..1:65536, 1 ms..268 s).
 * @param wdtps the 4-bit WDTPS value (0..11; 12..15 reserved).
 */
void EPIC_WDT_SetPrescaler(uint8_t wdtps);

#endif /* PIC16F88X_WDT_SLEEP_H */
