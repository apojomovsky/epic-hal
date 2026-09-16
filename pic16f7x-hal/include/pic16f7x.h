/* PIC16F7x family top-level entry: standard types, status codes,
 * build-time device selection, and the SFR mapping layer. DS30325
 * (16F72-77) and DS30498 (16F737-777) are authoritative for every
 * constant; each peripheral header cites its own section. Family:
 * classic mid-range, the 877A-era peripheral surface (USART, CCP1/CCP2,
 * SSP, ADC, and on the 40-pin parts PSP/PortD/PortE), 2/4/8 KW flash,
 * 64-368 B RAM, no data EEPROM (the PM* bank replaces it). */

#ifndef PIC16F7X_HAL_H
#define PIC16F7X_HAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* build-time device selection. Defaults to PIC16F77 when none is set;
 * defining more than one is an error. */

#if !defined(PIC16F72) && !defined(PIC16F73) && !defined(PIC16F74) && \
    !defined(PIC16F76) && !defined(PIC16F77) && \
    !defined(PIC16F737) && !defined(PIC16F747) && \
    !defined(PIC16F767) && !defined(PIC16F777)
#define PIC16F77   1
#endif

#if defined(PIC16F72) + defined(PIC16F73) + defined(PIC16F74) + \
    defined(PIC16F76) + defined(PIC16F77) + \
    defined(PIC16F737) + defined(PIC16F747) + \
    defined(PIC16F767) + defined(PIC16F777) > 1
#error "Define exactly one of PIC16F72 / PIC16F73 / PIC16F74 / PIC16F76 / PIC16F77 / PIC16F737 / PIC16F747 / PIC16F767 / PIC16F777."
#endif

/* Flash/RAM/EEPROM sizes per part (DS30325 / DS30498 device summary
 * tables). RAM bytes are the GPR total (all banks + common RAM). No
 * data EEPROM on any part: the PM* program-memory bank replaces it.
 * 16F72 is the minimal 28-pin die: it has no USART, no CCP2, no
 * PIR2/PIE2 (its interrupt surface is PIR1-only), hence HAS_USART,
 * HAS_CCP2 and HAS_PIR2 are all 0 there. Its SPI-only SSP is present.
 */
#if   defined(PIC16F72)
  #define PIC16F7X_FAMILY_FLASH_KW   2
  #define PIC16F7X_FAMILY_RAM_BYTES  128
  #define PIC16F7X_FAMILY_ADC_10BIT  0
  #define PIC16F7X_FAMILY_HAS_USART  0
  #define PIC16F7X_FAMILY_HAS_CCP2   0
  #define PIC16F7X_FAMILY_HAS_PIR2   0
  #define PIC16F7X_FAMILY_HAS_PORTC  1
  #define PIC16F7X_FAMILY_HAS_PORTD  0
  #define PIC16F7X_FAMILY_HAS_PORTE  0
  #define PIC16F7X_FAMILY_HAS_PSP    0
  #define PIC16F7X_FAMILY_HAS_SSP    1
  #define PIC16F7X_DEVICE_NAME       "PIC16F72"
#elif defined(PIC16F73)
  #define PIC16F7X_FAMILY_FLASH_KW   4
  #define PIC16F7X_FAMILY_RAM_BYTES  192
  #define PIC16F7X_FAMILY_ADC_10BIT  0
  #define PIC16F7X_FAMILY_HAS_USART  1
  #define PIC16F7X_FAMILY_HAS_CCP2   1
  #define PIC16F7X_FAMILY_HAS_PIR2   1
  #define PIC16F7X_FAMILY_HAS_PORTC  1
  #define PIC16F7X_FAMILY_HAS_PORTD  0
  #define PIC16F7X_FAMILY_HAS_PORTE  0
  #define PIC16F7X_FAMILY_HAS_PSP    0
  #define PIC16F7X_FAMILY_HAS_SSP    1
  #define PIC16F7X_DEVICE_NAME       "PIC16F73"
#elif defined(PIC16F74)
  #define PIC16F7X_FAMILY_FLASH_KW   4
  #define PIC16F7X_FAMILY_RAM_BYTES  128
  #define PIC16F7X_FAMILY_ADC_10BIT  0
  #define PIC16F7X_FAMILY_HAS_USART  1
  #define PIC16F7X_FAMILY_HAS_CCP2   1
  #define PIC16F7X_FAMILY_HAS_PIR2   1
  #define PIC16F7X_FAMILY_HAS_PORTC  1
  #define PIC16F7X_FAMILY_HAS_PORTD  1
  #define PIC16F7X_FAMILY_HAS_PORTE  1
  #define PIC16F7X_FAMILY_HAS_PSP    1
  #define PIC16F7X_FAMILY_HAS_SSP    1
  #define PIC16F7X_DEVICE_NAME       "PIC16F74"
#elif defined(PIC16F76)
  #define PIC16F7X_FAMILY_FLASH_KW   8
  #define PIC16F7X_FAMILY_RAM_BYTES  368
  #define PIC16F7X_FAMILY_ADC_10BIT  0
  #define PIC16F7X_FAMILY_HAS_USART  1
  #define PIC16F7X_FAMILY_HAS_CCP2   1
  #define PIC16F7X_FAMILY_HAS_PIR2   1
  #define PIC16F7X_FAMILY_HAS_PORTC  1
  #define PIC16F7X_FAMILY_HAS_PORTD  0
  #define PIC16F7X_FAMILY_HAS_PORTE  0
  #define PIC16F7X_FAMILY_HAS_PSP    0
  #define PIC16F7X_FAMILY_HAS_SSP    1
  #define PIC16F7X_DEVICE_NAME       "PIC16F76"
