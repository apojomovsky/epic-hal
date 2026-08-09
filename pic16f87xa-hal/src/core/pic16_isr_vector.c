/**
 * @file    pic16_isr_vector.c
 * @brief   Real-target interrupt-vector entry (XC8 target build only).
 *
 * @details
 *   Single PIC16 vector at 0x0004 (DS39582B §14.11): this file's
 *   `__interrupt()` handler calls the shared @ref epic_dispatch_all_irqs
 *   fan-out. XC8-only build (the host has no interrupt vector); the
 *   host harness registers the same dispatcher as its sim IRQ callback
 *   instead.
 */

#include "core/pic16_irq.h"

/* Definitions for target/pic16f87xa_platform.h's `extern volatile
 * uint8_t epic_irq_pie_scratch`/`epic_bank1_scratch` (see that header's
 * comments for what they're for); `__at`-pinned into PIC16 mid-range's
 * bank-independent common RAM. */
volatile uint8_t epic_irq_pie_scratch __at(0x70);
volatile uint8_t epic_bank1_scratch __at(0x71);

/* Strong extern prototype instead of including epic_harness.h, same
 * pattern pic16_irq_dispatch.c uses for the peripheral handlers. */
extern void epic_dispatch_all_irqs(void);

/**
 * @brief  Single PIC16 interrupt-vector handler. Delegates to the shared
 *         dispatcher so the fan-out logic lives in one place.
 *
 *         The preempted main line can be inside a bank-macro window
 *         (RP1:RP0 != 00) when this runs. XC8 v4.00 emits no banksel
 *         for the dispatch's PIR1/PIR2 reads and relies on RP=0 for
 *         the handlers' static accesses, so the whole ISR path must
 *         run in bank 0. Confirmed by the combination-matrix C1 gate
 *         (epic-combo-uart-ssp, 2026-08-09): without this, an
 *         interrupt taken inside an EEPROM/SSP bank window froze the
 *         timer callback count (the increments landed in the wrong
 *         bank's GPR). The vector's own prologue saves and restores
 *         STATUS, so the preempted context resumes its window
 *         unchanged.
 */
void __interrupt() PIC16_IRQ_Handler(void)
{
    asm("bcf STATUS,6");
    asm("bcf STATUS,5");
    epic_dispatch_all_irqs();
}
