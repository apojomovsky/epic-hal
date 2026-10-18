/**
 * PIC16F18855 HAL, consumer umbrella. Exposes the GPIO, Timer0, EUSART,
 * MSSP, PPS, EEPROM, NCO, WDT/Sleep and interrupt contract shared with
 * the other PIC14E families.
 */
#ifndef EPIC_H
#define EPIC_H

#include "peripherals/hal_gpio.h"
#include "peripherals/hal_timer0.h"
#include "peripherals/hal_usart.h"
#include "peripherals/hal_ssp.h"
#include "peripherals/hal_pps.h"
#include "peripherals/hal_eeprom.h"
#include "peripherals/hal_nco.h"
#include "core/hal_wdt_sleep.h"
#include "core/hal_irq.h"

#endif