#elif defined(PIC16F77)  /* exemplar, the default */
  #define PIC16F7X_FAMILY_FLASH_KW   8
  #define PIC16F7X_FAMILY_RAM_BYTES  368
  #define PIC16F7X_FAMILY_ADC_10BIT  0
  #define PIC16F7X_FAMILY_HAS_USART  1
  #define PIC16F7X_FAMILY_HAS_CCP2   1
  #define PIC16F7X_FAMILY_HAS_PIR2   1
  #define PIC16F7X_FAMILY_HAS_PORTC  1
  #define PIC16F7X_FAMILY_HAS_PORTD  1
  #define PIC16F7X_FAMILY_HAS_PORTE  1
  #define PIC16F7X_FAMILY_HAS_PSP    1
  #define PIC16F7X_FAMILY_HAS_SSP    1
  #define PIC16F7X_DEVICE_NAME       "PIC16F77"
#elif defined(PIC16F737)
  #define PIC16F7X_FAMILY_FLASH_KW   4
  #define PIC16F7X_FAMILY_RAM_BYTES  368
  #define PIC16F7X_FAMILY_ADC_10BIT  1
  #define PIC16F7X_FAMILY_HAS_USART  1
  #define PIC16F7X_FAMILY_HAS_CCP2   1
  #define PIC16F7X_FAMILY_HAS_PIR2   1
  #define PIC16F7X_FAMILY_HAS_PORTC  1
  #define PIC16F7X_FAMILY_HAS_PORTD  0
  #define PIC16F7X_FAMILY_HAS_PORTE  1
  #define PIC16F7X_FAMILY_HAS_PSP    0
  #define PIC16F7X_FAMILY_HAS_SSP    1
  #define PIC16F7X_DEVICE_NAME       "PIC16F737"
#elif defined(PIC16F747)
  #define PIC16F7X_FAMILY_FLASH_KW   4
  #define PIC16F7X_FAMILY_RAM_BYTES  368
  #define PIC16F7X_FAMILY_ADC_10BIT  1
  #define PIC16F7X_FAMILY_HAS_USART  1
  #define PIC16F7X_FAMILY_HAS_CCP2   1
  #define PIC16F7X_FAMILY_HAS_PIR2   1
  #define PIC16F7X_FAMILY_HAS_PORTC  1
  #define PIC16F7X_FAMILY_HAS_PORTD  1
  #define PIC16F7X_FAMILY_HAS_PORTE  1
  #define PIC16F7X_FAMILY_HAS_PSP    1
  #define PIC16F7X_FAMILY_HAS_SSP    1
  #define PIC16F7X_DEVICE_NAME       "PIC16F747"
#elif defined(PIC16F767)
  #define PIC16F7X_FAMILY_FLASH_KW   8
  #define PIC16F7X_FAMILY_RAM_BYTES  368
  #define PIC16F7X_FAMILY_ADC_10BIT  1
  #define PIC16F7X_FAMILY_HAS_USART  1
  #define PIC16F7X_FAMILY_HAS_CCP2   1
  #define PIC16F7X_FAMILY_HAS_PIR2   1
  #define PIC16F7X_FAMILY_HAS_PORTC  1
  #define PIC16F7X_FAMILY_HAS_PORTD  0
  #define PIC16F7X_FAMILY_HAS_PORTE  1
  #define PIC16F7X_FAMILY_HAS_PSP    0
  #define PIC16F7X_FAMILY_HAS_SSP    1
  #define PIC16F7X_DEVICE_NAME       "PIC16F767"
#else  /* PIC16F777 (default 8K/40-pin) */
  #define PIC16F7X_FAMILY_FLASH_KW   8
  #define PIC16F7X_FAMILY_RAM_BYTES  368
  #define PIC16F7X_FAMILY_ADC_10BIT  1
  #define PIC16F7X_FAMILY_HAS_USART  1
  #define PIC16F7X_FAMILY_HAS_CCP2   1
  #define PIC16F7X_FAMILY_HAS_PIR2   1
  #define PIC16F7X_FAMILY_HAS_PORTC  1
  #define PIC16F7X_FAMILY_HAS_PORTD  1
  #define PIC16F7X_FAMILY_HAS_PORTE  1
  #define PIC16F7X_FAMILY_HAS_PSP    1
  #define PIC16F7X_FAMILY_HAS_SSP    1
  #define PIC16F7X_DEVICE_NAME       "PIC16F777"
#endif

/* Family-neutral capability aliases (epic-common contract): exposed
 * under family-neutral names too, so family-agnostic consumers (the
 * task manager) can scale without referencing a family-specific macro. */
#define EPIC_FAMILY_RAM_BYTES   PIC16F7X_FAMILY_RAM_BYTES

/* EPIC_StatusTypeDef/EPIC_OK/... and the EPIC_BIT* macros are
 * architecture-blind, so they live in the shared layer; pulled in here
 * so one `#include "pic16f7x.h"` gives every consumer the same
 * status/bit vocabulary. */
#include "core/hal_status.h"

/* platform: SFR mapping + weak attribute. */
#include "pic16f7x_platform.h"

#endif /* PIC16F7X_HAL_H */
