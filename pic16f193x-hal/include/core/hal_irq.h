/**
 * @file    core/hal_irq.h
 * @brief   Family-neutral `HAL_IRQ_*` interrupt-control contract.
 *
 * @details
 *   Family-agnostic consumers include this neutral name instead of the
 *   family-specific `pic16f193x_irq.h`. Each family provides its own
 *   `core/hal_irq.h` that pulls in its family-specific irq header (which
 *   declares the `HAL_IRQ_*` functions against that family's `PIC*_IRQn`
 *   enum and interrupt registers). The build's include path selects which
 *   family's copy resolves.
 */

#ifndef HAL_IRQ_H
#define HAL_IRQ_H
#include "pic16f193x_irq.h"
#endif /* HAL_IRQ_H */
