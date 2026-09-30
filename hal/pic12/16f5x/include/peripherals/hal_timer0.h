/* Family-neutral Timer0 contract (EPIC_TIMER0_*). Pulls in the
 * family-specific pic16f5x_timer0.h; the build's include path picks
 * which family's copy resolves. */

#ifndef EPIC_TIMER0_H
#define EPIC_TIMER0_H
#include "peripherals/pic16f5x_timer0.h"
#endif /* EPIC_TIMER0_H */
