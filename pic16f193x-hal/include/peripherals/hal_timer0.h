/**
 * @file    peripherals/hal_timer0.h
 * @brief   Family-neutral Timer0 driver contract (`HAL_TIMER0_*`).
 *
 * @details
 *   Family-agnostic consumers include this neutral name instead of the
 *   family-specific `pic16f193x_timer0.h`. Each family provides its own
 *   `peripherals/hal_timer0.h` that pulls in its family-specific Timer0
 *   header (same `TIMER0_HandleTypeDef` / `HAL_TIMER0_*` API, family-shaped
 *   bodies). The build's include path selects which family's copy resolves.
 */

#ifndef HAL_TIMER0_H
#define HAL_TIMER0_H
#include "pic16f193x_timer0.h"
#endif /* HAL_TIMER0_H */
