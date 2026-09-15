/*
 * PIC18F2520 family top level: types, status codes, device selection,
 * SFR layer (@copyright © 2008 Microchip, DS39631E throughout). 28-pin:
 * PORTA/B/C plus RE3 input, no USB/SPP (no usbdiv/cpudiv/plldiv in
 * config). Two-vector interrupts (0008h/0018h; §9.0). Named _hal.h so
 * it never shadows the DFP proc header (the 628A lesson, epic-hal#137).
 */

#ifndef PIC18F2520_HAL_H
#define PIC18F2520_HAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @defgroup  PIC18F2520_Device Device Selection
 * @brief     Select the target device. Only the PIC18F2520 is in this
 *            family for now; the -D PIC18F2520 define is the contract
 *            the build driver emits. Defined here so a bare compile
 *            without the driver still selects a device.
 * @{
 */
#if !defined(PIC18F2520) && !defined(PIC18LF2520)
#define PIC18F2520   1
#endif
/** @} */

/* The PIC18F2520 is the only variant; keep the size facts here rather
 * than behind a 1-way #if so they stay a single source of truth. */
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
#define PIC18F2520_DEVICE_NAME           "PIC18F2520"

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
