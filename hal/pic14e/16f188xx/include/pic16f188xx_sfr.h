/**
 * SFR address map for the PIC16F188xx family, from the pinned
 * Microchip.PIC16F1xxxx_DFP 1.32.471 EDC (PIC16F18855.PIC) and proc
 * header (pic16f18855.h). DS40001802's PDF is bot-walled against
 * scripted fetch, so every fact below names the DFP as ground truth.
 * Bank 0 is shared; banked SFRs spell 0xFxx, or 0x1Fxx on
 * 16F18857/16F18877 (same low 12 bits, selected per part).
 */

#ifndef PIC16F188XX_SFR_H
#define PIC16F188XX_SFR_H

#include "pic16f188xx.h"

/* Core and interrupt control. INTCON carries only INTEDG, PEIE and GIE
 * (impl 0xC1); the peripheral flags and enables live in PIR0/PIE0. */
#define PIC_REG_INTCON        0x0BU
#define PIC_REG_PIR0          0x70CU
#define PIC_REG_PIE0          0x716U

/* GPIO bank 0 (PORTx/TRISx/LATx): same address on every part. */
#define PIC_REG_PORTA         0x0CU
#define PIC_REG_PORTB         0x0DU
#define PIC_REG_PORTC         0x0EU
#define PIC_REG_TRISA         0x11U
#define PIC_REG_TRISB         0x12U
#define PIC_REG_TRISC         0x13U
#define PIC_REG_LATA          0x16U
#define PIC_REG_LATB          0x17U
#define PIC_REG_LATC          0x18U
#if PIC16F188XX_FAMILY_HAS_PORTD
#define PIC_REG_PORTD         0x0FU
#define PIC_REG_TRISD         0x14U
#define PIC_REG_LATD          0x19U
#endif
/* PORTE/TRISE/LATE are bank 0 on every part (RE3 keeps no driver). */
#define PIC_REG_PORTE         0x10U
#define PIC_REG_TRISE         0x15U
#define PIC_REG_LATE          0x1AU
/* Timer0 bank 0 (TMR0L/TMR0H/T0CON0/T0CON1): same address on every part. */
#define PIC_REG_TMR0L         0x1CU
#define PIC_REG_TMR0H         0x1DU
#define PIC_REG_T0CON0        0x1EU
#define PIC_REG_T0CON1        0x1FU

/* Banked GPIO (ANSELx/WPUx/ODCONx/SLRCONx/INLVLx/IOCx): 0xFxx on five
 * parts, 0x1Fxx on 16F18857/16F18877 (same low 12 bits; selected on
 * PIC16F188XX_FAMILY_BANKED_HI). gen-sfr manages the canonical
 * spelling only: lines marked P57-77 keep their value, verified
 * against those parts' DFPs by the sfr-map audit's BANK_VARIANT_ADDRS.
 * PORTD's banked block is 40-pin only; PORTE's banked block exists on
 * every part for RE3. */
