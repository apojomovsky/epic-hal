/* Family-neutral entry point to the PIC16F63x/67x/68x HAL. Consumers
 * that must build unchanged across 8-bit PIC families include this name
 * instead of pic16f63x_67x_68x_hal.h; the build's include path selects
 * the family's copy. */

#ifndef EPIC_H
#define EPIC_H

#include "pic16f63x_67x_68x_hal.h"  /* standard types, status codes, platform */
#include "pic16f63x_67x_68x_sfr.h"  /* SFR address map + bit definitions */

/* Core. */
#include "core/pic16_irq.h"
#include "core/pic14_wdt_sleep.h"

/* Peripherals (family drivers on this ticket; the shared
 * pic14-midrange-core drivers for Timer0/Timer1/EEPROM). */
#include "peripherals/pic16f63x_67x_68x_gpio.h"
#include "peripherals/pic14_timer0.h"
#include "peripherals/pic14_timer1.h"
#include "peripherals/pic16f63x_67x_68x_comp.h"
#include "peripherals/pic14_eeprom.h"
#include "peripherals/pic16f63x_67x_68x_osc.h"

#endif /* EPIC_H */
