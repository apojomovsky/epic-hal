/**
 * @file    core/hal_irq.h
 * @brief   Family-neutral `EPIC_IRQ_*` interrupt-control contract.
 *
 * @details
 *   Family-agnostic consumers include this neutral name instead of
 *   `pic18_irq.h` directly; each family provides its own copy, declaring
 *   `EPIC_IRQ_*` against that family's own `PIC*_IRQn` enum.
 */

#ifndef EPIC_HAL_IRQ_H
#define EPIC_HAL_IRQ_H
#include "pic18_irq.h"
#endif /* EPIC_HAL_IRQ_H */
