/*
 * Family-neutral top-level entry point to the PIC18F6520 HAL. Consumers
 * that must build unchanged against any 8-bit PIC family include this
 * instead of `pic18f6520_hal.h`; each family ships its own `epic_hal.h`
 * under the same neutral name and the build's include path decides
 * which family's headers are pulled in.
 */

#ifndef EPIC_H
#define EPIC_H

#include "pic18f6520_hal.h"    /* standard types, status codes, platform   */
#include "pic18f6520_sfr.h"    /* SFR address map + bit definitions       */

/* Core. */
#include "core/pic18_irq.h"
#include "core/pic18f6520_wdt_sleep.h"

/* Peripherals (GPIO + Timer0-3; CCP/MSSP/EUSART/analog land later). */
#include "peripherals/pic18f6520_gpio.h"
#include "peripherals/pic18f6520_timer0.h"
#include "peripherals/pic18f6520_timer1.h"
#include "peripherals/pic18f6520_timer2.h"
#include "peripherals/pic18f6520_timer3.h"

#endif /* EPIC_H */
