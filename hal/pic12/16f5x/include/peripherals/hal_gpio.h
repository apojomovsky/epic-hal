/* Family-neutral GPIO contract (EPIC_GPIO_*). Pulls in the
 * family-specific pic16f5x_gpio.h; the build's include path picks
 * which family's copy resolves. */

#ifndef EPIC_HAL_GPIO_H
#define EPIC_HAL_GPIO_H
#include "peripherals/pic16f5x_gpio.h"
#endif /* EPIC_HAL_GPIO_H */
