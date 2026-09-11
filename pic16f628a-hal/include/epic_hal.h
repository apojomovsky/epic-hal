/* Family-neutral entry point to the PIC16F628A HAL. Consumers that
 * must build unchanged across 8-bit PIC families include this name
 * instead of pic16f628a.h; the build's include path selects the
 * family's copy. */

#ifndef EPIC_H
#define EPIC_H

#include "pic16f628a.h"       /* standard types, status codes, platform   */
#include "pic16f628a_sfr.h"   /* SFR address map + bit definitions       */

/* Core. */
#include "core/pic16_irq.h"
#include "core/pic14_wdt_sleep.h"

/* Peripherals (shared pic14-midrange-core drivers). */
#include "peripherals/pic14_gpio.h"
#include "peripherals/pic14_timer0.h"
#include "peripherals/pic14_timer1.h"
#include "peripherals/pic14_timer2.h"
#include "peripherals/pic14_ccp.h"
#include "peripherals/pic14_usart.h"
#include "peripherals/pic14_comp.h"
#include "peripherals/pic14_vref.h"
#include "peripherals/pic14_eeprom.h"

#endif /* EPIC_H */
