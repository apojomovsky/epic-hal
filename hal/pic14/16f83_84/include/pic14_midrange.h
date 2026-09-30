/* PIC14 mid-range family selectors for the PIC16F83/84/84A tier: no
 * PIR/PIE pair at all (EEIF/EEIE live in INTCON), no PCON (no BOR),
 * no SSP/ADC/CCP/timers 1-2/comparator/VREF/PSP/PORTC/D/E. Shared
 * pic14-midrange-core code includes this name; each family's copy
 * resolves via the path. */

#ifndef PIC14_MIDRANGE_H
#define PIC14_MIDRANGE_H
#include "pic16f83_84_hal.h"

/* Capability contract for shared pic14-midrange-core code (1 = present). */
#define PIC14MIDRANGE_FLASH_KW PIC16F83_84_FAMILY_FLASH_KW
#define PIC14MIDRANGE_HAS_PIR1 0
#define PIC14MIDRANGE_HAS_PIR2 0
#define PIC14MIDRANGE_HAS_SSP 0
#define PIC14MIDRANGE_HAS_SSPMSK 0
#define PIC14MIDRANGE_HAS_ADC 0
#define PIC14MIDRANGE_HAS_ADC_PCFG 0
#define PIC14MIDRANGE_HAS_CCP2 0
#define PIC14MIDRANGE_HAS_PSP 0
#define PIC14MIDRANGE_HAS_PORTC 0
#define PIC14MIDRANGE_HAS_PORTD 0
#define PIC14MIDRANGE_HAS_PORTE 0
#define PIC14MIDRANGE_HAS_COMP 1
#define PIC14MIDRANGE_HAS_COMP_DUAL 0
#define PIC14MIDRANGE_HAS_CM_PIR2 0
#define PIC14MIDRANGE_HAS_ULPWU 0
#define PIC14MIDRANGE_HAS_OSF 0
#define PIC14MIDRANGE_HAS_CM_PIR1 0
#define PIC14MIDRANGE_HAS_EE_PIR1 0
#define PIC14MIDRANGE_HAS_CMCON_BANK0 0
#define PIC14MIDRANGE_HAS_EEPROM_BANK1 0
#define PIC14MIDRANGE_HAS_EEPROM_BANK0 1
#define PIC14MIDRANGE_HAS_BRGH16 0
#define PIC14MIDRANGE_HAS_TMR1_GATE 0
#define PIC14MIDRANGE_HAS_ECCP 0
#define PIC14MIDRANGE_HAS_VRSS 0
#define PIC14MIDRANGE_HAS_WDTCON_BANK0 0
#define PIC14MIDRANGE_HAS_WDT_SW 0
#define PIC14MIDRANGE_HAS_ANSEL 0
#define PIC14MIDRANGE_HAS_OSCCON 0
#define PIC14MIDRANGE_HAS_BCL_DISPATCH 0
#define PIC14MIDRANGE_HAS_VRCON 0
#define PIC14MIDRANGE_HAS_PCON 0
/* Bank-independent GPR (mirrored across banks): 0x40..0x4F on the
 * 84/84A. The 16F83's smaller 36 B map ends at 0x2F, so nothing can
 * pin there; that part is excluded from every smoke for exactly this
 * reason (see the manifest). */
#define PIC14MIDRANGE_COMMON_RAM_BASE 0x40
#define PIC14MIDRANGE_PORTA_MASK 0x1FU
#endif /* PIC14_MIDRANGE_H */
