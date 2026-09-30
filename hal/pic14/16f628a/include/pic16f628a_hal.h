/* PIC16F628A family top-level entry: standard types, status codes,
 * build-time device selection, and the SFR mapping layer. DS40044G
 * (627A/628A/648A) and DS40300C (627/628) are authoritative for every
 * constant; each peripheral header cites its own section. Family
 * (DS40044G Table 1-1, DS40300C features): 18-pin, 1/2/4 KW flash,
 * 224/256 B RAM, 128/256 B data EEPROM, no ADC/SSP/CCP2/PSP/PIR2; the
 * peripheral set is identical on every part, only memory sizes (and
 * the LF suffix's voltage range) differ. */

#ifndef PIC16F628A_HAL_H
#define PIC16F628A_HAL_H

/* standard types. */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* build-time device selection. Defaults to PIC16F628A when none is
 * set; defining more than one is an error. */

#if !defined(PIC16F627) && !defined(PIC16F627A) && !defined(PIC16F628) && \
    !defined(PIC16F628A) && !defined(PIC16F648A) && \
    !defined(PIC16LF627A) && !defined(PIC16LF628A)
#define PIC16F628A   1
#endif

#if defined(PIC16F627) + defined(PIC16F627A) + defined(PIC16F628) + \
    defined(PIC16F628A) + defined(PIC16F648A) + defined(PIC16LF627A) + \
    defined(PIC16LF628A) > 1
#error "Define exactly one of PIC16F627 / PIC16F627A / PIC16F628 / PIC16F628A / PIC16F648A / PIC16LF627A / PIC16LF628A."
#endif

#if   defined(PIC16F627)
  #define PIC16F628A_FAMILY_FLASH_KW   1
  #define PIC16F628A_FAMILY_RAM_BYTES  224
  #define PIC16F628A_FAMILY_EEPROM_B   128
  #define PIC16F628A_FAMILY_ADC_CH     0
  #define PIC16F628A_FAMILY_HAS_PORTC  0
  #define PIC16F628A_FAMILY_HAS_PORTD  0
  #define PIC16F628A_FAMILY_HAS_PORTE  0
  #define PIC16F628A_FAMILY_HAS_PSP    0
  #define PIC16F628A_DEVICE_NAME       "PIC16F627"
#elif defined(PIC16F628)
  #define PIC16F628A_FAMILY_FLASH_KW   2
  #define PIC16F628A_FAMILY_RAM_BYTES  224
  #define PIC16F628A_FAMILY_EEPROM_B   128
  #define PIC16F628A_FAMILY_ADC_CH     0
  #define PIC16F628A_FAMILY_HAS_PORTC  0
  #define PIC16F628A_FAMILY_HAS_PORTD  0
  #define PIC16F628A_FAMILY_HAS_PORTE  0
  #define PIC16F628A_FAMILY_HAS_PSP    0
  #define PIC16F628A_DEVICE_NAME       "PIC16F628"
#elif defined(PIC16F627A)
  #define PIC16F628A_FAMILY_FLASH_KW   1
  #define PIC16F628A_FAMILY_RAM_BYTES  224
  #define PIC16F628A_FAMILY_EEPROM_B   128
  #define PIC16F628A_FAMILY_ADC_CH     0
  #define PIC16F628A_FAMILY_HAS_PORTC  0
  #define PIC16F628A_FAMILY_HAS_PORTD  0
  #define PIC16F628A_FAMILY_HAS_PORTE  0
  #define PIC16F628A_FAMILY_HAS_PSP    0
  #define PIC16F628A_DEVICE_NAME       "PIC16F627A"
#elif defined(PIC16F648A)
  #define PIC16F628A_FAMILY_FLASH_KW   4
  #define PIC16F628A_FAMILY_RAM_BYTES  256
  #define PIC16F628A_FAMILY_EEPROM_B   256
  #define PIC16F628A_FAMILY_ADC_CH     0
  #define PIC16F628A_FAMILY_HAS_PORTC  0
  #define PIC16F628A_FAMILY_HAS_PORTD  0
  #define PIC16F628A_FAMILY_HAS_PORTE  0
  #define PIC16F628A_FAMILY_HAS_PSP    0
  #define PIC16F628A_DEVICE_NAME       "PIC16F648A"
#elif defined(PIC16LF627A)
  #define PIC16F628A_FAMILY_FLASH_KW   1
  #define PIC16F628A_FAMILY_RAM_BYTES  224
  #define PIC16F628A_FAMILY_EEPROM_B   128
  #define PIC16F628A_FAMILY_ADC_CH     0
  #define PIC16F628A_FAMILY_HAS_PORTC  0
  #define PIC16F628A_FAMILY_HAS_PORTD  0
  #define PIC16F628A_FAMILY_HAS_PORTE  0
  #define PIC16F628A_FAMILY_HAS_PSP    0
  #define PIC16F628A_DEVICE_NAME       "PIC16LF627A"
#elif defined(PIC16LF628A)
  #define PIC16F628A_FAMILY_FLASH_KW   2
  #define PIC16F628A_FAMILY_RAM_BYTES  224
  #define PIC16F628A_FAMILY_EEPROM_B   128
  #define PIC16F628A_FAMILY_ADC_CH     0
  #define PIC16F628A_FAMILY_HAS_PORTC  0
  #define PIC16F628A_FAMILY_HAS_PORTD  0
  #define PIC16F628A_FAMILY_HAS_PORTE  0
  #define PIC16F628A_FAMILY_HAS_PSP    0
  #define PIC16F628A_DEVICE_NAME       "PIC16LF628A"
#else  /* PIC16F628A (default) */
  #define PIC16F628A_FAMILY_FLASH_KW   2
  #define PIC16F628A_FAMILY_RAM_BYTES  224
  #define PIC16F628A_FAMILY_EEPROM_B   128
  #define PIC16F628A_FAMILY_ADC_CH     0
  #define PIC16F628A_FAMILY_HAS_PORTC  0
  #define PIC16F628A_FAMILY_HAS_PORTD  0
  #define PIC16F628A_FAMILY_HAS_PORTE  0
  #define PIC16F628A_FAMILY_HAS_PSP    0
  #define PIC16F628A_DEVICE_NAME       "PIC16F628A"
#endif

/* Family-neutral capability aliases (epic-common contract): exposed
 * under family-neutral names too, so family-agnostic consumers (the
 * task manager) can scale without referencing a family-specific macro. */
#define EPIC_FAMILY_RAM_BYTES   PIC16F628A_FAMILY_RAM_BYTES

/* EPIC_StatusTypeDef/EPIC_OK/... and the EPIC_BIT* macros are
 * architecture-blind, so they live in the shared layer; pulled in here
 * so one `#include "pic16f628a_hal.h"` gives every consumer the same
 * status/bit vocabulary. */
#include "core/hal_status.h"

/* platform: SFR mapping + weak attribute. */
#include "pic16f628a_platform.h"

#endif /* PIC16F628A_HAL_H */
