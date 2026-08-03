/**
 * @file    pic16f193x_isr_vector.c
 * @brief   Real-target interrupt-vector entry (XC8 target build only).
 *
 * @details
 *   Single PIC16F193X vector at 0x0004 (DS41364B §4.0): this file's
 *   `__interrupt()` handler calls the shared @ref pic8_dispatch_all_irqs
 *   fan-out. XC8-only build (the host has no interrupt vector); the host
 *   harness registers the same dispatcher as its sim IRQ callback instead.
 *
 *   The Enhanced Mid-range core saves W/STATUS/BSR/FSR0/FSR1/PCLATH to
 *   shadow registers in hardware on interrupt entry and restores them on
 *   RETFIE (DS41364B §4.1), so no manual push/pop is needed here (unlike
 *   classic PIC16F87XA). No bank-switch scratch bytes are pinned either:
 *   this core's platform header does not use the inline-asm bank-switch
 *   macros that forced the classic family's `__at`-pinned common-RAM
 *   scratch bytes (pic8_irq_pie_scratch / pic8_bank1_scratch).
 */

#include "core/pic16f193x_irq.h"

/* Strong extern prototype instead of including pic8_harness.h, same
 * pattern pic16f193x_irq_dispatch.c uses for the peripheral handlers. */
extern void pic8_dispatch_all_irqs(void);

/**
 * @brief  Single PIC16F193X interrupt-vector handler. Delegates to the
 *         shared dispatcher so the fan-out logic lives in one place.
 */
void __interrupt() PIC16F193X_IRQ_Handler(void)
{
    pic8_dispatch_all_irqs();
}
