/* PIC14 mid-range family selectors for the PIC16F7x family (72-77 and
 * 737-777): classic mid-range with the full 877A-era surface on the
 * 40-pin parts (USART, CCP1/CCP2, SSP, ADC, PSP), USB-less, no data
 * EEPROM (the PM* program-memory bank replaces it; see the family
 * MANUAL.md). Shared pic14-midrange-core code includes this name; each
 * family's copy resolves via the include path. */

#ifndef PIC14_MIDRANGE_H
#define PIC14_MIDRANGE_H
#include "pic16f7x.h"

/* Capability contract for shared pic14-midrange-core code (1 = present). */
#define PIC14MIDRANGE_FLASH_KW PIC16F7X_FAMILY_FLASH_KW
#define PIC14MIDRANGE_HAS_PIR1 1
#define PIC14MIDRANGE_HAS_PIR2 PIC16F7X_FAMILY_HAS_PIR2
#define PIC14MIDRANGE_HAS_SSP PIC16F7X_FAMILY_HAS_SSP
#define PIC14MIDRANGE_HAS_SSPMSK 0
/* The 16F77 MSSP is SPI-only: it has no SSPCON2 register (DFP-
 * verified), unlike the 87XA/88X. The shared SSP driver compiles out
 * its I2C-master helpers (Start/Stop/ACK/Receive) under this guard. */
#define PIC14MIDRANGE_HAS_SSPCON2 0
#define PIC14MIDRANGE_HAS_ADC 1
#define PIC14MIDRANGE_HAS_ADC_PCFG 0
/* DS30325 (72-77) carries one 8-bit ADRES and no ADCON2; DS30498
 * (737-777) carries the 10-bit ADRESH/ADRESL + ADCON2. The shared ADC
 * driver branches on this to pick the result read path. */
#define PIC14MIDRANGE_HAS_ADC_10BIT PIC16F7X_FAMILY_ADC_10BIT
#define PIC14MIDRANGE_HAS_TMR2 1
#define PIC14MIDRANGE_HAS_CCP1 1
#define PIC14MIDRANGE_HAS_USART PIC16F7X_FAMILY_HAS_USART
#define PIC14MIDRANGE_HAS_CCP2 PIC16F7X_FAMILY_HAS_CCP2
#define PIC14MIDRANGE_HAS_PSP PIC16F7X_FAMILY_HAS_PSP
#define PIC14MIDRANGE_HAS_PORTC PIC16F7X_FAMILY_HAS_PORTC
#define PIC14MIDRANGE_HAS_PORTD PIC16F7X_FAMILY_HAS_PORTD
#define PIC14MIDRANGE_HAS_PORTE PIC16F7X_FAMILY_HAS_PORTE
#define PIC14MIDRANGE_HAS_COMP_DUAL 0
/* The 7x family has no comparator peripheral (no CMCON on any part,
 * DFP-verified), so both comparator dispatch paths compile out. */
#define PIC14MIDRANGE_HAS_CM_PIR2 0
#define PIC14MIDRANGE_HAS_CM_PIR1 0
#define PIC14MIDRANGE_HAS_EE_PIR1 0
#define PIC14MIDRANGE_HAS_EE_PIR2 0
#define PIC14MIDRANGE_HAS_CMCON_BANK0 0
#define PIC14MIDRANGE_HAS_EEPROM_BANK0 0
#define PIC14MIDRANGE_HAS_EEPROM_BANK1 0
/* The 16F77 USART is the classic single-BRGH shape: TXSTA<BRGH> only,
 * no SPBRGH (9-bit BRG) and no BAUDCTL register (DFP-verified). The
 * shared USART driver uses HAS_BRGH16 to decide whether to touch those
 * registers, so it stays 0 here. */
#define PIC14MIDRANGE_HAS_BRGH16 0
#define PIC14MIDRANGE_HAS_TMR1_GATE 0
#define PIC14MIDRANGE_HAS_ECCP 0
#define PIC14MIDRANGE_HAS_VRSS 0
#define PIC14MIDRANGE_HAS_WDTCON_BANK1 0
#define PIC14MIDRANGE_HAS_WDTCON_BANK0 0
#define PIC14MIDRANGE_HAS_WDT_SW 0
#define PIC14MIDRANGE_HAS_ANSEL 0
#define PIC14MIDRANGE_HAS_OSCCON 0
/* The 16F77 PIR2 carries only CCP2IF (DFP-verified); there is no SSP
 * bus-collision flag on this part, unlike the 87XA/88X, so the shared
 * BCL dispatch rows compile out. */
#define PIC14MIDRANGE_HAS_BCL_DISPATCH 0
#define PIC14MIDRANGE_HAS_VRCON 0
#define PIC14MIDRANGE_COMMON_RAM_BASE 0x70
/* PORTA is 6 usable pins (RA0-RA5; RA6/RA7 are the OSCI/OSCO pins on
 * the 7x, not GPIO), same as the 87XA. The shared gpio driver uses
 * this mask to bound drive_input/read_output. */
#define PIC14MIDRANGE_PORTA_MASK 0x3FU
#endif /* PIC14_MIDRANGE_H */
