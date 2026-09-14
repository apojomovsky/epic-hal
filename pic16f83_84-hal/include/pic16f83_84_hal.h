/* PIC16F83_84 family top-level entry: standard types, status codes,
 * build-time device selection, and the SFR mapping layer. DS35007B
 * (16F84A) and DS30189 (16F83/16F84) are authoritative for every
 * constant; each peripheral header cites its own section. Family: 18
 * pins, 0.5/1/1 KW flash, 36/68/68 B RAM, 64 B data EEPROM, GPIO
 * (PORTA 5-bit + PORTB), Timer0, EEPROM, WDT; no USART/CCP/timers
 * 1-2/comparator/VREF/ADC/SSP/PIR/PCON. The SFR map is byte-identical
 * across all three parts (DFP-verified); only memory sizes differ. */

#ifndef PIC16F83_84_HAL_H
#define PIC16F83_84_HAL_H

/* standard types. */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* build-time device selection. Defaults to PIC16F84A when none is
 * set; defining more than one is an error. */

#if !defined(PIC16F83) && !defined(PIC16F84) && !defined(PIC16F84A)
#define PIC16F84A   1
#endif

#if defined(PIC16F83) + defined(PIC16F84) + defined(PIC16F84A) > 1
#error "Define exactly one of PIC16F83 / PIC16F84 / PIC16F84A."
#endif

#if   defined(PIC16F83)
  /* 512 words = 0.5 KW; the KW column ceils to 1 (its only consumer
   * is the dispatcher's two-page >= 4 test, and the part is single
   * page either way). */
  #define PIC16F83_84_FAMILY_FLASH_KW  1
  #define PIC16F83_84_FAMILY_RAM_BYTES 36
  #define PIC16F83_84_FAMILY_EEPROM_B  64
  #define PIC16F83_84_FAMILY_ADC_CH    0
  #define PIC16F83_84_FAMILY_HAS_PORTC 0
  #define PIC16F83_84_FAMILY_HAS_PORTD 0
  #define PIC16F83_84_FAMILY_HAS_PORTE 0
  #define PIC16F83_84_FAMILY_HAS_PSP   0
  #define PIC16F83_84_DEVICE_NAME      "PIC16F83"
#elif defined(PIC16F84)
  #define PIC16F83_84_FAMILY_FLASH_KW  1
  #define PIC16F83_84_FAMILY_RAM_BYTES 68
  #define PIC16F83_84_FAMILY_EEPROM_B  64
  #define PIC16F83_84_FAMILY_ADC_CH    0
  #define PIC16F83_84_FAMILY_HAS_PORTC 0
  #define PIC16F83_84_FAMILY_HAS_PORTD 0
  #define PIC16F83_84_FAMILY_HAS_PORTE 0
  #define PIC16F83_84_FAMILY_HAS_PSP   0
  #define PIC16F83_84_DEVICE_NAME      "PIC16F84"
#else  /* PIC16F84A (default) */
  #define PIC16F83_84_FAMILY_FLASH_KW  1
  #define PIC16F83_84_FAMILY_RAM_BYTES 68
  #define PIC16F83_84_FAMILY_EEPROM_B  64
  #define PIC16F83_84_FAMILY_ADC_CH    0
  #define PIC16F83_84_FAMILY_HAS_PORTC 0
  #define PIC16F83_84_FAMILY_HAS_PORTD 0
  #define PIC16F83_84_FAMILY_HAS_PORTE 0
  #define PIC16F83_84_FAMILY_HAS_PSP   0
  #define PIC16F83_84_DEVICE_NAME      "PIC16F84A"
#endif

/* Family-neutral capability aliases (epic-common contract): exposed
 * under family-neutral names too, so family-agnostic consumers (the
 * task manager) can scale without referencing a family-specific macro. */
#define EPIC_FAMILY_RAM_BYTES   PIC16F83_84_FAMILY_RAM_BYTES

/* EPIC_StatusTypeDef/EPIC_OK/... and the EPIC_BIT* macros are
 * architecture-blind, so they live in the shared layer; pulled in here
 * so one `#include "pic16f83_84_hal.h"` gives every consumer the same
 * status/bit vocabulary. */
#include "core/hal_status.h"

/* platform: SFR mapping + weak attribute. */
#include "pic16f83_84_platform.h"

#endif /* PIC16F83_84_HAL_H */
