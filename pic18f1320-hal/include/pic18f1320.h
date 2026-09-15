/*
 * PIC18F1320: top-level types, status codes, SFR mapping layer
 * (DS39605F throughout). 18-pin part, smallest of epic-hal's three new
 * PIC18 families: 4096 words flash, 240 B RAM, no USB/MSSP/CCP2/
 * comparator (absent from the DFP). Same Access Bank/two-vector shape
 * as pic18fxx5x-hal. @copyright © 2007 Microchip (DS39605F).
 */

#ifndef PIC18F1320_H
#define PIC18F1320_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @defgroup  PIC18F1320_Device Device identity and capacity
 * @brief     This HAL supports one device: PIC18F1320 (p18f1320.toml in
 *            epic-cc: flash_words=4096, ram_banks=[[0x0010,0x00FF]]).
 * @{
 */
#define PIC18F1320_FAMILY_FLASH_BYTES    8192U   /**< 4096 words x 2. */
#define PIC18F1320_FAMILY_FLASH_INSTR    4096U
#define PIC18F1320_FAMILY_RAM_BYTES      240U    /**< 0x10-0xFF, single bank. */
#define PIC18F1320_FAMILY_EEPROM_B       256U
#define PIC18F1320_FAMILY_IO_PINS        16U     /**< RA0-7 + RB0-7, 18-pin package. */
#define PIC18F1320_FAMILY_ADC_CH         5U      /**< AN0-AN4, DS39605F §17.0. */
#define PIC18F1320_DEVICE_NAME           "PIC18F1320"
/** @} */

/** Family-neutral alias, so family-agnostic consumers (e.g. the task
 *  manager) can scale without a family-specific name; matches
 *  pic18fxx5x.h's own EPIC_FAMILY_RAM_BYTES pattern. */
#define EPIC_FAMILY_RAM_BYTES   PIC18F1320_FAMILY_RAM_BYTES

/** Status enum and bit macros are architecture-blind, shared across
 *  every 8-bit PIC family; see epic-common/include/core/hal_status.h. */
#include "core/hal_status.h"

/**
 * @defgroup PIC18F1320_SFR Special Function Register mapping
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

#endif /* PIC18F1320_H */
