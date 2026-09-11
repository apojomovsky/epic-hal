/* PIC16F628A top-level entry: standard types, status codes, build-time
 * device selection, and the SFR mapping layer. DS40044G is authoritative
 * for every constant; each peripheral header cites its own section.
 * Single-part family: 18-pin, 2 KW flash, 224 B RAM, 128 B data EEPROM,
 * no ADC/SSP/CCP2/PSP/PIR2. */

#ifndef PIC16F628A_HAL_H
#define PIC16F628A_HAL_H

/* standard types. */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* build-time device selection (single part; defined when absent). */

#if !defined(PIC16F628A)
#define PIC16F628A   1
#endif

#define PIC16F628A_FAMILY_FLASH_KW   2
#define PIC16F628A_FAMILY_RAM_BYTES  224
#define PIC16F628A_FAMILY_EEPROM_B   128
#define PIC16F628A_FAMILY_ADC_CH     0
#define PIC16F628A_FAMILY_HAS_PORTC  0
#define PIC16F628A_FAMILY_HAS_PORTD  0
#define PIC16F628A_FAMILY_HAS_PORTE  0
#define PIC16F628A_FAMILY_HAS_PSP    0
#define PIC16F628A_DEVICE_NAME       "PIC16F628A"

/* Family-neutral capability aliases (epic-common contract): exposed
 * under family-neutral names too, so family-agnostic consumers (the
 * task manager) can scale without referencing a family-specific macro. */
#define EPIC_FAMILY_RAM_BYTES   PIC16F628A_FAMILY_RAM_BYTES

/* EPIC_StatusTypeDef/EPIC_OK/... and the EPIC_BIT* macros are
 * architecture-blind, so they live in the shared layer; pulled in here
 * so one `#include "pic16f628a_hal.h"` gives every consumer the same
 * status/bit vocabulary. */
#include "core/hal_status.h"

/* platform: SFR mapping + weak attribute. */
#include "pic16f628a_platform.h"

#endif /* PIC16F628A_HAL_H */
