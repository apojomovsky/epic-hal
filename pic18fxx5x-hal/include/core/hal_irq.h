/**
 * @file    core/hal_irq.h
 * @brief   Family-neutral `HAL_IRQ_*` interrupt-control contract.
 *
 * @details
 *   Family-agnostic consumers include this neutral name instead of
 *   `pic18_irq.h` directly; each family provides its own copy, declaring
 *   `HAL_IRQ_*` against that family's own `PIC*_IRQn` enum.
 */

#ifndef HAL_IRQ_H
#define HAL_IRQ_H
#include "pic18_irq.h"
#endif /* HAL_IRQ_H */
