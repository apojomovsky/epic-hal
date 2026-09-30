/* Family-neutral entry point to the PIC16F83/84/84A HAL. Consumers
 * that must build unchanged across 8-bit PIC families include this
 * name instead of pic16f83_84_hal.h; the build's include path selects
 * the family's copy. */

#ifndef EPIC_H
#define EPIC_H

#include "pic16f83_84_hal.h"      /* standard types, status codes, platform */
#include "pic16f83_84_sfr.h"      /* SFR address map + bit definitions      */

/* Core. */
#include "core/pic16_irq.h"
#include "core/pic14_wdt_sleep.h"

/* Peripherals (shared pic14-midrange-core drivers; this bare tier
 * has GPIO, Timer0 and data EEPROM only). */
#include "peripherals/pic14_gpio.h"
#include "peripherals/pic14_timer0.h"
#include "peripherals/pic14_eeprom.h"

#endif /* EPIC_H */
