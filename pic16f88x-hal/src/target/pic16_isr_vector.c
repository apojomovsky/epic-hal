/* Real-target interrupt-vector entry (XC8 build only): the single
 * PIC16 vector at 0x0004 (DS40001291H §14.11) delegates to the shared
 * epic_dispatch_all_irqs fan-out. The host registers the same
 * dispatcher as its sim IRQ callback instead. */

#include "core/pic16_irq.h"

/* Definitions for target/pic16f88x_platform.h's `extern volatile
 * uint8_t epic_irq_pie_scratch`/`epic_bank1_scratch` (see that header's
 * comments for what they're for); `__at`-pinned into PIC16 mid-range's
 * bank-independent common RAM. */
volatile uint8_t epic_irq_pie_scratch __at(0x70);
volatile uint8_t epic_bank1_scratch __at(0x71);

/**
 * @brief Fan-out dispatcher invoked from the interrupt vector; defined
 *        in pic16_irq_dispatch.c.
 *
 * Strong extern prototype instead of including epic_harness.h, same
 * pattern pic16_irq_dispatch.c uses for the peripheral handlers.
 */
extern void epic_dispatch_all_irqs(void);

/* Single PIC16 vector handler; delegates to the shared dispatcher so
 * the fan-out logic lives in one place. The preempted main line can be
 * inside a bank-macro window (RP1:RP0 != 0) when this runs: XC8 v4.00
 * emits no banksel for the dispatch's PIR reads, so the whole ISR path
 * must run in bank 0. The vector's own prologue saves and restores
 * STATUS, so the preempted context resumes its window unchanged. */

/**
 * @brief Single PIC16 interrupt-vector entry, delegates to
 *        @ref epic_dispatch_all_irqs.
 */
void __interrupt() PIC16_IRQ_Handler(void)
{
    asm("bcf STATUS,6");
    asm("bcf STATUS,5");
    epic_dispatch_all_irqs();
}
