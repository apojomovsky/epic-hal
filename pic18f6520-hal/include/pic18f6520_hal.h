/*
 * PIC18F6520 family top level: types, status codes, device selection,
 * SFR layer (DS39609B throughout). 64-pin: PORTA-G, no USB/SPP (no
 * usbdiv/cpudiv/plldiv/vregen in config). Two-vector interrupts
 * (0008h/0018h; DS39609B §9.0). Named _hal.h so it never shadows the
 * DFP proc header (the 628A lesson, epic-hal#137).
 */

#ifndef PIC18F6520_HAL_H
#define PIC18F6520_HAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @defgroup  PIC18F6520_Device Device Selection
 * @brief     Select the target device via a -D define (the build
 *            driver's contract). Defaults to PIC18F6520 when none is set.
 * @{
 */
#if !defined(PIC18F6520) && !defined(PIC18LF6520) && \
    !defined(PIC18F6620) && !defined(PIC18LF6620) && \
    !defined(PIC18F6720) && !defined(PIC18LF6720)
#define PIC18F6520   1
#endif

#if defined(PIC18F6520) + defined(PIC18F6620) + defined(PIC18F6720) > 1
#error "Define exactly one of PIC18F6520 / PIC18F6620 / PIC18F6720."
#endif
/** @} */

/**
 * @defgroup PIC18F6520_Capability Per-variant capability macros
 * @brief    Facts that differ across the family, derived from each
 *           part's DFP EDC data, never assumed from the datasheet
 *           family grouping alone.
 * @{
 */
#if defined(PIC18F6520)
 #define PIC18F6520_FAMILY_FLASH_BYTES    32768U  /**< 32 KB (DS39609B §4.0, 16384 words). */
 #define PIC18F6520_FAMILY_FLASH_INSTR    16384U
 #define PIC18F6520_FAMILY_RAM_BYTES      2032U   /**< 0x0010-0x07FF (DS39609B §4.0, Figure 4-6). */
 #define PIC18F6520_FAMILY_EEPROM_B       1024U   /**< 1 KB, EEADRH<1:0> (DS39609B §7.0). */
 #define PIC18F6520_FAMILY_IO_PINS        52U     /* PORTA-G digital I/O (DS39609B Table 1-1). */
 #define PIC18F6520_FAMILY_ADC_CH         12U     /* AN0-11, DS39609B §19.0. */
 #define PIC18F6520_FAMILY_HAS_PORTD      1
 #define PIC18F6520_FAMILY_HAS_PORTE      1
 #define PIC18F6520_FAMILY_HAS_SPP        0   /* no USB, so no SPP (DS39609B Table 1-1). */
 #define PIC18F6520_FAMILY_HAS_USB        0
 #define PIC18F6520_DEVICE_NAME           "PIC18F6520"
#elif defined(PIC18F6620)
#define PIC18F6520_FAMILY_FLASH_BYTES    65536U  /**< 64 KB (DS39609B §4.0, 32768 words). */
#define PIC18F6520_FAMILY_FLASH_INSTR    32768U
#define PIC18F6520_FAMILY_RAM_BYTES      3824U   /**< 0x0010-0x0EFF, GPR banks 0-14 (DS39609B §4.0). */
#define PIC18F6520_FAMILY_EEPROM_B       1024U   /**< 1 KB, same EEADRH shape as the 6520 (DS39609B §7.0). */
#define PIC18F6520_FAMILY_IO_PINS        52U     /* PORTA-G digital I/O, same pinout as the 6520. */
#define PIC18F6520_FAMILY_ADC_CH         12U     /* AN0-11, same as the 6520. */
#define PIC18F6520_FAMILY_HAS_PORTD      1
#define PIC18F6520_FAMILY_HAS_PORTE      1
#define PIC18F6520_FAMILY_HAS_SPP        0
#define PIC18F6520_FAMILY_HAS_USB        0
#define PIC18F6520_DEVICE_NAME           "PIC18F6620"
#elif defined(PIC18F6720)
#define PIC18F6520_FAMILY_FLASH_BYTES    131072U /**< 128 KB (DS39609B §4.0, 65536 words). */
#define PIC18F6520_FAMILY_FLASH_INSTR    65536U
#define PIC18F6520_FAMILY_RAM_BYTES      3824U   /**< 0x0010-0x0EFF, same GPR map as the 6620. */
#define PIC18F6520_FAMILY_EEPROM_B       1024U   /**< 1 KB, same as the 6520. */
#define PIC18F6520_FAMILY_IO_PINS        52U     /* PORTA-G digital I/O, same pinout as the 6520. */
#define PIC18F6520_FAMILY_ADC_CH         12U     /* AN0-11, same as the 6520. */
#define PIC18F6520_FAMILY_HAS_PORTD      1
#define PIC18F6520_FAMILY_HAS_PORTE      1
#define PIC18F6520_FAMILY_HAS_SPP        0
#define PIC18F6520_FAMILY_HAS_USB        0
#define PIC18F6520_DEVICE_NAME           "PIC18F6720"
#endif
/** @} */
/**
 * Family-neutral aliases of the capability macros above, so family-agnostic
 * consumers (e.g. the task manager) can scale without a family-specific
 * name; `pic16f87xa.h` defines the same names to the PIC16 value.
 */
#define EPIC_FAMILY_RAM_BYTES   PIC18F6520_FAMILY_RAM_BYTES

/** Status enum and bit macros are architecture-blind, shared across
 *  every 8-bit PIC family; see epic-common/include/core/hal_status.h. */
#include "core/hal_status.h"

/**
 * @defgroup PIC18F6520_SFR Special Function Register mapping
 * @brief   How every SFR is stored and how the weak attribute is spelled.
 *
 * The same source reads `epic_sfr_read8(addr)` (or `EPIC_REG8`) on both
 * builds; the implementation is chosen by include path, not `#ifdef`: host
 * CMake resolves `pic18_platform.h` to `include/host/...` (a memory-backed
 * register file), the XC8 Makefile to `include/target/...` (direct
 * volatile dereference). `@ref EPIC_WEAK` is likewise defined there.
 * @{
 */
#include "pic18_platform.h"
/** @} */

#endif /* PIC18F6520_HAL_H */
