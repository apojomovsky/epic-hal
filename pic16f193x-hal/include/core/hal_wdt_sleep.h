/**
 * @file    core/hal_wdt_sleep.h
 * @brief   Family-neutral WDT / Sleep / BOR / POR helper contract.
 *
 * @details
 *   Family-agnostic consumers include this neutral name instead of the
 *   family-specific `pic16f193x_wdt_sleep.h`. Each family provides its own
 *   `core/hal_wdt_sleep.h` that pulls in its family-specific WDT/Sleep
 *   header (same `HAL_WDT_Refresh` / `HAL_Sleep_Enter` / `HAL_BOR_*` /
 *   `HAL_POR_*` API, family-shaped bodies). The build's include path
 *   selects which family's copy resolves.
 */

#ifndef HAL_WDT_SLEEP_H
#define HAL_WDT_SLEEP_H
#include "pic16f193x_wdt_sleep.h"
#endif /* HAL_WDT_SLEEP_H */
