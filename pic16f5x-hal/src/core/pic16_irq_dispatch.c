/* No-op fan-out dispatcher (epic-common contract): called by the
 * harness on both builds, and from a real target's interrupt vector on
 * families that have one. The 5x core has no interrupt vector at all
 * (DS41213D §4.0), so there is nothing to fan out: this exists so the
 * harness and family-agnostic consumers link unchanged. */

#include "core/epic_harness.h"

/**
 * @brief Fan out to every peripheral IRQHandler. No-op on this core:
 *        no interrupt sources exist.
 */
void epic_dispatch_all_irqs(void)
{
    /* No interrupt sources on the 5x core. */
}
