/* PIC14 mid-range family selectors for the PIC16F818/819 tier: the
 * classic surface minus everything the 18-pin die does not have. Two
 * parts only, and they differ solely in memory size (1/2 KW flash,
 * 128/256 B RAM, 128/256 B EEPROM). Shared pic14-midrange-core code
 * includes this name; each family's copy resolves via the path. */

#ifndef PIC14_MIDRANGE_H
#define PIC14_MIDRANGE_H
#include "pic16f818_819_hal.h"

/* Capability contract for shared pic14-midrange-core code (1 = present).
 * Every macro is defined explicitly: an omitted PIC14MIDRANGE_* reads
 * as 0 and silently compiles the feature out. */
#define PIC14MIDRANGE_FLASH_KW PIC16F818_819_FAMILY_FLASH_KW
#define PIC14MIDRANGE_HAS_PIR1 1
/* PIR2 carries EEIF only (no CCP2/BCL/comparator), so the dispatcher's
 * PIR2 block reduces to the EEPROM row. */
#define PIC14MIDRANGE_HAS_PIR2 1
#define PIC14MIDRANGE_HAS_SSP 1
/* No SSPCON2 and no SSPMSK on this die (DFP-verified), so the shared
 * driver compiles out the I2C-master and address-mask surface and
 * keeps the SPI master/slave path. */
#define PIC14MIDRANGE_HAS_SSPCON2 0
#define PIC14MIDRANGE_HAS_SSPMSK 0
#define PIC14MIDRANGE_HAS_ADC 1
/* ADCON1 carries the 87XA-style PCFG3:PCFG0 reference/mux table plus
 * ADCS2; there is no ANSEL, so pin analog select is PCFG-only. */
#define PIC14MIDRANGE_HAS_ADC_PCFG 1
#define PIC14MIDRANGE_HAS_ADC_10BIT 1
#define PIC14MIDRANGE_HAS_TMR2 1
#define PIC14MIDRANGE_HAS_CCP1 1
#define PIC14MIDRANGE_HAS_CCP2 0
#define PIC14MIDRANGE_HAS_USART 0
#define PIC14MIDRANGE_HAS_PSP 0
#define PIC14MIDRANGE_HAS_PORTC 0
#define PIC14MIDRANGE_HAS_PORTD 0
#define PIC14MIDRANGE_HAS_PORTE 0
/* No comparator and no VREF module anywhere on this die (DS39598F
 * §1.0, DFP-verified): no CMCON, no CVRCON. */
#define PIC14MIDRANGE_HAS_COMP 0
#define PIC14MIDRANGE_HAS_COMP_DUAL 0
#define PIC14MIDRANGE_HAS_CM_PIR1 0
#define PIC14MIDRANGE_HAS_CM_PIR2 0
#define PIC14MIDRANGE_HAS_CMCON_BANK0 0
#define PIC14MIDRANGE_HAS_VRCON 0
#define PIC14MIDRANGE_HAS_VRSS 0
#define PIC14MIDRANGE_HAS_ULPWU 0
#define PIC14MIDRANGE_HAS_OSF 0
/* EEIF is PIR2<4>, EEIE is PIE2<4>, so neither PIR1 nor EECON1 carries
 * the EEPROM event (unlike the 628A and the 83/84 tiers). */
#define PIC14MIDRANGE_HAS_EE_PIR1 0
#define PIC14MIDRANGE_HAS_EE_PIR2 1
/* The default EEPROM placement: data pair plus EEDATH/EEADRH in Bank 2,
 * control pair in Bank 3, as on the 87XA/88X. */
#define PIC14MIDRANGE_HAS_EEPROM_BANK0 0
#define PIC14MIDRANGE_HAS_EEPROM_BANK1 0
#define PIC14MIDRANGE_HAS_BRGH16 0
#define PIC14MIDRANGE_HAS_TMR1_GATE 0
#define PIC14MIDRANGE_HAS_ECCP 0
#define PIC14MIDRANGE_HAS_WDTCON_BANK0 0
#define PIC14MIDRANGE_HAS_WDTCON_BANK1 0
/* No WDTCON register and no OSCCON<SWDTEN>: the WDT is configuration
 * only on this die. */
#define PIC14MIDRANGE_HAS_WDT_SW 0
#define PIC14MIDRANGE_HAS_ANSEL 0
/* OSCCON/OSCTUNE exist (DS39598F Register 4-2 / 4-1), but with IRCF and
 * IOFS only: no SCS, OSTS or SWDTEN. Reserved contract flag; the shared
 * core has no consumer for it. */
#define PIC14MIDRANGE_HAS_OSCCON 1
#define PIC14MIDRANGE_HAS_BCL_DISPATCH 0
/* PCON carries nBOR/nPOR, and the config word has BOREN. */
#define PIC14MIDRANGE_HAS_PCON 1
/* Bank-independent GPR 0x70..0x7F, mirrored into Banks 1/2/3
 * (DS39598F Figure 2-3), the same window the 87XA/88X/7x use, so the
 * ISR scratch pins land at 0x70/0x71. */
#define PIC14MIDRANGE_COMMON_RAM_BASE 0x70
/* PORTA is RA0..RA4 plus RA6/RA7 in INTRC-with-CLKO-off configurations;
 * RA5 is MCLR-only (DS39598F Table 1-2), so it is excluded from the
 * implemented-pin mask, as on the 628A. */
#define PIC14MIDRANGE_PORTA_MASK 0xDFU
#endif /* PIC14_MIDRANGE_H */
