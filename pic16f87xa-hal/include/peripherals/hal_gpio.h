/**
 * @file    peripherals/hal_gpio.h
 * @brief   Family-neutral GPIO contract (`HAL_GPIO_*` + RB-change hook).
 *
 * @details
 *   Family-agnostic consumers include this neutral name instead of the
 *   family-specific `pic16f87xa_gpio.h`; the build's include path picks
 *   which family's copy resolves. Beyond the portable `HAL_GPIO_Init/
 *   Read/Write/...`, this also carries the RB<7:4> change-interrupt hook
 *   (@ref HAL_GPIO_RegisterChangeCallback / @ref RB_IRQHandler) that
 *   `pic8-encoder` builds on.
 */

#ifndef HAL_GPIO_H
#define HAL_GPIO_H
#include "pic16f87xa_gpio.h"
#endif /* HAL_GPIO_H */
