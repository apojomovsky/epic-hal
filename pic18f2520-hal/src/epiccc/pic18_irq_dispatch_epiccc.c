/* epic-cc dispatch, GPIO + Timer0 foundation tier: nothing to vector
 * beyond Timer0 overflow and the RB<7:4> change, matching the XC8
 * foundation fan-out with the same per-source gating semantics. Sources
 * outside the slice are not listed in the HAL subset and cannot vector
 * (PIE off). */

#include "core/pic18_irq.h"

/** @brief Timer0 overflow IRQ handler. */
extern void TIMER0_IRQHandler(void);
/** @brief RB<7:4> change IRQ handler. */
extern void RB_IRQHandler(void);

/**
 * @brief  Dispatch the tier's pending interrupt sources under the same
 *         per-source gating rules as the full fan-out.
 */
void epic_dispatch_all_irqs(void)
{
    uint8_t intcon = epic_sfr_read8(PIC_REG_INTCON);
    if (intcon & PIC_INTCON_TMR0IF) TIMER0_IRQHandler();
    if (intcon & PIC_INTCON_RBIF)   RB_IRQHandler();
}
