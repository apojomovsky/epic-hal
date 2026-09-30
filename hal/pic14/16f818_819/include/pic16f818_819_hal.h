/* PIC16F818_819 family top-level entry: standard types, status codes,
 * build-time device selection, and the SFR mapping layer. DS39598F is
 * authoritative for every constant; each peripheral header cites its
 * own register section. Family: 18/20/28-pin, 1/2 KW flash, 128/256 B
 * RAM, 128/256 B data EEPROM, PORTA + PORTB, Timer0/1/2, CCP1, SSP
 * (SPI + I2C slave), 10-bit 5-channel ADC, WDT, Sleep. No USART,
 * comparator, VREF, CCP2, PSP, PORTC/D/E. The SFR map is byte-identical
 * across both parts (DFP-verified); only memory sizes differ. */

#ifndef PIC16F818_819_HAL_H
#define PIC16F818_819_HAL_H

/* standard types. */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* build-time device selection. Defaults to PIC16F819 when none is
 * set; defining more than one is an error. */

#if !defined(PIC16F818) && !defined(PIC16F819)
#define PIC16F819   1
#endif

#if defined(PIC16F818) + defined(PIC16F819) > 1
#error "Define exactly one of PIC16F818 / PIC16F819."
#endif

#if   defined(PIC16F818)
  #define PIC16F818_819_FAMILY_FLASH_KW  1
  #define PIC16F818_819_FAMILY_RAM_BYTES 128
  #define PIC16F818_819_FAMILY_EEPROM_B  128
  #define PIC16F818_819_FAMILY_ADC_CH    5
  #define PIC16F818_819_DEVICE_NAME      "PIC16F818"
#else  /* PIC16F819 (default) */
  #define PIC16F818_819_FAMILY_FLASH_KW  2
  #define PIC16F818_819_FAMILY_RAM_BYTES 256
  #define PIC16F818_819_FAMILY_EEPROM_B  256
  #define PIC16F818_819_FAMILY_ADC_CH    5
  #define PIC16F818_819_DEVICE_NAME      "PIC16F819"
#endif

/* Family-neutral capability aliases (epic-common contract): exposed
 * under family-neutral names too, so family-agnostic consumers (the
 * task manager) can scale without referencing a family-specific macro. */
#define EPIC_FAMILY_RAM_BYTES   PIC16F818_819_FAMILY_RAM_BYTES

/* EPIC_StatusTypeDef/EPIC_OK/... and the EPIC_BIT* macros are
 * architecture-blind, so they live in the shared layer; pulled in here
 * so one `#include "pic16f818_819_hal.h"` gives every consumer the same
 * status/bit vocabulary. */
#include "core/hal_status.h"

/* platform: SFR mapping + weak attribute. */
#include "pic16f818_819_platform.h"

#endif /* PIC16F818_819_HAL_H */
