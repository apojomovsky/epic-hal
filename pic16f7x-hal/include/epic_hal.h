/* Family-neutral entry point to the PIC16F7x HAL. Consumers that must
 * build unchanged across 8-bit PIC families include this name instead
 * of pic16f7x.h; the build's include path selects the family's copy. */

#ifndef EPIC_H
#define EPIC_H

#include "pic16f7x.h"       /* standard types, status codes, platform   */
#include "pic16f7x_sfr.h"   /* SFR address map + bit definitions        */

/* Core. */
#include "core/pic16_irq.h"
#include "core/pic14_wdt_sleep.h"

/* Peripherals. */
#include "peripherals/pic16f7x_gpio.h"
#include "peripherals/pic16f7x_timer0.h"
#include "peripherals/pic16f7x_timer1.h"
#include "peripherals/pic16f7x_timer2.h"
#include "peripherals/pic16f7x_ccp.h"
#include "peripherals/pic16f7x_adc.h"
#if PIC16F7X_FAMILY_HAS_USART
#include "peripherals/pic16f7x_usart.h"
#endif
#if PIC16F7X_FAMILY_HAS_SSP
#include "peripherals/pic16f7x_ssp.h"
#endif
#if PIC16F7X_FAMILY_HAS_PSP
#include "peripherals/pic16f7x_psp.h"
#endif

#endif /* EPIC_H */
