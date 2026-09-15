/*
 * PIC18F2520 family: top-level types, status codes, build-time device
 * selection, and the SFR mapping layer. DS39631E (PIC18F2420/2520/4420/
 * 4520) is authoritative for every constant, bit name, and reset value;
 * each peripheral header cites its section. The PIC18F2520 is a 28-pin
 * part: PORTA/B/C plus a single RE3 input (no PORTD), a 10-channel ADC,
 * ECCP1 + CCP2, MSSP (SSP), EUSART, two comparators, 256 B data EEPROM.
 * It has no USB peripheral and no SPP, so the config words carry none of
 * the 2455-family's usbdiv/cpudiv/plldiv/vregen fields. It shares the
 * PIC18 two-vector interrupt scheme (0008h high, 0018h low; DS39631E
 * §9.0).
 *
 * The umbrella file is named pic18f2520_hal.h, not pic18f2520.h: the
 * DFP proc header for this part is literally pic18f2520.h, and an
 * umbrella named the same would shadow it through -I at link time and
 * strip the compiler's auto-included device SFRs (the PIC16F628A
 * lesson, epic-hal#137). The host/target platform headers are reached
 * through pic18_platform.h, chosen by include path.
 *
 * @copyright © 2008 Microchip Technology Inc. (datasheet DS39631E).
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
#define PIC18F2520_FAMILY_IO_PINS        24U
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
