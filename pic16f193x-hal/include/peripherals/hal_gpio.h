/**
 * @file    peripherals/hal_gpio.h
 * @brief   Family-neutral GPIO contract (`EPIC_GPIO_*` + change-interrupt hook).
 *
 * @details
 *   Family-agnostic consumers include this neutral name instead of the
 *   family-specific `pic16f193x_gpio.h`; the build's include path picks
 *   which family's copy resolves. Beyond the portable `EPIC_GPIO_Init/
 *   Read/Write/...`, this also carries the PORTB change-interrupt hook
 *   (@ref EPIC_GPIO_RegisterChangeCallback / @ref IOC_IRQHandler) that
 *   family-agnostic consumers build on.
 */

#ifndef EPIC_GPIO_H
#define EPIC_GPIO_H
#include "pic16f193x_gpio.h"
#endif /* EPIC_GPIO_H */
