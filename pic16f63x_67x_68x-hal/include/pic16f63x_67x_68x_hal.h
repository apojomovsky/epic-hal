/* PIC16F63x/67x/68x family top-level entry: standard types, status
 * codes, build-time device selection, and the SFR mapping layer.
 * DS40001262F (631/677/685/689/690) is authoritative for the 16F631
 * constants; each peripheral header cites its own section. Family
 * shape on this ticket: the 20-pin 16F631/677 (dual comparators,
 * Timer0 + Timer1 with gate, no USART/CCP/Timer2; 631: 1024 W flash,
 * 64 B SRAM, 128 B EEPROM, no ADC. 677: 2048 W flash, 128 B SRAM,
 * 256 B EEPROM, ADC + SSP; DS40001262F Table 1). The remaining
 * siblings (630/639/676/684/685/688/689) join in epic-hal#152 under
 * capability macros, so this header selects exactly one device the
 * same way the 628A umbrella does. */

#ifndef PIC16F63X_67X_68X_HAL_H
#define PIC16F63X_67X_68X_HAL_H

/* standard types. */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* build-time device selection. Single-device family on this ticket
 * (the #152 siblings extend both conditionals); defaults to PIC16F631
 * when none is set, defining more than one is an error. */
#if !defined(PIC16F631) && !defined(PIC16F677)
#define PIC16F631   1
#endif

#if defined(PIC16F631) + defined(PIC16F677) > 1
#error "Define exactly one of PIC16F631 / PIC16F677."
#endif

#if defined(PIC16F677)
  #define PIC16F63X_67X_68X_FAMILY_FLASH_KW   2
  #define PIC16F63X_67X_68X_FAMILY_RAM_BYTES  128
  #define PIC16F63X_67X_68X_FAMILY_EEPROM_B   256
  #define PIC16F63X_67X_68X_FAMILY_ADC_CH     12
  #define PIC16F63X_67X_68X_FAMILY_HAS_PORTC  1
  #define PIC16F63X_67X_68X_FAMILY_HAS_PORTD  0
  #define PIC16F63X_67X_68X_FAMILY_HAS_PORTE  0
  #define PIC16F63X_67X_68X_FAMILY_HAS_ANSELH 1
  #define PIC16F63X_67X_68X_DEVICE_NAME       "PIC16F677"
#else  /* PIC16F631 (default) */
  #define PIC16F63X_67X_68X_FAMILY_FLASH_KW   1
  #define PIC16F63X_67X_68X_FAMILY_RAM_BYTES  64
  #define PIC16F63X_67X_68X_FAMILY_EEPROM_B   128
  #define PIC16F63X_67X_68X_FAMILY_ADC_CH     0
  #define PIC16F63X_67X_68X_FAMILY_HAS_PORTC  1
  #define PIC16F63X_67X_68X_FAMILY_HAS_PORTD  0
  #define PIC16F63X_67X_68X_FAMILY_HAS_PORTE  0
  #define PIC16F63X_67X_68X_FAMILY_HAS_ANSELH 0
  #define PIC16F63X_67X_68X_DEVICE_NAME       "PIC16F631"
#endif

/* Family-neutral capability aliases (epic-common contract): exposed
 * under family-neutral names too, so family-agnostic consumers (the
 * task manager) can scale without referencing a family-specific macro. */
#define EPIC_FAMILY_RAM_BYTES   PIC16F63X_67X_68X_FAMILY_RAM_BYTES

/* EPIC_StatusTypeDef/EPIC_OK/... and the EPIC_BIT* macros are
 * architecture-blind, so they live in the shared layer; pulled in here
 * so one `#include "pic16f63x_67x_68x_hal.h"` gives every consumer the
 * same status/bit vocabulary. */
#include "core/hal_status.h"

/* platform: SFR mapping + weak attribute. */
#include "pic16f63x_67x_68x_platform.h"

#endif /* PIC16F63X_67X_68X_HAL_H */
