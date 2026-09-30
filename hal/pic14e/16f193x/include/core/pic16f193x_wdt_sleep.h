/**
 * CPU-level helpers: Watchdog Timer, Brown-out Reset, Sleep
 * (DS41364B §3.0 PCON, §24.0 WDT/WDTCON, §24.2 Sleep). The watchdog is
 * enabled by the WDTEN config bit, or per-window via SWDTEN in WDTCON
 * (§24.1); either way call EPIC_WDT_Refresh periodically or the chip
 * resets. Config bits themselves are emitted by the mcu Makefile, not
 * here. EPIC_Sleep_Enter is a no-op on host (no real CPU to stop).
 */

#ifndef PIC16F193X_WDT_SLEEP_H
#define PIC16F193X_WDT_SLEEP_H

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"

/**
 * @brief  Refresh the Watchdog Timer (`clrwdt`, no-op on host). Must be
 *         called more often than the WDT period on a real target.
 */
void EPIC_WDT_Refresh(void);

/**
 * @brief  Enter Sleep (`sleep` asm on target, no-op on host; callers
 *         should keep driving pic16f193x_sim_step()). Real target halts
 *         until any enabled interrupt wakes it (DS41364B §24.2).
 */
void EPIC_Sleep_Enter(void);

/**
 * @brief  Returns 1 if the last reset was a Brown-out Reset
 *         (PCON<BOR>, DS41364B §3.0). Clear after reading via
 *         @ref EPIC_BOR_ClearFlag.
 * @return 1 if the last reset was a Brown-out Reset, 0 otherwise
 */
uint8_t EPIC_BOR_GetStatus(void);

/**
 * @brief Clear PCON<BOR>.
 */
void EPIC_BOR_ClearFlag(void);

/**
 * @brief  Returns 1 if the device just powered on (PCON<POR>,
 *         DS41364B §3.0). Set only on Power-on Reset.
 * @return 1 if a Power-on Reset just occurred, 0 otherwise
 */
uint8_t EPIC_POR_GetStatus(void);

/**
 * @brief Clear PCON<POR>.
 */
void EPIC_POR_ClearFlag(void);

#endif /* PIC16F193X_WDT_SLEEP_H */
