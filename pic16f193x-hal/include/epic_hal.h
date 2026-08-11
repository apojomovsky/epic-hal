/**
 * Family-neutral top-level entry point to the PIC16F193X HAL: consumers
 * that build unchanged against any 8-bit PIC family include this neutral
 * name instead of `pic16f193x.h`. Each family provides its own `epic_hal.h`
 * under the same name, selected by which family's HAL tree is on the
 * include path.
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
#include "peripherals/pic16f193x_timer1.h"
#include "peripherals/pic16f193x_ccp.h"

#endif /* EPIC_H */
