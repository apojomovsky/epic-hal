/**
 * @file    pic18fxx5x.h
 * @brief   PIC18F2455/2550/4455/4550 family, top-level types, status codes,
 *          build-time device selection, and the simulated/real-target SFR
 *          mapping layer.
 *
 * @details
 *   The single entry point for the PIC18F2455 family HAL, mirroring
 *   `pic16f87xa.h` (and `stm32fxxx_hal.h` in STM32Cube): standard integer
 *   types, status codes, and the platform layer that defines how every SFR
 *   is stored. The datasheet (DS39632E) is authoritative for every
 *   constant, bit name, and reset value implemented here; every peripheral
 *   header cites the section it maps to.
 *
 * Target family (DS39632E page 1, "PIC18F2455/2550/4455/4550" feature
 * table):
 *   - PIC18F2455, 28-pin,  24 KB flash, 12288 instr, 2048 B RAM, 256 B
 *                   EEPROM, 24 I/O, 10 ADC ch, 2 CCP / 0 ECCP, no SPP.
 *   - PIC18F2550, 28-pin,  32 KB flash, 16384 instr, 2048 B RAM, 256 B
 *                   EEPROM, 24 I/O, 10 ADC ch, 2 CCP / 0 ECCP, no SPP.
 *   - PIC18F4455, 40/44-pin, 24 KB flash, 12288 instr, 2048 B RAM, 256 B
 *                   EEPROM, 35 I/O, 13 ADC ch, 1 CCP / 1 ECCP, SPP,
 *                   PORTD + PORTE.
 *   - PIC18F4550, 40/44-pin, 32 KB flash, 16384 instr, 2048 B RAM, 256 B
 *                   EEPROM, 35 I/O, 13 ADC ch, 1 CCP / 1 ECCP, SPP,
 *                   PORTD + PORTE.
 *   All four have the USB module, MSSP (SPI + I2C master), EUSART, two
 *   analog comparators, and the dual-priority interrupt scheme (vectors at
 *   0008h high, 0018h low; DS39632E §9.0).
 *
 * @copyright © 2009 Microchip Technology Inc. (datasheet DS39632E).
 */

#ifndef PIC18FXX5X_H
#define PIC18FXX5X_H

/* ───────────────────────── standard types ───────────────────────── */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ───────────────────────── build-time device selection ──────────── */

/**
 * @defgroup  PIC18FXX5X_Device Device Selection
 * @brief     Select exactly one target device before including any
 *            peripheral header. Defaults to PIC18F4550 when none is set
 *            (40/44-pin, the family's most-featured part).
 * @{
 */
#if !defined(PIC18F2455) && !defined(PIC18F2550) && \
    !defined(PIC18F4455) && !defined(PIC18F4550)
#define PIC18F4550   1
#endif

#if defined(PIC18F2455) + defined(PIC18F2550) + \
    defined(PIC18F4455) + defined(PIC18F4550) > 1
#error "Define exactly one of PIC18F2455 / PIC18F2550 / PIC18F4455 / PIC18F4550."
#endif

#if   defined(PIC18F2455)
  #define PIC18FXX5X_FAMILY_FLASH_BYTES    24576U  /**< 24 KB.             */
  #define PIC18FXX5X_FAMILY_FLASH_INSTR    12288U
  #define PIC18FXX5X_FAMILY_RAM_BYTES      2048U
  #define PIC18FXX5X_FAMILY_EEPROM_B       256U
  #define PIC18FXX5X_FAMILY_IO_PINS        24U
  #define PIC18FXX5X_FAMILY_ADC_CH         10U
  #define PIC18FXX5X_FAMILY_HAS_PORTD      0
  #define PIC18FXX5X_FAMILY_HAS_PORTE      0
  #define PIC18FXX5X_FAMILY_HAS_SPP        0
  #define PIC18FXX5X_FAMILY_HAS_USB        1
  #define PIC18FXX5X_DEVICE_NAME           "PIC18F2455"
