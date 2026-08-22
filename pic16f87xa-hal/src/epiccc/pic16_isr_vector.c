/* epic-cc interrupt-vector entry (sibling to
 * src/target/pic16_isr_vector.c); single PIC16 vector at 0x0004
 * (DS39582B §14.11) delegates to the shared epic_dispatch_all_irqs
 * fan-out. No manual STATUS banking: the compiler inserts BANKSELs.
 * No __at-pinned scratch: the epiccc platform header uses plain
 * volatile RMW. */

#include "core/pic16_irq.h"

extern void epic_dispatch_all_irqs(void);

/**
 * @brief Single PIC16 interrupt-vector entry, delegates to
 *        epic_dispatch_all_irqs. Marked with the msp430 interrupt
 *        attribute so irparse tags it isr and isel places it at the
 *        vector.
 */
__attribute__((interrupt(0))) void PIC16_IRQ_Handler(void)
{
    epic_dispatch_all_irqs();
}
