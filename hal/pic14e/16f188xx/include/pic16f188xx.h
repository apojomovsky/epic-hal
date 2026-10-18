/**
 * PIC16F188xx HAL umbrella: device select, port widths, and the SFR map.
 * Family headers include this; consumers use epic_hal.h. DS40001802 is
 * the datasheet; its PDF is bot-walled against scripted fetch, so every
 * address and mask below names the pinned DFP EDC as ground truth.
 */
#ifndef PIC16F188XX_H
#define PIC16F188XX_H

#include "core/hal_status.h"
#include "pic16f188xx_platform.h"

#if !defined(PIC16F18854) && !defined(PIC16F18855) && \
    !defined(PIC16F18856) && !defined(PIC16F18857) && \
    !defined(PIC16F18875) && !defined(PIC16F18876) && \
    !defined(PIC16F18877)
#define PIC16F18877   1
#endif

#if defined(PIC16F18854) + defined(PIC16F18855) + \
    defined(PIC16F18856) + defined(PIC16F18857) + \
    defined(PIC16F18875) + defined(PIC16F18876) + \
    defined(PIC16F18877) > 1
#error "Define exactly one of PIC16F18854/55/56/57/75/76/77."
#endif

/* All seven parts share one SFR map (each EDC is cloned from a sibling);
 * the 40-pin parts add PORTD and widen PORTE. HAS_PORTD/HAS_PORTE follow
 * the PIC16F193X precedent: the GPIO dispatch and the sfr header gate
 * every PORTD/PORTE access on them. */
#if defined(PIC16F18875) || defined(PIC16F18876) || defined(PIC16F18877)
  #define PIC16F188XX_FAMILY_HAS_PORTD  1
  #define PIC16F188XX_FAMILY_HAS_PORTE  1
#else
  #define PIC16F188XX_FAMILY_HAS_PORTD  0
  #define PIC16F188XX_FAMILY_HAS_PORTE  0
#endif

#if   defined(PIC16F18854)
  #define PIC16F188XX_DEVICE_NAME       "PIC16F18854"
#elif defined(PIC16F18855)
  #define PIC16F188XX_DEVICE_NAME       "PIC16F18855"
#elif defined(PIC16F18856)
  #define PIC16F188XX_DEVICE_NAME       "PIC16F18856"
#elif defined(PIC16F18857)
  #define PIC16F188XX_DEVICE_NAME       "PIC16F18857"
#elif defined(PIC16F18875)
  #define PIC16F188XX_DEVICE_NAME       "PIC16F18875"
#elif defined(PIC16F18876)
  #define PIC16F188XX_DEVICE_NAME       "PIC16F18876"
#else  /* PIC16F18877 */
  #define PIC16F188XX_DEVICE_NAME       "PIC16F18877"
#endif

/* Banked-SFR spelling: the DFP spells every 0xFxx-banked SFR with a
 * 0x1Fxx base on 16F18857/16F18877 (same low 12 bits; XC8's own proc
 * headers, the EDCs, and MPLAB SIM agree per part). The sfr header
 * selects the spelling on this macro. */
#if defined(PIC16F18857) || defined(PIC16F18877)
  #define PIC16F188XX_FAMILY_BANKED_HI  1
#else
  #define PIC16F188XX_FAMILY_BANKED_HI  0
#endif

/* Implemented pins per port, from the pinned DFP EDCs. PORTA..PORTC
 * report impl 0xff (PORTC reads 0xc0 but all eight RC fields exist,
 * so the 0xFF mask stands); RA6/RA7 need FEXTOSC=OFF for digital use.
 * PORTD is fully implemented on the 40-pin parts (impl 0xff). PORTE
 * is RE0..RE2 plus input-only RE3 (MCLR): TRISE/LATE/ANSELE report
 * impl 0x7, so the driver covers the three driven pins only.
 * PORTE/IOCE exist on every part for RE3. */
#define PIC16F188XX_FAMILY_PORTA_MASK 0xFFU
#define PIC16F188XX_FAMILY_LATA_MASK  0xFFU
#define PIC16F188XX_FAMILY_PORTB_MASK 0xFFU
#define PIC16F188XX_FAMILY_PORTC_MASK 0xFFU
#define PIC16F188XX_FAMILY_PORTD_MASK 0xFFU
#define PIC16F188XX_FAMILY_PORTE_MASK 0x07U
/* IOCxP/IOCxN/IOCxF are implemented for every pin of PORTA..PORTC.
 * PORTD has no IOC registers; PORTE IOC is RE3 only, which has no
 * driver, so neither port arms edges. */
#define PIC16F188XX_FAMILY_IOCA_MASK  0xFFU
#define PIC16F188XX_FAMILY_IOCB_MASK  0xFFU
#define PIC16F188XX_FAMILY_IOCC_MASK  0xFFU
#define PIC16F188XX_FAMILY_IOCD_MASK  0x00U
#define PIC16F188XX_FAMILY_IOCE_MASK  0x00U
/* Timer0, Timer1, Timer2 and Timer4 have drivers; demos gate timer use on these
 * so the same demo source builds across the PIC14E families. */
#define PIC16F188XX_HAS_TMR0 1
#define PIC16F188XX_HAS_TMR1 1
#define PIC16F188XX_HAS_TMR2 1
#define PIC16F188XX_HAS_TMR4 1
/* One EUSART, one MSSP (MSSP1) and the PPS selector are driven; the
 * second MSSP instance stays unported. */
#define PIC16F188XX_HAS_USART 1
#define PIC16F188XX_HAS_SSP   1
#define PIC16F188XX_HAS_PPS   1
/* CCP1..CCP5 and the dedicated PWM6/PWM7 modules are driven. */
#define PIC16F188XX_HAS_CCP   1
#define PIC16F188XX_HAS_PWM   1

#include "pic16f188xx_sfr.h"

#endif
