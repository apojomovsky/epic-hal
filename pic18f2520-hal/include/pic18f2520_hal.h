/*
 * PIC18F2520 family top level: types, status codes, device selection,
 * SFR layer (@copyright © 2008 Microchip, DS39631E / DS39599 throughout).
 * 28-pin: PORTA/B/C plus RE3 input, no USB/SPP (no usbdiv/cpudiv/plldiv
 * in config). Two-vector interrupts (0008h/0018h; §9.0). Named _hal.h so
 * it never shadows the DFP proc header (the 628A lesson, epic-hal#137).
 */

#ifndef PIC18F2520_HAL_H
#define PIC18F2520_HAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @defgroup  PIC18F2520_Device Device Selection
 * @brief     Select the target device via a -D define (the build
 *            driver's contract). Defaults to PIC18F2520 when none is set.
 * @{
 */
#if !defined(PIC18F2520) && !defined(PIC18LF2520) && \
    !defined(PIC18F2220) && !defined(PIC18LF2220)
#define PIC18F2520   1
#endif

#if defined(PIC18F2520) + defined(PIC18F2220) > 1
#error "Define exactly one of PIC18F2520 / PIC18F2220."
#endif
/** @} */

/**
 * @defgroup  PIC18F2520_Capability Per-variant capability macros
 * @brief     Facts that differ across the family, derived from each
 *            part's DFP EDC data, never assumed from the datasheet
 *            family grouping alone (epic-hal AGENTS.md "probe, don't
 *            assume"). HAS_ECCP1 gates ECCP1AS/PWM1CON auto-shutdown
 *            (pic18f2520_ccp.c); HAS_BRG16 gates the BAUDCON/SPBRGH
 *            16-bit baud generator and auto-baud detect
 *            (pic18f2520_usart.c). Both are 1 on the 2520 (DS39631E),
 *            0 on the 2220 (DS39599: standard CCP1, 8-bit BRG only, no
 *            BAUDCON register at all).
 * @{
 */
#if defined(PIC18F2520)
#define PIC18F2520_FAMILY_FLASH_BYTES    32768U  /**< 32 KB (DS39631E §4.0). */
#define PIC18F2520_FAMILY_FLASH_INSTR    16384U
#define PIC18F2520_FAMILY_RAM_BYTES      1520U   /**< 0x0010-0x05FF (DS39631E §3.0). */
#define PIC18F2520_FAMILY_EEPROM_B       256U
#define PIC18F2520_FAMILY_IO_PINS        24U   /* PORTA/B/C; RE3/MCLR input excluded (DS39631E Table 1-1 counts 25). */
#define PIC18F2520_FAMILY_ADC_CH         10U
#define PIC18F2520_FAMILY_HAS_PORTD      0
#define PIC18F2520_FAMILY_HAS_PORTE      0   /* RE3/MCLR input only; no LATx/TRISx. */
#define PIC18F2520_FAMILY_HAS_SPP        0   /* no USB, so no SPP.   */
#define PIC18F2520_FAMILY_HAS_USB        0
#define PIC18F2520_FAMILY_HAS_ECCP1      1
#define PIC18F2520_FAMILY_HAS_BRG16      1
#define PIC18F2520_DEVICE_NAME           "PIC18F2520"
#elif defined(PIC18F2220)
#define PIC18F2520_FAMILY_FLASH_BYTES    4096U   /**< 4 KB (DS39599 §4.0). */
#define PIC18F2520_FAMILY_FLASH_INSTR    2048U
#define PIC18F2520_FAMILY_RAM_BYTES      512U    /**< 0x000-0x1FF (DS39599 §3.0). */
#define PIC18F2520_FAMILY_EEPROM_B       256U
#define PIC18F2520_FAMILY_IO_PINS        24U   /* PORTA/B/C; no PORTE at all on the 2220 (DS39599 Table 1-1). */
#define PIC18F2520_FAMILY_ADC_CH         10U
#define PIC18F2520_FAMILY_HAS_PORTD      0
#define PIC18F2520_FAMILY_HAS_PORTE      0   /* No PORTE SFR at all, not even an MCLR-only stub. */
#define PIC18F2520_FAMILY_HAS_SPP        0
#define PIC18F2520_FAMILY_HAS_USB        0
#define PIC18F2520_FAMILY_HAS_ECCP1      0   /* Standard CCP1: no ECCP1AS/PWM1CON (DS39599). */
#define PIC18F2520_FAMILY_HAS_BRG16      0   /* No BAUDCON register: 8-bit SPBRG only (DS39599). */
#define PIC18F2520_DEVICE_NAME           "PIC18F2220"
#endif
/** @} */

/**
 * Family-neutral aliases of the capability macros above, so family-agnostic
 * consumers (e.g. the task manager) can scale without a family-specific
 * name; `pic16f87xa.h` defines the same names to the PIC16 value.
 */
#define EPIC_FAMILY_RAM_BYTES   PIC18F2520_FAMILY_RAM_BYTES

/** Status enum and bit macros are architecture-blind, shared across
 *  every 8-bit PIC family; see epic-common/include/core/hal_status.h. */
#include "core/hal_status.h"

/**
 * @defgroup PIC18F2520_SFR Special Function Register mapping
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

#endif /* PIC18F2520_HAL_H */
