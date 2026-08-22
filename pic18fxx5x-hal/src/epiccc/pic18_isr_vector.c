/*
 * epic-cc interrupt-vector entry (sibling to
 * src/target/pic18_isr_vector.c). PIC18 has two hardware vectors
 * (0008h high, 0018h low, DS39632E §9.0) but isel-pic18's
 * single-vector compatibility mode (docs/31 D-8) supports at most one
 * ISR. One high-priority entry that dispatches the full fan-out is the
 * correct single-vector shape; the low vector is not emitted.
 */

extern void epic_dispatch_all_irqs(void);

/**
 * @brief High-priority interrupt vector (0008h). Delegates to the shared
 *        dispatcher. Marked isr so isel places it at the vector.
 */
__attribute__((interrupt(0))) void PIC18_IRQ_HandlerHigh(void)
{
    epic_dispatch_all_irqs();
}
