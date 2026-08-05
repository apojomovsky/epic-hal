/**
 * @file    pic16f193x.h
 * @brief   PIC16F193X family, top-level types, status codes, build-time device
 *          selection, and the simulated/real-target SFR mapping layer.
 *
 * @details
 *   Single entry point for the PIC16F193X HAL: standard types, status enums,
 *   and the device-specific SFR include. DS41364B is authoritative for every
 *   constant; each peripheral header cites its own section.
 *
 *   Enhanced Mid-range core (DS41364B §2.0, §4.0): BSR banked data memory
 *   (up to 32 banks x 128 bytes), single interrupt vector at 0x0004 with no
 *   priority and automatic context save. Distinct from classic PIC16F87XA
 *   (RP0/RP1 banking, manual ISR save) and from PIC18 (Access Bank, two
 *   vectors with priority), so this is its own family, not a variant.
 *
 * Target family (DS41364B §1.0, Table 1-1; the preliminary rev B datasheet
 * documented all six parts; current Microchip revisions split them across
 * DS41364C, DS40001574D, DS41575, but the SFR/peripheral layout is identical
 * across all six, so one HAL covers every variant):
 *   - PIC16F1933, 28-pin,  4 KW flash, 256 B RAM, 256 B EEPROM, 11 ADC ch.
 *   - PIC16F1934, 40/44-pin, 4 KW flash, 256 B RAM, 256 B EEPROM, 14 ADC ch.,
 *                   PORTD + PORTE.
 *   - PIC16F1936, 28-pin,  8 KW flash, 512 B RAM, 256 B EEPROM, 11 ADC ch.
 *   - PIC16F1937, 40/44-pin, 8 KW flash, 512 B RAM, 256 B EEPROM, 14 ADC ch.,
 *                   PORTD + PORTE.
 *   - PIC16F1938, 28-pin, 16 KW flash, 1024 B RAM, 256 B EEPROM, 11 ADC ch.
 *   - PIC16F1939, 40/44-pin, 16 KW flash, 1024 B RAM, 256 B EEPROM, 14 ADC ch.,
 *                   PORTD + PORTE.
 *
 * @copyright © 2009 Microchip Technology Inc. (datasheet DS41364B).
 */

#ifndef PIC16F193X_H
#define PIC16F193X_H

/* ───────────────────────── standard types ───────────────────────── */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ───────────────────────── build-time device selection ──────────── */

/**
 * @defgroup  PIC16F193X_Device Device Selection
 * @brief     Select exactly one target device before including any
 *            peripheral header. Defaults to PIC16F1937 when none is set.
 * @{
 */
#if !defined(PIC16F1933) && !defined(PIC16F1934) && \
    !defined(PIC16F1936) && !defined(PIC16F1937) && \
    !defined(PIC16F1938) && !defined(PIC16F1939)
#define PIC16F1937   1
#endif

#if defined(PIC16F1933) + defined(PIC16F1934) + \
    defined(PIC16F1936) + defined(PIC16F1937) + \
    defined(PIC16F1938) + defined(PIC16F1939) > 1
#error "Define exactly one of PIC16F1933/1934/1936/1937/1938/1939."
#endif

#if   defined(PIC16F1933)
  #define PIC16F193X_FAMILY_FLASH_KW   4
  #define PIC16F193X_FAMILY_RAM_BYTES   256
  #define PIC16F193X_FAMILY_EEPROM_B   256
  #define PIC16F193X_FAMILY_ADC_CH     11
  #define PIC16F193X_FAMILY_HAS_PORTD  0
  #define PIC16F193X_FAMILY_HAS_PORTE  0
  #define PIC16F193X_DEVICE_NAME       "PIC16F1933"
#elif defined(PIC16F1934)
  #define PIC16F193X_FAMILY_FLASH_KW   4
  #define PIC16F193X_FAMILY_RAM_BYTES   256
  #define PIC16F193X_FAMILY_EEPROM_B   256
  #define PIC16F193X_FAMILY_ADC_CH     14
  #define PIC16F193X_FAMILY_HAS_PORTD  1
  #define PIC16F193X_FAMILY_HAS_PORTE  1
  #define PIC16F193X_DEVICE_NAME       "PIC16F1934"
