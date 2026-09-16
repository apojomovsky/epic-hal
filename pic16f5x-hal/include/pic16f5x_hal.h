/* PIC16F5x family top-level entry: standard types, status codes,
 * build-time device selection, and the SFR mapping layer. DS41213D
 * (16F5x) is authoritative for every constant; each peripheral header
 * cites its own section. Family: 12-bit baseline core, no interrupt
 * vector, 2-level hardware stack, FSR-selected data banks, TRIS/OPTION
 * as dedicated instructions (not file registers). Parts: 16F54
 * (canonical exemplar), 16F57, 16F59, 16F505, 16F506. */

#ifndef PIC16F5X_HAL_H
#define PIC16F5X_HAL_H

/* standard types. */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* build-time device selection. Defaults to PIC16F54 when none is
 * set; defining more than one is an error. */

#if !defined(PIC16F54) && !defined(PIC16F57) && !defined(PIC16F59) \
    && !defined(PIC16F505) && !defined(PIC16F506)
#define PIC16F54   1
#endif

#if defined(PIC16F54) + defined(PIC16F57) + defined(PIC16F59) \
    + defined(PIC16F505) + defined(PIC16F506) > 1
#error "Define exactly one of PIC16F54 / PIC16F57 / PIC16F59 / PIC16F505 / PIC16F506."
#endif

#if defined(PIC16F54)
  /* 512 words, 25 B GPR (0x07..0x1F), no bank bits (single data
   * bank). DS41213D §1.0. */
  #define PIC16F5X_FAMILY_FLASH_KW  1
  #define PIC16F5X_FAMILY_RAM_BYTES 25
  #define PIC16F5X_FAMILY_FSR_BANK_BITS 0
  #define PIC16F5X_FAMILY_HAS_PORTA 1
  #define PIC16F5X_FAMILY_HAS_PORTB 1
  #define PIC16F5X_FAMILY_HAS_PORTC 0
  #define PIC16F5X_FAMILY_HAS_PORTD 0
  #define PIC16F5X_FAMILY_HAS_PORTE 0
  #define PIC16F5X_FAMILY_HAS_OSCCAL 0
  #define PIC16F5X_FAMILY_HAS_COMP_ADC 0
  #define PIC16F5X_DEVICE_NAME      "PIC16F54"
#elif defined(PIC16F57)
  /* 2048 words, 72 B GPR (0x07..0x4F), single data bank. */
  #define PIC16F5X_FAMILY_FLASH_KW  2
  #define PIC16F5X_FAMILY_RAM_BYTES 72
  #define PIC16F5X_FAMILY_FSR_BANK_BITS 0
  #define PIC16F5X_FAMILY_HAS_PORTA 1
  #define PIC16F5X_FAMILY_HAS_PORTB 1
  #define PIC16F5X_FAMILY_HAS_PORTC 1
  #define PIC16F5X_FAMILY_HAS_PORTD 0
  #define PIC16F5X_FAMILY_HAS_PORTE 0
  #define PIC16F5X_FAMILY_HAS_OSCCAL 0
  #define PIC16F5X_FAMILY_HAS_COMP_ADC 0
  #define PIC16F5X_DEVICE_NAME      "PIC16F57"
#elif defined(PIC16F59)
  /* 2048 words, 73 B GPR, single data bank, PORTD/E added. */
  #define PIC16F5X_FAMILY_FLASH_KW  2
  #define PIC16F5X_FAMILY_RAM_BYTES 73
  #define PIC16F5X_FAMILY_FSR_BANK_BITS 0
  #define PIC16F5X_FAMILY_HAS_PORTA 1
  #define PIC16F5X_FAMILY_HAS_PORTB 1
  #define PIC16F5X_FAMILY_HAS_PORTC 1
  #define PIC16F5X_FAMILY_HAS_PORTD 1
  #define PIC16F5X_FAMILY_HAS_PORTE 1
  #define PIC16F5X_FAMILY_HAS_OSCCAL 0
  #define PIC16F5X_FAMILY_HAS_COMP_ADC 0
  #define PIC16F5X_DEVICE_NAME      "PIC16F59"
#elif defined(PIC16F505)
  /* 1024 words, 72 B GPR, 4 data banks selected by FSR<6:5>, no
   * PORTA (OSCCAL takes file 0x05), 20-pin. */
  #define PIC16F5X_FAMILY_FLASH_KW  1
  #define PIC16F5X_FAMILY_RAM_BYTES 72
  #define PIC16F5X_FAMILY_FSR_BANK_BITS 2
  #define PIC16F5X_FAMILY_HAS_PORTA 0
  #define PIC16F5X_FAMILY_HAS_PORTB 1
  #define PIC16F5X_FAMILY_HAS_PORTC 1
  #define PIC16F5X_FAMILY_HAS_PORTD 0
  #define PIC16F5X_FAMILY_HAS_PORTE 0
  #define PIC16F5X_FAMILY_HAS_OSCCAL 1
  #define PIC16F5X_FAMILY_HAS_COMP_ADC 0
  #define PIC16F5X_DEVICE_NAME      "PIC16F505"
#else  /* PIC16F506 */
  /* 1024 words, 72 B GPR, 4 data banks, comparator+ADC added. */
  #define PIC16F5X_FAMILY_FLASH_KW  1
  #define PIC16F5X_FAMILY_RAM_BYTES 72
  #define PIC16F5X_FAMILY_FSR_BANK_BITS 2
  #define PIC16F5X_FAMILY_HAS_PORTA 0
  #define PIC16F5X_FAMILY_HAS_PORTB 1
  #define PIC16F5X_FAMILY_HAS_PORTC 1
  #define PIC16F5X_FAMILY_HAS_PORTD 0
  #define PIC16F5X_FAMILY_HAS_PORTE 0
  #define PIC16F5X_FAMILY_HAS_OSCCAL 1
  #define PIC16F5X_FAMILY_HAS_COMP_ADC 1
  #define PIC16F5X_DEVICE_NAME      "PIC16F506"
#endif

/* Family-neutral capability aliases (epic-common contract): exposed
 * under family-neutral names too, so family-agnostic consumers can
 * scale without referencing a family-specific macro. */
#define EPIC_FAMILY_RAM_BYTES   PIC16F5X_FAMILY_RAM_BYTES

/* EPIC_StatusTypeDef/EPIC_OK/... and the EPIC_BIT* macros are
 * architecture-blind, so they live in the shared layer; pulled in here
 * so one `#include "pic16f5x_hal.h"` gives every consumer the same
 * status/bit vocabulary. */
#include "core/hal_status.h"

/* platform: SFR mapping + weak attribute. */
#include "pic16f5x_platform.h"

#endif /* PIC16F5X_HAL_H */
