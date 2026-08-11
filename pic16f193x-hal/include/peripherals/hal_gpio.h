/**
 * Family-neutral GPIO contract (`EPIC_GPIO_*` + change-interrupt hook):
 * pulls in the family-specific `pic16f193x_gpio.h`. The build's include
 * path picks which family's copy resolves.
 */

#ifndef EPIC_GPIO_H
#define EPIC_GPIO_H
#include "pic16f193x_gpio.h"
#endif /* EPIC_GPIO_H */