#elif defined(PIC16F1936)
  #define PIC16F193X_FAMILY_FLASH_KW   8
  #define PIC16F193X_FAMILY_RAM_BYTES   512
  #define PIC16F193X_FAMILY_EEPROM_B   256
  #define PIC16F193X_FAMILY_ADC_CH     11
  #define PIC16F193X_FAMILY_HAS_PORTD  0
  #define PIC16F193X_FAMILY_HAS_PORTE  0
  #define PIC16F193X_DEVICE_NAME       "PIC16F1936"
#elif defined(PIC16F1937)
  #define PIC16F193X_FAMILY_FLASH_KW   8
  #define PIC16F193X_FAMILY_RAM_BYTES   512
  #define PIC16F193X_FAMILY_EEPROM_B   256
  #define PIC16F193X_FAMILY_ADC_CH     14
  #define PIC16F193X_FAMILY_HAS_PORTD  1
  #define PIC16F193X_FAMILY_HAS_PORTE  1
  #define PIC16F193X_DEVICE_NAME       "PIC16F1937"
#elif defined(PIC16F1938)
  #define PIC16F193X_FAMILY_FLASH_KW   16
  #define PIC16F193X_FAMILY_RAM_BYTES  1024
  #define PIC16F193X_FAMILY_EEPROM_B   256
  #define PIC16F193X_FAMILY_ADC_CH     11
  #define PIC16F193X_FAMILY_HAS_PORTD  0
  #define PIC16F193X_FAMILY_HAS_PORTE  0
  #define PIC16F193X_DEVICE_NAME       "PIC16F1938"
#else  /* PIC16F1939 */
  #define PIC16F193X_FAMILY_FLASH_KW   16
  #define PIC16F193X_FAMILY_RAM_BYTES  1024
  #define PIC16F193X_FAMILY_EEPROM_B   256
  #define PIC16F193X_FAMILY_ADC_CH     14
  #define PIC16F193X_FAMILY_HAS_PORTD  1
  #define PIC16F193X_FAMILY_HAS_PORTE  1
  #define PIC16F193X_DEVICE_NAME       "PIC16F1939"
#endif
/** @} */

/* LCD segment count: 24 on 40/44-pin (1934/1937/1939), 16 on 28-pin
 * (1933/1936/1938), per DS41364B §1 device table. */
#if PIC16F193X_FAMILY_HAS_PORTD
  #define PIC16F193X_FAMILY_LCD_SEGMENTS  24
#else
  #define PIC16F193X_FAMILY_LCD_SEGMENTS  16
#endif

/* Family-neutral capability aliases (pic8-common contract): exposed
 * under family-neutral names too, so family-agnostic consumers (the
 * task manager) can scale without referencing a family-specific macro.
 * `pic16f87xa.h` and `pic18fxx5x.h` define the same names to their own
 * values. */
#define PIC8_FAMILY_RAM_BYTES   PIC16F193X_FAMILY_RAM_BYTES

/* EPIC_StatusTypeDef/EPIC_OK/... and the PIC8_BIT* macros are
 * architecture-blind, so they live in the shared layer; pulled in here
 * so one `#include "pic16f193x.h"` gives every consumer the same
 * status/bit vocabulary. */
#include "core/hal_status.h"

/* ───────────── platform: SFR mapping + weak attribute ───────────── */
/**
 * @defgroup PIC16F193X_SFR Special Function Register mapping
 * @brief   How every SFR is stored and how the weak attribute is spelled.
 *
 * Same source reads `pic8_sfr_read8()`/`PIC8_REG8()` on both builds;
 * the build's include path picks `include/host/...` (memory-backed) or
 * `include/target/...` (direct volatile deref), not `#ifdef`.
 * @{
 */
#include "pic16f193x_platform.h"
/** @} */

#endif /* PIC16F193X_H */
