/**
 * @file    peripherals/hal_gpio.h
 * @brief   Family-neutral GPIO contract (`HAL_GPIO_*` + change-interrupt hook).
 *
 * @details
 *   Family-agnostic consumers include this neutral name instead of the
 *   family-specific `pic16f193x_gpio.h`; the build's include path picks
 *   which family's copy resolves. Beyond the portable `HAL_GPIO_Init/
 *   Read/Write/...`, this also carries the PORTB change-interrupt hook
 *   (@ref HAL_GPIO_RegisterChangeCallback / @ref IOC_IRQHandler) that
 *   family-agnostic consumers build on.
 */

#ifndef HAL_GPIO_H
#define HAL_GPIO_H
#include "pic16f193x_gpio.h"
#endif /* HAL_GPIO_H */
