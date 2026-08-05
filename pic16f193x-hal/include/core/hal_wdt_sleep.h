/**
 * @file    core/hal_wdt_sleep.h
 * @brief   Family-neutral WDT / Sleep / BOR / POR helper contract.
 *
 * @details
 *   Family-agnostic consumers include this neutral name instead of the
 *   family-specific `pic16f193x_wdt_sleep.h`. Each family provides its own
 *   `core/hal_wdt_sleep.h` that pulls in its family-specific WDT/Sleep
 *   header (same `EPIC_WDT_Refresh` / `EPIC_Sleep_Enter` / `EPIC_BOR_*` /
 *   `EPIC_POR_*` API, family-shaped bodies). The build's include path
 *   selects which family's copy resolves.
 */

#ifndef EPIC_WDT_SLEEP_H
#define EPIC_WDT_SLEEP_H
#include "pic16f193x_wdt_sleep.h"
#endif /* EPIC_WDT_SLEEP_H */
