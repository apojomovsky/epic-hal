/* epic-cc dispatch tier: Timer0 overflow and Timer2 match to their
 * handlers (the blink smoke and the epic-tick timebase), every other
 * flag dropped so a stale flag cannot re-trigger. The full fan-out
 * (pic16f193x_irq_dispatch.c) strong-references every peripheral
 * handler, which would pull the whole HAL into the whole-program link;
 * tiers grow by adding entries here, the same shape as
 * pic16f87xa-hal's pic16_irq_dispatch_epiccc.c. */

#include "core/pic16f193x_irq.h"

/** @brief TIMER0_IRQHandler, strong in pic16f193x_timer0.c.
 */
extern void TIMER0_IRQHandler(void);

/** @brief TIMER2_IRQHandler, weak in pic16f193x_timer246.c.
 */
extern void TIMER2_IRQHandler(void);

/**
 * @brief Dispatch all pending IRQs, blink plus tick tier.
 */
void epic_dispatch_all_irqs(void)
{
    uint8_t intcon = EPIC_REG8(PIC_REG_INTCON);
    if (intcon & PIC_INTCON_TMR0IF) TIMER0_IRQHandler();

    /* Timer2 match drives the epic-tick timebase. The handler clears
     * TMR2IF itself, so re-read first: writing the stale snapshot back
     * would re-set the just-cleared flag and re-trigger forever. */
    uint8_t pir1 = EPIC_REG8(PIC_REG_PIR1);
    if (pir1 & PIC_PIR1_TMR2IF) TIMER2_IRQHandler();
    /* Everything neither tier enables: clear so a latched flag does
     * not re-trigger on every later ISR. TMR1IF in particular latches
     * at every 65536-cycle wrap even with TMR1IE off. */
    if (EPIC_REG8(PIC_REG_PIR1)) EPIC_REG8(PIC_REG_PIR1) = 0u;
    uint8_t pir2 = EPIC_REG8(PIC_REG_PIR2);
    if (pir2) EPIC_REG8(PIC_REG_PIR2) = 0u;
    uint8_t pir3 = EPIC_REG8(PIC_REG_PIR3);
    if (pir3) EPIC_REG8(PIC_REG_PIR3) = 0u;
}
