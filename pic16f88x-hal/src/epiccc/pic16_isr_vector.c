/* epic-cc interrupt-vector entry: the single PIC16 vector at 0x0004
 * (DS40001291H 14.11) delegates to the shared dispatcher. XC8 uses
 * its interrupt attribute plus bank-normalizing asm; epic-cc uses
 * the interrupt attribute and relies on its banking pass. */

#include "core/pic16_irq.h"

/**
 * @brief Fan-out dispatcher invoked from the interrupt vector; defined
 *        in pic16_irq_dispatch.c.
 */
extern void epic_dispatch_all_irqs(void);

/**
 * @brief Single PIC16 interrupt-vector entry, delegates to
 *        @ref epic_dispatch_all_irqs.
 */
void __attribute__((interrupt(0))) PIC16_IRQ_Handler(void)
{
    epic_dispatch_all_irqs();
}
