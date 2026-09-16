/* CPU-level helpers: Watchdog Timer and Sleep (DS41213D §8.0 WDT,
 * §6.0/7.0 reset). The 5x core has NO PCON register (no BOR/POR flags,
 * verified absent from every part's EDC), so the BOR/POR helpers of the
 * 14-bit families do not exist here. WDT refresh is the native
 * `clrwdt` instruction on target, a no-op on host; Sleep is the native
 * `sleep` instruction on target (wakes only on reset or a WDT
 * time-out: there are no interrupts on this core), a no-op on host. */

#ifndef PIC16F5X_WDT_SLEEP_H
#define PIC16F5X_WDT_SLEEP_H

#include "pic16f5x_hal.h"
#include "pic16f5x_sfr.h"

/**
 * @brief  Refresh the Watchdog Timer (`clrwdt`, no-op on host). Must be
 *         called more often than the WDT period on a real target
 *         (DS41213D §8.0: typ. 18 ms x prescale ratio when enabled by
 *         the WDT config bit).
 */
void EPIC_WDT_Refresh(void);

/**
 * @brief  Enter Sleep (`sleep` asm on target, no-op on host). A real
 *         target wakes only on MCLR reset or a WDT time-out: there is
 *         no interrupt path on this core (DS41213D §7.0, §8.0).
 */
void EPIC_Sleep_Enter(void);

#endif /* PIC16F5X_WDT_SLEEP_H */
