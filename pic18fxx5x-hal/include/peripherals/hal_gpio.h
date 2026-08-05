/**
 * @file    peripherals/hal_gpio.h
 * @brief   Family-neutral GPIO contract (`EPIC_GPIO_*` + RB-change hook).
 *
 * @details
 *   Family-agnostic consumers include this neutral name instead of
 *   `pic18fxx5x_gpio.h` directly; each family provides its own copy, and
 *   the build's include path picks which one resolves. Also carries the
 *   RB<7:4> change-interrupt hook `epic-encoder` builds on.
 */

#ifndef EPIC_GPIO_H
#define EPIC_GPIO_H
#include "pic18fxx5x_gpio.h"
#endif /* EPIC_GPIO_H */
