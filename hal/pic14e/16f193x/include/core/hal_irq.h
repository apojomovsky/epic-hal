/**
 * Family-neutral `EPIC_IRQ_*` contract: pulls in the family-specific
 * `pic16f193x_irq.h`. The build's include path picks which family's copy
 * resolves.
 */

#ifndef EPIC_HAL_IRQ_H
#define EPIC_HAL_IRQ_H
#include "pic16f193x_irq.h"
#endif /* EPIC_HAL_IRQ_H */
