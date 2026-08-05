/**
 * @file    pic18_isr_vector.c
 * @brief   Real-target interrupt-vector entries (XC8 target build only).
 *
 * @details
 *   Both PIC18 vectors (0008h high, 0018h low, DS39632E §9.0) delegate to
 *   the shared fan-out @ref epic_dispatch_all_irqs; each peripheral
 *   IRQHandler checks its own flag, so calling the full dispatch from
 *   both is correct. Built only by the XC8 Makefile; the host build
 *   registers the same dispatcher as its sim IRQ callback instead.
 */

/* Declared in epic_harness.h (shared). Declared here as a strong extern
 * prototype instead of including that header, to keep the harness's
 * unused inline (epic_harness_report) out of this translation unit's
 * warning surface, the same pattern pic16_isr_vector.c uses. */
extern void epic_dispatch_all_irqs(void);

/**
 * @brief  High-priority interrupt vector (0008h). Delegates to the shared
 *         dispatcher.
 */
void __interrupt(high_priority) PIC18_IRQ_HandlerHigh(void)
{
    epic_dispatch_all_irqs();
}

/**
 * @brief  Low-priority interrupt vector (0018h). Delegates to the shared
 *         dispatcher.
 */
void __interrupt(low_priority) PIC18_IRQ_HandlerLow(void)
{
    epic_dispatch_all_irqs();
}
