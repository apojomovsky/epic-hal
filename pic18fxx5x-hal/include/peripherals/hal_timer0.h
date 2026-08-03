/**
 * @file    peripherals/hal_timer0.h
 * @brief   Family-neutral Timer0 driver contract (`HAL_TIMER0_*`).
 *
 * @details
 *   Family-agnostic consumers include this neutral name instead of
 *   `pic18fxx5x_timer0.h` directly; each family provides its own copy,
 *   and the build's include path picks which one resolves.
 */

#ifndef HAL_TIMER0_H
#define HAL_TIMER0_H
#include "pic18fxx5x_timer0.h"
#endif /* HAL_TIMER0_H */
