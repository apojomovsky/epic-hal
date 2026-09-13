/* PIC16F63x/67x/68x family top-level entry: standard types, status
 * codes, build-time device selection, and the SFR mapping layer.
 * DS40001262F Table 1 is authoritative; each peripheral header cites
 * its own section. Onboarded here: 16F631 (1 KW/64 B/128 B, no ADC)
 * and 16F677 canonical (2 KW/128 B/256 B, ADC + SSP). The rest join
 * in epic-hal#152; this header selects exactly one device. */

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
