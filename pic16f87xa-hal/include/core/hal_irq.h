/**
 * @file    core/hal_irq.h
 * @brief   Family-neutral `EPIC_IRQ_*` interrupt-control contract.
 *
 * @details
 *   Family-agnostic consumers include this neutral name instead of the
 *   family-specific `pic16_irq.h`. Each family provides its own
 *   `core/hal_irq.h` that pulls in its family-specific irq header (which
 *   declares the `EPIC_IRQ_*` functions against that family's `PIC*_IRQn`
 *   enum and interrupt registers). The build's include path selects which
 *   family's copy resolves.
 */

#ifndef EPIC_IRQ_H
#define EPIC_IRQ_H
#include "pic16_irq.h"
#endif /* EPIC_IRQ_H */
