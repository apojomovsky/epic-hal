/* Family-neutral entry point to the PIC16F818/819 HAL. Consumers that
 * must build unchanged across 8-bit PIC families include this name
 * instead of pic16f818_819_hal.h; the build's include path selects the
 * family's copy. */

#ifndef EPIC_H
#define EPIC_H

#include "pic16f818_819_hal.h"       /* standard types, status, platform */
#include "pic16f818_819_sfr.h"       /* SFR address map + bit definitions */

/* Core. */
#include "core/pic16_irq.h"
#include "core/pic14_wdt_sleep.h"

/* Peripherals. */
#include "peripherals/hal_gpio.h"
#include "peripherals/hal_timer0.h"
#include "peripherals/hal_timer1.h"
#include "peripherals/hal_timer2.h"
#include "peripherals/hal_ccp.h"
#include "peripherals/hal_ssp.h"
#include "peripherals/hal_adc.h"
#include "peripherals/hal_eeprom.h"

#endif /* EPIC_H */
