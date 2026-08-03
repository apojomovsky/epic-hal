/**
 * @file    peripherals/hal_timer2.h
 * @brief   Family-neutral Timer2 driver contract (`HAL_TIMER2_*`).
 *
 * @details
 *   Family-agnostic consumers include this neutral name instead of
 *   `pic18fxx5x_timer2.h` directly; each family provides its own copy,
 *   and the build's include path picks which one resolves.
 */

#ifndef HAL_TIMER2_H
#define HAL_TIMER2_H
#include "pic18fxx5x_timer2.h"
#endif /* HAL_TIMER2_H */
