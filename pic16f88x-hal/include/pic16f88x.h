/* PIC16F88X top-level entry: standard types, status codes, build-time
 * device selection, and the SFR mapping layer. DS40001291H is
 * authoritative for every constant; each peripheral header cites its own
 * section. Devices (DS40001291H §1.0, Table "Family Types"): 882/883/
 * 884/886/887, 2/4/8 KW flash, 128/256/368 B RAM, 128/256 B EEPROM,
 * 11/14 ADC channels. */

#ifndef PIC16F88X_H
#define PIC16F88X_H

/* standard types. */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* build-time device selection. */

/**
 * @defgroup  PIC16F88X_Device Device Selection
 * @brief     Select exactly one target device before including any
 *            peripheral header. Defaults to PIC16F887 when none is set.
 * @{
 */
#if !defined(PIC16F882) && !defined(PIC16F883) && !defined(PIC16F884) && \
    !defined(PIC16F886) && !defined(PIC16F887)
#define PIC16F887   1
#endif

#if defined(PIC16F882) + defined(PIC16F883) + defined(PIC16F884) + \
    defined(PIC16F886) + defined(PIC16F887) > 1
#error "Define exactly one of PIC16F882 / PIC16F883 / PIC16F884 / PIC16F886 / PIC16F887."
#endif

#if   defined(PIC16F882)
  #define PIC16F88X_FAMILY_FLASH_KW   2
  #define PIC16F88X_FAMILY_RAM_BYTES  128
  #define PIC16F88X_FAMILY_EEPROM_B   128
  #define PIC16F88X_FAMILY_ADC_CH     11
  #define PIC16F88X_FAMILY_HAS_PORTD  0
  #define PIC16F88X_FAMILY_HAS_PORTE  0
  #define PIC16F88X_FAMILY_HAS_BANK2_GPR 0
  #define PIC16F88X_DEVICE_NAME       "PIC16F882"
#elif defined(PIC16F883)
  #define PIC16F88X_FAMILY_FLASH_KW   4
  #define PIC16F88X_FAMILY_RAM_BYTES  256
  #define PIC16F88X_FAMILY_EEPROM_B   256
  #define PIC16F88X_FAMILY_ADC_CH     11
  #define PIC16F88X_FAMILY_HAS_PORTD  0
  #define PIC16F88X_FAMILY_HAS_PORTE  0
  #define PIC16F88X_FAMILY_HAS_BANK2_GPR 1
  #define PIC16F88X_DEVICE_NAME       "PIC16F883"
#elif defined(PIC16F884)
  #define PIC16F88X_FAMILY_FLASH_KW   4
  #define PIC16F88X_FAMILY_RAM_BYTES  256
  #define PIC16F88X_FAMILY_EEPROM_B   256
  #define PIC16F88X_FAMILY_ADC_CH     14
  #define PIC16F88X_FAMILY_HAS_PORTD  1
  #define PIC16F88X_FAMILY_HAS_PORTE  1
  #define PIC16F88X_FAMILY_HAS_BANK2_GPR 1
  #define PIC16F88X_DEVICE_NAME       "PIC16F884"
#elif defined(PIC16F886)
  #define PIC16F88X_FAMILY_FLASH_KW   8
  #define PIC16F88X_FAMILY_RAM_BYTES  368
  #define PIC16F88X_FAMILY_EEPROM_B   256
  #define PIC16F88X_FAMILY_ADC_CH     11
  #define PIC16F88X_FAMILY_HAS_PORTD  0
  #define PIC16F88X_FAMILY_HAS_PORTE  0
  #define PIC16F88X_FAMILY_HAS_BANK2_GPR 1
  #define PIC16F88X_DEVICE_NAME       "PIC16F886"
#else  /* PIC16F887 */
  #define PIC16F88X_FAMILY_FLASH_KW   8
  #define PIC16F88X_FAMILY_RAM_BYTES  368
  #define PIC16F88X_FAMILY_EEPROM_B   256
  #define PIC16F88X_FAMILY_ADC_CH     14
  #define PIC16F88X_FAMILY_HAS_PORTD  1
  #define PIC16F88X_FAMILY_HAS_PORTE  1
  #define PIC16F88X_FAMILY_HAS_BANK2_GPR 1
  #define PIC16F88X_DEVICE_NAME       "PIC16F887"
#endif
/** @} */

/* Family-neutral capability aliases (epic-common contract): exposed
 * under family-neutral names too, so family-agnostic consumers (the
 * task manager) can scale without referencing a family-specific macro.
 * `pic16f87xa.h` and `pic18fxx5x.h` define the same names to their own
 * values. */
#define EPIC_FAMILY_RAM_BYTES   PIC16F88X_FAMILY_RAM_BYTES

/* EPIC_StatusTypeDef/EPIC_OK/... and the EPIC_BIT* macros are
 * architecture-blind, so they live in the shared layer; pulled in here
 * so one `#include "pic16f88x.h"` gives every consumer the same
 * status/bit vocabulary. */
#include "core/hal_status.h"

/* platform: SFR mapping + weak attribute. */
/**
 * @defgroup PIC16F88X_SFR Special Function Register mapping
 * @brief   How every SFR is stored and how the weak attribute is spelled.
 *
 * Same source reads `epic_sfr_read8()`/`EPIC_REG8()` on both builds;
 * the build's include path picks `include/host/...` (memory-backed) or
 * `include/target/...` (direct volatile deref), not `#ifdef`.
 * @{
 */
#include "pic16f88x_platform.h"
/** @} */

#endif /* PIC16F88X_H */