#elif defined(PIC18F2550)
  #define PIC18FXX5X_FAMILY_FLASH_BYTES    32768U  /**< 32 KB.             */
  #define PIC18FXX5X_FAMILY_FLASH_INSTR    16384U
  #define PIC18FXX5X_FAMILY_RAM_BYTES      2048U
  #define PIC18FXX5X_FAMILY_EEPROM_B       256U
  #define PIC18FXX5X_FAMILY_IO_PINS        24U
  #define PIC18FXX5X_FAMILY_ADC_CH         10U
  #define PIC18FXX5X_FAMILY_HAS_PORTD      0
  #define PIC18FXX5X_FAMILY_HAS_PORTE      0
  #define PIC18FXX5X_FAMILY_HAS_SPP        0
  #define PIC18FXX5X_FAMILY_HAS_USB        1
  #define PIC18FXX5X_DEVICE_NAME           "PIC18F2550"
#elif defined(PIC18F4455)
  #define PIC18FXX5X_FAMILY_FLASH_BYTES    24576U  /**< 24 KB.             */
  #define PIC18FXX5X_FAMILY_FLASH_INSTR    12288U
  #define PIC18FXX5X_FAMILY_RAM_BYTES      2048U
  #define PIC18FXX5X_FAMILY_EEPROM_B       256U
  #define PIC18FXX5X_FAMILY_IO_PINS        35U
  #define PIC18FXX5X_FAMILY_ADC_CH         13U
  #define PIC18FXX5X_FAMILY_HAS_PORTD      1
  #define PIC18FXX5X_FAMILY_HAS_PORTE      1
  #define PIC18FXX5X_FAMILY_HAS_SPP        1
  #define PIC18FXX5X_FAMILY_HAS_USB        1
  #define PIC18FXX5X_DEVICE_NAME           "PIC18F4455"
#else  /* PIC18F4550 */
  #define PIC18FXX5X_FAMILY_FLASH_BYTES    32768U  /**< 32 KB.             */
  #define PIC18FXX5X_FAMILY_FLASH_INSTR    16384U
  #define PIC18FXX5X_FAMILY_RAM_BYTES      2048U
  #define PIC18FXX5X_FAMILY_EEPROM_B       256U
  #define PIC18FXX5X_FAMILY_IO_PINS        35U
  #define PIC18FXX5X_FAMILY_ADC_CH         13U
  #define PIC18FXX5X_FAMILY_HAS_PORTD      1
  #define PIC18FXX5X_FAMILY_HAS_PORTE      1
  #define PIC18FXX5X_FAMILY_HAS_SPP        1
  #define PIC18FXX5X_FAMILY_HAS_USB        1
  #define PIC18FXX5X_DEVICE_NAME           "PIC18F4550"
#endif
/** @} */

/* ─────────── family-neutral capability aliases (pic8-common contract) ── */
/**
 * Family-neutral aliases of the capability macros above, so family-agnostic
 * consumers (e.g. the task manager) can scale without a family-specific
 * name; `pic16f87xa.h` defines the same names to the PIC16 value.
 */
#define PIC8_FAMILY_RAM_BYTES   PIC18FXX5X_FAMILY_RAM_BYTES

/* ─────────── shared HAL status codes + bit helpers (pic8-common) ── */
/** Status enum and bit macros are architecture-blind, shared across
 *  every 8-bit PIC family; see pic8-common/include/core/hal_status.h. */
#include "core/hal_status.h"

/* ───────────── platform: SFR mapping + weak attribute ───────────── */
/**
 * @defgroup PIC18FXX5X_SFR Special Function Register mapping
 * @brief   How every SFR is stored and how the weak attribute is spelled.
 *
 * The same source reads `pic8_sfr_read8(addr)` (or `PIC8_REG8`) on both
 * builds; the implementation is chosen by include path, not `#ifdef`: host
 * CMake resolves `pic18_platform.h` to `include/host/...` (a memory-backed
 * register file), the XC8 Makefile to `include/target/...` (direct
 * volatile dereference). `@ref PIC8_WEAK` is likewise defined there.
 * @{
 */
#include "pic18_platform.h"
/** @} */

#endif /* PIC18FXX5X_H */
