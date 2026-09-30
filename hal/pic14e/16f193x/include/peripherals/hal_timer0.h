/**
 * Family-neutral Timer0 driver contract (`EPIC_TIMER0_*`): pulls in the
 * family-specific `pic16f193x_timer0.h`. The build's include path picks
 * which family's copy resolves.
 */

#ifndef EPIC_TIMER0_H
#define EPIC_TIMER0_H
#include "pic16f193x_timer0.h"
#endif /* EPIC_TIMER0_H */
