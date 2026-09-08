/* epic-cc interrupt-vector entry: the single PIC16F193X vector at
 * 0x0004 (DS41364B section 4.0) delegates to the shared dispatcher. The
 * Enhanced Mid-range core saves W/STATUS/BSR/FSR0/FSR1/PCLATH to shadow
 * registers in hardware on entry and restores them on RETFIE
 * (DS41364B section 4.1), so no manual push/pop is needed; the XC8
 * target's __interrupt() spelling and its __at()-pinned PIE scratch
 * byte are both replaced here, the latter because epic-cc's banking
 * pass makes the plain-C PIE RMWs correct without inline asm. */

#include "core/pic16f193x_irq.h"

/**
 * @brief Fan-out dispatcher invoked from the interrupt vector; defined
 *        in pic16f193x_irq_dispatch_epiccc.c.
 */
extern void epic_dispatch_all_irqs(void);

/**
 * @brief  Single PIC16F193X interrupt-vector handler, epic-cc spelling
 *         of the target's __interrupt(); vector 0 is 0x0004.
 */
void __attribute__((interrupt(0))) PIC16F193X_IRQ_Handler(void)
{
    epic_dispatch_all_irqs();
}
