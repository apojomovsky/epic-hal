/**
 * @file    core/hal_wdt_sleep.h
 * @brief   Family-neutral WDT / Sleep / BOR / POR helper contract.
 *
 * @details
 *   Family-agnostic consumers include this neutral name instead of
 *   `pic18fxx5x_wdt_sleep.h` directly; each family provides its own copy,
 *   and the build's include path picks which one resolves.
 */

#ifndef EPIC_WDT_SLEEP_H
#define EPIC_WDT_SLEEP_H
#include "pic18fxx5x_wdt_sleep.h"
#endif /* EPIC_WDT_SLEEP_H */
