/**
 * @file    epic_hal.h
 * @brief   Family-neutral top-level entry point to the PIC16F193X HAL.
 *
 * @details
 *   A consumer that builds unchanged against any 8-bit PIC family
 *   includes this neutral name instead of `pic16f193x.h`; each family
 *   provides its own `epic_hal.h` under the same name, selected by
 *   which family's HAL tree is on the include path.
 *
 *   Foundation scope: only the core (IRQ, WDT/Sleep) and the GPIO +
 *   Timer0 peripherals are present. Each peripheral phase appends its
 *   header include here as it is built.
 */

#ifndef EPIC_H
#define EPIC_H

#include "pic16f193x.h"       /* standard types, status codes, platform    */
#include "pic16f193x_sfr.h"   /* SFR address map + bit definitions        */

/* Core. */
#include "core/pic16f193x_irq.h"
#include "core/pic16f193x_wdt_sleep.h"

/* Peripherals. */
#include "peripherals/pic16f193x_gpio.h"
#include "peripherals/pic16f193x_timer0.h"

#endif /* EPIC_H */
