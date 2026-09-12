/* PIC14 mid-range family selectors for PIC16F88X: device selection,
 * FAMILY_* macros, status codes and the platform layer. Shared
 * pic14-midrange-core code includes this name; each family's copy
 * resolves via the include path. */

#ifndef PIC14_MIDRANGE_H
#define PIC14_MIDRANGE_H
#include "pic16f88x.h"

/* Capability contract for shared pic14-midrange-core code (1 = present).
 * Per-device values ride on the FAMILY_* selectors above; family-fixed
 * values are literals. */
#define PIC14MIDRANGE_FLASH_KW PIC16F88X_FAMILY_FLASH_KW
#define PIC14MIDRANGE_HAS_PIR2 1
#define PIC14MIDRANGE_HAS_SSP 1
#define PIC14MIDRANGE_HAS_SSPMSK 1
#define PIC14MIDRANGE_HAS_ADC 1
#define PIC14MIDRANGE_HAS_ADC_PCFG 0
#define PIC14MIDRANGE_HAS_CCP2 1
#define PIC14MIDRANGE_HAS_PSP 0
#define PIC14MIDRANGE_HAS_PORTC 1
#define PIC14MIDRANGE_HAS_PORTD PIC16F88X_FAMILY_HAS_PORTD
#define PIC14MIDRANGE_HAS_PORTE PIC16F88X_FAMILY_HAS_PORTE
#define PIC14MIDRANGE_HAS_COMP_DUAL 1
#define PIC14MIDRANGE_HAS_CM_PIR1 0
#define PIC14MIDRANGE_HAS_EE_PIR1 0
#define PIC14MIDRANGE_HAS_CMCON_BANK0 0
#define PIC14MIDRANGE_HAS_EEPROM_BANK1 0
#define PIC14MIDRANGE_HAS_BRGH16 1
#define PIC14MIDRANGE_HAS_TMR1_GATE 1
#define PIC14MIDRANGE_HAS_ECCP 1
#define PIC14MIDRANGE_HAS_VRSS 1
#define PIC14MIDRANGE_HAS_WDT_SW 1
#define PIC14MIDRANGE_HAS_ANSEL 1
#define PIC14MIDRANGE_HAS_OSCCON 1
#define PIC14MIDRANGE_HAS_BCL_DISPATCH 0
#define PIC14MIDRANGE_HAS_VRCON 1
#define PIC14MIDRANGE_HAS_VRSS 1
#endif /* PIC14_MIDRANGE_H */
