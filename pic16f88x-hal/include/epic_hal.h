/* Family-neutral entry point to the PIC16F88X HAL. Consumers that must
 * build unchanged across 8-bit PIC families include this name instead
 * of pic16f88x.h; the build's include path selects the family's copy. */

#ifndef EPIC_H
#define EPIC_H

#include "pic16f88x.h"       /* standard types, status codes, platform   */
#include "pic16f88x_sfr.h"   /* SFR address map + bit definitions       */

/* Core. */
#include "core/pic16_irq.h"
#include "core/pic16f88x_wdt_sleep.h"

/* Peripherals. */
#include "peripherals/pic16f88x_gpio.h"
#include "peripherals/pic16f88x_timer0.h"
#include "peripherals/pic16f88x_timer1.h"
#include "peripherals/pic16f88x_timer2.h"
#include "peripherals/pic16f88x_ccp.h"
#include "peripherals/pic16f88x_usart.h"
#include "peripherals/pic16f88x_ssp.h"
#include "peripherals/pic16f88x_adc.h"
#include "peripherals/pic16f88x_comp.h"
#include "peripherals/pic16f88x_vref.h"
#include "peripherals/pic16f88x_srlatch.h"
#include "peripherals/pic16f88x_eeprom.h"
#include "peripherals/pic16f88x_osc.h"
#include "peripherals/pic16f88x_ulpwu.h"

#endif /* EPIC_H */
