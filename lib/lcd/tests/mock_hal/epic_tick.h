/** Host stub of epic_tick.h for the LCD transport tests.

Resolves in place of lib/tick's header on the transport test targets'
include path. Records delay_ms calls so the tests can assert the
transport's delay mapping. */

#ifndef EPIC_TICK_H
#define EPIC_TICK_H

#include <stdint.h>

/** @brief Record a millisecond delay request. */
void epic_tick_delay_ms(uint32_t ms);

#endif /* EPIC_TICK_H */
