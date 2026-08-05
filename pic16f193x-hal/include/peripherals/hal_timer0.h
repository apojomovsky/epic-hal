/**
 * @file    peripherals/hal_timer0.h
 * @brief   Family-neutral Timer0 driver contract (`EPIC_TIMER0_*`).
 *
 * @details
 *   Family-agnostic consumers include this neutral name instead of the
 *   family-specific `pic16f193x_timer0.h`. Each family provides its own
 *   `peripherals/hal_timer0.h` that pulls in its family-specific Timer0
 *   header (same `TIMER0_HandleTypeDef` / `EPIC_TIMER0_*` API, family-shaped
 *   bodies). The build's include path selects which family's copy resolves.
 */

#ifndef EPIC_TIMER0_H
#define EPIC_TIMER0_H
#include "pic16f193x_timer0.h"
#endif /* EPIC_TIMER0_H */
