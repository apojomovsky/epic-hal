/*
 * Family-neutral top-level entry point to the PIC18F1320 HAL; each
 * family ships its own `epic_hal.h` under this name, picked up by the
 * build's include path. Foundation phase only: GPIO and Timer0.
 */

#ifndef EPIC_H
#define EPIC_H

#include "pic18f1320.h"       /* standard types, status codes, platform   */
#include "pic18f1320_sfr.h"   /* SFR address map + bit definitions       */

/* Core. */
#include "core/pic18_irq.h"
#include "core/pic18f1320_wdt_sleep.h"

/* Peripherals. */
#include "peripherals/pic18f1320_gpio.h"
#include "peripherals/pic18f1320_timer0.h"

#endif /* EPIC_H */
