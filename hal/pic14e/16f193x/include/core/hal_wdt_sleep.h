/**
 * Family-neutral WDT / Sleep / BOR / POR contract: pulls in the
 * family-specific `pic16f193x_wdt_sleep.h`. The build's include path picks
 * which family's copy resolves.
 */

#ifndef EPIC_WDT_SLEEP_H
#define EPIC_WDT_SLEEP_H
#include "pic16f193x_wdt_sleep.h"
#endif /* EPIC_WDT_SLEEP_H */
