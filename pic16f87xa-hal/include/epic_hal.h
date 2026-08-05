/**
 * @file    epic_hal.h
 * @brief   Family-neutral top-level entry point to the PIC16F87XA HAL.
 *
 * @details
 *   A consumer that builds unchanged against any 8-bit PIC family
 *   includes this neutral name instead of `pic16f87xa.h`; each family
 *   provides its own `epic_hal.h` under the same name, selected by
 *   which family's HAL tree is on the include path.
 */

#ifndef PIC8_EPIC_H
#define PIC8_EPIC_H

#include "pic16f87xa.h"       /* standard types, status codes, platform   */
#include "pic16f87xa_sfr.h"   /* SFR address map + bit definitions       */

/* Core. */
#include "core/pic16_irq.h"
#include "core/pic16f87xa_wdt_sleep.h"

/* Peripherals. */
#include "peripherals/pic16f87xa_gpio.h"
#include "peripherals/pic16f87xa_timer0.h"
#include "peripherals/pic16f87xa_timer1.h"
#include "peripherals/pic16f87xa_timer2.h"
#include "peripherals/pic16f87xa_ccp.h"
#include "peripherals/pic16f87xa_usart.h"
#include "peripherals/pic16f87xa_ssp.h"
#include "peripherals/pic16f87xa_adc.h"
#include "peripherals/pic16f87xa_comp.h"
#include "peripherals/pic16f87xa_vref.h"
#include "peripherals/pic16f87xa_eeprom.h"
#if PIC16F87XA_FAMILY_HAS_PSP
#include "peripherals/pic16f87xa_psp.h"
#endif

#endif /* PIC8_EPIC_H */
