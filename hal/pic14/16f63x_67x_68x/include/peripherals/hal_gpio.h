/* Family-neutral GPIO contract (EPIC_GPIO_* + RB-change hook). Pulls
 * in the family-specific gpio header; the include path picks which
 * family's copy resolves. */

#ifndef EPIC_GPIO_H
#define EPIC_GPIO_H
#include "pic16f63x_67x_68x_gpio.h"
#endif /* EPIC_GPIO_H */