#if PIC16F188XX_FAMILY_BANKED_HI
#define PIC_REG_ANSELA        0x1F38U  /* P57-77 */
#define PIC_REG_WPUA          0x1F39U  /* P57-77 */
#define PIC_REG_ODCONA        0x1F3AU  /* P57-77 */
#define PIC_REG_SLRCONA       0x1F3BU  /* P57-77 */
#define PIC_REG_INLVLA        0x1F3CU  /* P57-77 */
#define PIC_REG_IOCAP         0x1F3DU  /* P57-77 */
#define PIC_REG_IOCAN         0x1F3EU  /* P57-77 */
#define PIC_REG_IOCAF         0x1F3FU  /* P57-77 */
#define PIC_REG_ANSELB        0x1F43U  /* P57-77 */
#define PIC_REG_WPUB          0x1F44U  /* P57-77 */
#define PIC_REG_ODCONB        0x1F45U  /* P57-77 */
#define PIC_REG_SLRCONB       0x1F46U  /* P57-77 */
#define PIC_REG_INLVLB        0x1F47U  /* P57-77 */
#define PIC_REG_IOCBP         0x1F48U  /* P57-77 */
#define PIC_REG_IOCBN         0x1F49U  /* P57-77 */
#define PIC_REG_IOCBF         0x1F4AU  /* P57-77 */
#define PIC_REG_ANSELC        0x1F4EU  /* P57-77 */
#define PIC_REG_WPUC          0x1F4FU  /* P57-77 */
#define PIC_REG_ODCONC        0x1F50U  /* P57-77 */
#define PIC_REG_SLRCONC       0x1F51U  /* P57-77 */
#define PIC_REG_INLVLC        0x1F52U  /* P57-77 */
#define PIC_REG_IOCCP         0x1F53U  /* P57-77 */
#define PIC_REG_IOCCN         0x1F54U  /* P57-77 */
#define PIC_REG_IOCCF         0x1F55U  /* P57-77 */
#define PIC_REG_WPUE          0x1F65U  /* P57-77 */
#define PIC_REG_INLVLE        0x1F68U  /* P57-77 */
#define PIC_REG_IOCEP         0x1F69U  /* P57-77 */
#define PIC_REG_IOCEN         0x1F6AU  /* P57-77 */
#define PIC_REG_IOCEF         0x1F6BU  /* P57-77 */
#else
#define PIC_REG_ANSELA        0xF38U
#define PIC_REG_WPUA          0xF39U
#define PIC_REG_ODCONA        0xF3AU
#define PIC_REG_SLRCONA       0xF3BU
#define PIC_REG_INLVLA        0xF3CU
#define PIC_REG_IOCAP         0xF3DU
#define PIC_REG_IOCAN         0xF3EU
#define PIC_REG_IOCAF         0xF3FU
#define PIC_REG_ANSELB        0xF43U
#define PIC_REG_WPUB          0xF44U
#define PIC_REG_ODCONB        0xF45U
#define PIC_REG_SLRCONB       0xF46U
#define PIC_REG_INLVLB        0xF47U
#define PIC_REG_IOCBP         0xF48U
#define PIC_REG_IOCBN         0xF49U
#define PIC_REG_IOCBF         0xF4AU
#define PIC_REG_ANSELC        0xF4EU
#define PIC_REG_WPUC          0xF4FU
#define PIC_REG_ODCONC        0xF50U
#define PIC_REG_SLRCONC       0xF51U
#define PIC_REG_INLVLC        0xF52U
#define PIC_REG_IOCCP         0xF53U
#define PIC_REG_IOCCN         0xF54U
#define PIC_REG_IOCCF         0xF55U
#define PIC_REG_WPUE          0xF65U
#define PIC_REG_INLVLE        0xF68U
#define PIC_REG_IOCEP         0xF69U
#define PIC_REG_IOCEN         0xF6AU
#define PIC_REG_IOCEF         0xF6BU
#endif
#if PIC16F188XX_FAMILY_HAS_PORTD
#if PIC16F188XX_FAMILY_BANKED_HI
#define PIC_REG_ANSELD        0x1F59U  /* P57-77 */
#define PIC_REG_WPUD          0x1F5AU  /* P57-77 */
#define PIC_REG_ODCOND        0x1F5BU  /* P57-77 */
#define PIC_REG_SLRCOND       0x1F5CU  /* P57-77 */
#define PIC_REG_INLVLD        0x1F5DU  /* P57-77 */
#else
#define PIC_REG_ANSELD        0xF59U
#define PIC_REG_WPUD          0xF5AU
#define PIC_REG_ODCOND        0xF5BU
#define PIC_REG_SLRCOND       0xF5CU
#define PIC_REG_INLVLD        0xF5DU
#endif
#endif
#if PIC16F188XX_FAMILY_HAS_PORTE
#if PIC16F188XX_FAMILY_BANKED_HI
#define PIC_REG_ANSELE        0x1F64U  /* P57-77 */
#define PIC_REG_ODCONE        0x1F66U  /* P57-77 */
#define PIC_REG_SLRCONE       0x1F67U  /* P57-77 */
#else
#define PIC_REG_ANSELE        0xF64U
#define PIC_REG_ODCONE        0xF66U
#define PIC_REG_SLRCONE       0xF67U
#endif
#endif

/* INTCON bits: INTEDG bit 0, PEIE bit 6, GIE bit 7. */
#define PIC_INTCON_INTEDG       EPIC_BIT(0)
#define PIC_INTCON_PEIE         EPIC_BIT(6)
#define PIC_INTCON_GIE          EPIC_BIT(7)

/* PIR0/PIE0 carry the INT, IOC and TMR0 flags and enables in bits 0, 4
 * and 5; the other bits are unimplemented (impl 0x31). */
#define PIC_PIR0_INTF         EPIC_BIT(0)
#define PIC_PIR0_IOCIF        EPIC_BIT(4)
#define PIC_PIR0_TMR0IF       EPIC_BIT(5)
#define PIC_PIE0_INTE         EPIC_BIT(0)
#define PIC_PIE0_IOCIE        EPIC_BIT(4)
#define PIC_PIE0_TMR0IE       EPIC_BIT(5)
/* T0CON0 bits: T0EN bit 7, T0OUT bit 5 (read-only), T016BIT bit 4,
 * T0OUTPS value in bits 3:0 (bit 6 unimplemented, impl 0xBF). */
#define PIC_T0CON0_T0EN         EPIC_BIT(7)
#define PIC_T0CON0_T0OUT        EPIC_BIT(5)
#define PIC_T0CON0_T016BIT      EPIC_BIT(4)
#define PIC_T0CON0_T0OUTPS_MASK 0x0FU

/* T0CON1 bits: T0CS value in bits 7:5, T0ASYNC bit 4, T0CKPS value in
 * bits 3:0 (impl 0xFF). */
#define PIC_T0CON1_T0CS_SHIFT   5U
#define PIC_T0CON1_T0CS_MASK    0xE0U
#define PIC_T0CON1_T0ASYNC      EPIC_BIT(4)
#define PIC_T0CON1_T0CKPS_MASK  0x0FU

#endif /* PIC16F188XX_SFR_H */
