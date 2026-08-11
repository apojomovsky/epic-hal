/*
 * Real-target interrupt-vector entries (XC8 target build only). Both
 * PIC18 vectors (0008h high, 0018h low, DS39632E §9.0) delegate to the
 * shared fan-out @ref epic_dispatch_all_irqs; each peripheral IRQHandler
 * checks its own flag, so calling the full dispatch from both is correct.
 * The host build registers the same dispatcher as its sim IRQ callback.
 */

/**
 * @brief  Shared IRQ fan-out dispatcher, declared here as a strong extern
 *         instead of via epic_harness.h, to keep the harness's unused
 *         inline (epic_harness_report) out of this unit's warning
 *         surface; same pattern as pic16_isr_vector.c.
 */
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
