/* epic-cc dispatch, timer tier: fans out Timer0/RB plus Timer1/2/3,
 * matching the XC8 fan-out with the same per-source gating semantics.
 * Sources outside the slice are not listed in the HAL subset and cannot
 * vector (PIE off). */

#include "core/pic18_irq.h"

/** @brief Timer0 overflow IRQ handler. */
extern void TIMER0_IRQHandler(void);
/** @brief RB<7:4> change IRQ handler. */
extern void RB_IRQHandler(void);
/** @brief Timer1 overflow IRQ handler. */
extern void TIMER1_IRQHandler(void);
/** @brief Timer2 match IRQ handler. */
extern void TIMER2_IRQHandler(void);
/** @brief Timer3 overflow IRQ handler. */
extern void TIMER3_IRQHandler(void);

/**
 * @brief  Dispatch the tier's pending interrupt sources under the same
 *         per-source gating rules as the full fan-out.
 */
void epic_dispatch_all_irqs(void)
{
    uint8_t intcon = epic_sfr_read8(PIC_REG_INTCON);
    if (intcon & PIC_INTCON_TMR0IF) TIMER0_IRQHandler();
    if (intcon & PIC_INTCON_RBIF)   RB_IRQHandler();
    uint8_t pir1 = epic_sfr_read8(PIC_REG_PIR1);
    if (pir1 & PIC_PIR1_TMR1IF)
    {
        if (epic_sfr_read8(PIC_REG_PIE1) & PIC_PIE1_TMR1IE)
        {
            TIMER1_IRQHandler();
        }
        else
        {
            EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_TMR1IF);
        }
    }
    if (pir1 & PIC_PIR1_TMR2IF) TIMER2_IRQHandler();
    uint8_t pir2 = epic_sfr_read8(PIC_REG_PIR2);
    if (pir2 & PIC_PIR2_TMR3IF) TIMER3_IRQHandler();
}
