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
#define PIC_REG_PIR4          0x710U
#define PIC_REG_PIE4          0x71AU

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
/* Timer1 bank 4 (TMR1L/TMR1H/T1CON/T1GCON/T1GATE/T1CLK) and Timer2 bank 5
 * (TMR2/PR2/T2CON/T2HLT/T2CLKCON/T2RST): same address on every part. The
 * EDC names the Timer2 count and period registers T2TMR and T2PR. */
#define PIC_REG_TMR1L         0x20CU
#define PIC_REG_TMR1H         0x20DU
#define PIC_REG_T1CON         0x20EU
#define PIC_REG_T1GCON        0x20FU
#define PIC_REG_T1GATE        0x210U
#define PIC_REG_T1CLK         0x211U
#define PIC_REG_TMR2          0x28CU
#define PIC_REG_PR2           0x28DU
#define PIC_REG_T2CON         0x28EU
#define PIC_REG_T2HLT         0x28FU
#define PIC_REG_T2CLKCON      0x290U
#define PIC_REG_T2RST         0x291U

/* CCP1..CCP5 (CCPRxL/H, CCPxCON, CCPxCAP) in bank 6, CCPTMRS0/1 in bank 4
 * and PWM6/PWM7 (PWMxDCL/H, PWMxCON) in bank 7: same address on every
 * part. PIR6/PIE6 hold the CCP flags and enables. */
#define PIC_REG_PIR6          0x712U
#define PIC_REG_PIE6          0x71CU
#define PIC_REG_CCPTMRS0      0x21EU
#define PIC_REG_CCPTMRS1      0x21FU
#define PIC_REG_CCPR1L        0x30CU
#define PIC_REG_CCPR1H        0x30DU
#define PIC_REG_CCP1CON       0x30EU
#define PIC_REG_CCP1CAP       0x30FU
#define PIC_REG_CCPR2L        0x310U
#define PIC_REG_CCPR2H        0x311U
#define PIC_REG_CCP2CON       0x312U
#define PIC_REG_CCP2CAP       0x313U
#define PIC_REG_CCPR3L        0x314U
#define PIC_REG_CCPR3H        0x315U
#define PIC_REG_CCP3CON       0x316U
#define PIC_REG_CCP3CAP       0x317U
#define PIC_REG_CCPR4L        0x318U
#define PIC_REG_CCPR4H        0x319U
#define PIC_REG_CCP4CON       0x31AU
#define PIC_REG_CCP4CAP       0x31BU
#define PIC_REG_CCPR5L        0x31CU
#define PIC_REG_CCPR5H        0x31DU
#define PIC_REG_CCP5CON       0x31EU
#define PIC_REG_CCP5CAP       0x31FU
#define PIC_REG_PWM6DCL       0x38CU
#define PIC_REG_PWM6DCH       0x38DU
#define PIC_REG_PWM6CON       0x38EU
#define PIC_REG_PWM7DCL       0x390U
#define PIC_REG_PWM7DCH       0x391U
#define PIC_REG_PWM7CON       0x392U

/* PIR3/PIE3 carry the MSSP1 and EUSART flags and enables: same address
 * on every part. */
#define PIC_REG_PIR3          0x70FU
#define PIC_REG_PIE3          0x719U
/* EUSART (RC1REG..BAUD1CON) and MSSP1 (SSP1BUF..SSP1CON3): same address
 * on every part. The 1 suffix names the first instance. */
#define PIC_REG_RC1REG        0x119U
#define PIC_REG_TX1REG        0x11AU
#define PIC_REG_SP1BRGL       0x11BU
#define PIC_REG_SP1BRGH       0x11CU
#define PIC_REG_RC1STA        0x11DU
#define PIC_REG_TX1STA        0x11EU
#define PIC_REG_BAUD1CON      0x11FU
#define PIC_REG_SSP1BUF       0x18CU
#define PIC_REG_SSP1ADD       0x18DU
#define PIC_REG_SSP1MSK       0x18EU
#define PIC_REG_SSP1STAT      0x18FU
#define PIC_REG_SSP1CON1      0x190U
#define PIC_REG_SSP1CON2      0x191U
#define PIC_REG_SSP1CON3      0x192U
/* ADC bank 1 (ADRESL/ADRESH/ADCON0..ADPCH): same address on every
 * part. PIR1/PIE1 carry ADIF/ADIE in bit 0, PIR2/PIE2 the C1IF/C1IE
 * and C2IF/C2IE pairs in bits 0 and 1. */
#define PIC_REG_ADRESL        0x8CU
#define PIC_REG_ADRESH        0x8DU
#define PIC_REG_ADCON0        0x93U
#define PIC_REG_ADCON1        0x94U
#define PIC_REG_ADCON2        0x95U
#define PIC_REG_ADCLK         0x98U
#define PIC_REG_ADACT         0x99U
#define PIC_REG_ADREF         0x9AU
#define PIC_REG_ADPCH         0x9EU
#define PIC_REG_PIR1          0x70DU
#define PIC_REG_PIE1          0x717U
#define PIC_REG_PIR2          0x70EU
#define PIC_REG_PIE2          0x718U

/* NCO1 (accumulator NCO1ACCL/H/U, increment NCO1INCL/H/U, NCO1CON,
 * NCO1CLK) and the NVM block (NVMADRL..NVMCON2): same address on every
 * part. NVMCON2 is the write-only 0x55/0xAA unlock register. */
#define PIC_REG_PIR7          0x713U
#define PIC_REG_PIE7          0x71DU
#define PIC_REG_NCO1ACCL      0x58CU
#define PIC_REG_NCO1ACCH      0x58DU
#define PIC_REG_NCO1ACCU      0x58EU
#define PIC_REG_NCO1INCL      0x58FU
#define PIC_REG_NCO1INCH      0x590U
#define PIC_REG_NCO1INCU      0x591U
#define PIC_REG_NCO1CON       0x592U
#define PIC_REG_NCO1CLK       0x593U
#define PIC_REG_NVMADRL       0x81AU
#define PIC_REG_NVMADRH       0x81BU
#define PIC_REG_NVMDATL       0x81CU
#define PIC_REG_NVMDATH       0x81DU
#define PIC_REG_NVMCON1       0x81EU
#define PIC_REG_NVMCON2       0x81FU
/* FVR (FVRCON), DAC1 (DAC1CON0/1, 5-bit) and the comparator pair
 * (CMOUT, CM1CON0/1, CM1NSEL/PSEL, CM2CON0/1, CM2NSEL/PSEL): same
 * address on every part. */
#define PIC_REG_FVRCON        0x90CU
#define PIC_REG_DAC1CON0      0x90EU
#define PIC_REG_DAC1CON1      0x90FU
#define PIC_REG_CMOUT         0x98FU
#define PIC_REG_CM1CON0       0x990U
#define PIC_REG_CM1CON1       0x991U
#define PIC_REG_CM1NSEL       0x992U
#define PIC_REG_CM1PSEL       0x993U
#define PIC_REG_CM2CON0       0x994U
#define PIC_REG_CM2CON1       0x995U
#define PIC_REG_CM2NSEL       0x996U
#define PIC_REG_CM2PSEL       0x997U
/* WDT control (WDTCON0/WDTCON1), power and reset status (PCON0) and the
 * Doze/Idle control (CPUDOZE): same address on every part. */
#define PIC_REG_WDTCON0       0x80CU
#define PIC_REG_WDTCON1       0x80DU
#define PIC_REG_PCON0         0x813U
#define PIC_REG_CPUDOZE       0x88CU

/* PPS (PPSLOCK, xxxPPS input selects, RxyPPS output selects): 0xE8F..
 * and 0xF10.. on five parts, 0x1E8F../0x1F10.. on 16F18857/16F18877
 * (same low 12 bits; selected on PIC16F188XX_FAMILY_BANKED_HI, lines
 * marked P57-77 as for the banked GPIO block). The RxyPPS selects are
 * eight consecutive registers per port (RA0PPS..RA7PPS from the base);
 * PORTD/PORTE exist on the 40-pin parts only. */
#if PIC16F188XX_FAMILY_BANKED_HI
#define PIC_REG_PPSLOCK       0x1E8FU  /* P57-77 */
#define PIC_REG_T0CKIPPS      0x1E91U  /* P57-77 */
#define PIC_REG_SSP1CLKPPS    0x1EC5U  /* P57-77 */
#define PIC_REG_SSP1DATPPS    0x1EC6U  /* P57-77 */
#define PIC_REG_SSP1SSPPS     0x1EC7U  /* P57-77 */
#define PIC_REG_RXPPS         0x1ECBU  /* P57-77 */
#define PIC_REG_CCP1PPS       0x1EA1U  /* P57-77 */
#define PIC_REG_CCP2PPS       0x1EA2U  /* P57-77 */
#define PIC_REG_CCP3PPS       0x1EA3U  /* P57-77 */
#define PIC_REG_CCP4PPS       0x1EA4U  /* P57-77 */
#define PIC_REG_CCP5PPS       0x1EA5U  /* P57-77 */
#define PIC_REG_RA0PPS        0x1F10U  /* P57-77 */
#define PIC_REG_RB0PPS        0x1F18U  /* P57-77 */
#define PIC_REG_RC0PPS        0x1F20U  /* P57-77 */
#if PIC16F188XX_FAMILY_HAS_PORTD
#define PIC_REG_RD0PPS        0x1F28U  /* P57-77 */
#endif
#else
#define PIC_REG_PPSLOCK       0xE8FU
#define PIC_REG_T0CKIPPS      0xE91U
#define PIC_REG_SSP1CLKPPS    0xEC5U
#define PIC_REG_SSP1DATPPS    0xEC6U
#define PIC_REG_SSP1SSPPS     0xEC7U
#define PIC_REG_RXPPS         0xECBU
#define PIC_REG_CCP1PPS       0xEA1U
#define PIC_REG_CCP2PPS       0xEA2U
#define PIC_REG_CCP3PPS       0xEA3U
#define PIC_REG_CCP4PPS       0xEA4U
#define PIC_REG_CCP5PPS       0xEA5U
#define PIC_REG_RA0PPS        0xF10U
#define PIC_REG_RB0PPS        0xF18U
#define PIC_REG_RC0PPS        0xF20U
#if PIC16F188XX_FAMILY_HAS_PORTD
#define PIC_REG_RD0PPS        0xF28U
#endif
#endif

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

/* PIR4/PIE4 carry the Timer1 and Timer2 flags and enables in bits 0 and 1
 * (impl 0x3F, bits 7:6 unimplemented). */
#define PIC_PIR4_TMR1IF       EPIC_BIT(0)
#define PIC_PIR4_TMR2IF       EPIC_BIT(1)
#define PIC_PIE4_TMR1IE       EPIC_BIT(0)
#define PIC_PIE4_TMR2IE       EPIC_BIT(1)

/* T1CON bits: ON bit 0, RD16 bit 1, SYNC bit 2 (set: external input not
 * synchronized), CKPS value in bits 5:4 (impl 0x37). */
#define PIC_T1CON_ON            EPIC_BIT(0)
#define PIC_T1CON_RD16          EPIC_BIT(1)
#define PIC_T1CON_nSYNC         EPIC_BIT(2)
#define PIC_T1CON_CKPS_SHIFT    4U
#define PIC_T1CON_CKPS_MASK     0x30U

/* T1GCON bits: GE bit 7, GPOL bit 6, GTM bit 5, GSPM bit 4, GGO/DONE
 * bit 3 (cleared by hardware), GVAL bit 2 (read-only); impl 0xFC. */
#define PIC_T1GCON_GE           EPIC_BIT(7)
#define PIC_T1GCON_GPOL         EPIC_BIT(6)
#define PIC_T1GCON_GTM          EPIC_BIT(5)
#define PIC_T1GCON_GSPM         EPIC_BIT(4)
#define PIC_T1GCON_GGO_nDONE  EPIC_BIT(3)
#define PIC_T1GCON_GVAL         EPIC_BIT(2)

/* T1GATE GSS value in bits 4:0 (impl 0x1F); T1CLK CS value in bits 3:0
 * (impl 0x0F). */
#define PIC_T1GATE_GSS_MASK     0x1FU
#define PIC_T1CLK_CS_MASK       0x0FU

/* T2CON bits: ON bit 7, CKPS value in bits 6:4, OUTPS value in bits 3:0
 * (impl 0xFF). */
#define PIC_T2CON_ON            EPIC_BIT(7)
#define PIC_T2CON_CKPS_SHIFT    4U
#define PIC_T2CON_CKPS_MASK     0x70U
#define PIC_T2CON_OUTPS_MASK    0x0FU

/* T2HLT bits: PSYNC bit 7, CKPOL bit 6, CKSYNC bit 5, MODE value in
 * bits 4:0 (impl 0xFF). T2CLKCON CS value in bits 3:0 (impl 0x0F);
 * T2RST RSEL value in bits 4:0 (impl 0x1F). */
#define PIC_T2HLT_PSYNC         EPIC_BIT(7)
#define PIC_T2HLT_CKPOL         EPIC_BIT(6)
#define PIC_T2HLT_CKSYNC        EPIC_BIT(5)
#define PIC_T2HLT_MODE_MASK     0x1FU
#define PIC_T2CLKCON_CS_MASK    0x0FU
#define PIC_T2RST_RSEL_MASK     0x1FU
/* PIR3/PIE3 bits: SSP1 event bit 0, SSP1 bus collision bit 1, EUSART TX
 * bit 4, EUSART RX bit 5 (impl 0x3F; bits 2 and 3 are the MSSP2 pair,
 * unused here). TXIF and RCIF are read-only status: hardware clears
 * them (TXREG write, RCREG read). */
#define PIC_PIR3_SSP1IF         EPIC_BIT(0)
#define PIC_PIR3_BCL1IF         EPIC_BIT(1)
#define PIC_PIR3_TXIF           EPIC_BIT(4)
#define PIC_PIR3_RCIF           EPIC_BIT(5)
#define PIC_PIE3_SSP1IE         EPIC_BIT(0)
#define PIC_PIE3_BCL1IE         EPIC_BIT(1)
#define PIC_PIE3_TXIE           EPIC_BIT(4)
#define PIC_PIE3_RCIE           EPIC_BIT(5)

/* RC1STA bits: SPEN 7, RX9 6, SREN 5, CREN 4, ADDEN 3, FERR 2 (read-only),
 * OERR 1 (read-only, clears when CREN is cleared), RX9D 0 (read-only). */
#define PIC_RC1STA_SPEN         EPIC_BIT(7)
#define PIC_RC1STA_RX9          EPIC_BIT(6)
#define PIC_RC1STA_CREN         EPIC_BIT(4)
#define PIC_RC1STA_ADDEN        EPIC_BIT(3)
#define PIC_RC1STA_FERR         EPIC_BIT(2)
#define PIC_RC1STA_OERR         EPIC_BIT(1)
#define PIC_RC1STA_RX9D         EPIC_BIT(0)

/* TX1STA bits: CSRC 7, TX9 6, TXEN 5, SYNC 4, SENDB 3, BRGH 2, TRMT 1
 * (read-only, set when the shift register is empty), TX9D 0. POR 0x02. */
#define PIC_TX1STA_TX9          EPIC_BIT(6)
#define PIC_TX1STA_TXEN         EPIC_BIT(5)
#define PIC_TX1STA_SYNC         EPIC_BIT(4)
#define PIC_TX1STA_BRGH         EPIC_BIT(2)
#define PIC_TX1STA_TRMT         EPIC_BIT(1)
#define PIC_TX1STA_TX9D         EPIC_BIT(0)
#define PIC_TX1STA_POR_VALUE    0x02U

/* BAUD1CON bits: ABDOVF 7 (read-only), RCIDL 6 (read-only, set while the
 * receiver is idle), SCKP 4, BRG16 3, WUE 1, ABDEN 0 (impl 0xDB, POR 0x40). */
#define PIC_BAUD1CON_RCIDL      EPIC_BIT(6)
#define PIC_BAUD1CON_BRG16      EPIC_BIT(3)

/* SSP1STAT bits: SMP 7, CKE 6, DA 5, P 4, S 3, RW 2, UA 1, BF 0. Only SMP
 * and CKE are software-writable; the rest are hardware status. */
#define PIC_SSP1STAT_SMP        EPIC_BIT(7)
#define PIC_SSP1STAT_CKE        EPIC_BIT(6)
#define PIC_SSP1STAT_DA         EPIC_BIT(5)
#define PIC_SSP1STAT_P          EPIC_BIT(4)
#define PIC_SSP1STAT_S          EPIC_BIT(3)
#define PIC_SSP1STAT_RW         EPIC_BIT(2)
#define PIC_SSP1STAT_BF         EPIC_BIT(0)
#define PIC_SSP1STAT_WRITE_MASK 0xC0U

/* SSP1CON1 bits: WCOL 7, SSPOV 6, SSPEN 5, CKP 4, SSPM value in bits 3:0. */
#define PIC_SSP1CON1_WCOL       EPIC_BIT(7)
#define PIC_SSP1CON1_SSPOV      EPIC_BIT(6)
#define PIC_SSP1CON1_SSPEN      EPIC_BIT(5)
#define PIC_SSP1CON1_CKP        EPIC_BIT(4)
#define PIC_SSP1CON1_SSPM_MASK  0x0FU

/* SSP1CON2 bits (I2C master): GCEN 7, ACKSTAT 6 (read-only, 0 = slave
 * acknowledged), ACKDT 5, ACKEN 4, RCEN 3, PEN 2, RSEN 1, SEN 0. The five
 * low bits are bus operations the hardware clears on completion. */
#define PIC_SSP1CON2_ACKSTAT    EPIC_BIT(6)
#define PIC_SSP1CON2_ACKDT      EPIC_BIT(5)
#define PIC_SSP1CON2_ACKEN      EPIC_BIT(4)
#define PIC_SSP1CON2_RCEN       EPIC_BIT(3)
#define PIC_SSP1CON2_PEN        EPIC_BIT(2)
#define PIC_SSP1CON2_RSEN       EPIC_BIT(1)
#define PIC_SSP1CON2_SEN        EPIC_BIT(0)
#define PIC_SSP1CON2_OP_MASK    0x1FU
/* ADCON0 bits (DS40001802D Register 23-1): ADON 7, ADCONT 6, ADCS 4
 * (1: FRC, 0: FOSC through ADCLK), ADFM 2 (1: right justified), ADGO
 * 0 (set to start, hardware clears). Bit 3 is unimplemented on the
 * datasheet diagram, so only the documented format bit is named. */
#define PIC_ADCON0_ADON         EPIC_BIT(7)
#define PIC_ADCON0_ADCONT       EPIC_BIT(6)
#define PIC_ADCON0_ADCS         EPIC_BIT(4)
#define PIC_ADCON0_ADFM         EPIC_BIT(2)
#define PIC_ADCON0_GO_NDONE     EPIC_BIT(0)
/* ADPCH channel field in bits 5:0, ADCLK divider in bits 5:0, ADACT
 * trigger select in bits 4:0 (DS40001802D Table 23-2). */
#define PIC_ADPCH_CHS_MASK      0x3FU
#define PIC_ADCLK_ADCCS_MASK    0x3FU
#define PIC_ADACT_ADACT_MASK    0x1FU
/* ADREF bits (Register 23-7): ADPREF value in bits 1:0
 * (00 VDD, 10 VREF+ pin, 11 FVR buffer 1), ADNREF bit 4. */
#define PIC_ADREF_ADPREF_MASK   0x03U
#define PIC_ADREF_ADNREF        EPIC_BIT(4)
/* PIR1/PIE1 carry ADIF/ADIE in bit 0; PIR2/PIE2 the C1 and C2 pairs
 * in bits 0 and 1. */
#define PIC_PIR1_ADIF           EPIC_BIT(0)
#define PIC_PIE1_ADIE           EPIC_BIT(0)
#define PIC_PIR2_C1IF           EPIC_BIT(0)
#define PIC_PIR2_C2IF           EPIC_BIT(1)
#define PIC_PIE2_C1IE           EPIC_BIT(0)
#define PIC_PIE2_C2IE           EPIC_BIT(1)
/* FVRCON bits (Register 16-1): FVREN 7, FVRRDY 6 (read-only, always 1
 * on 16F18855/75 once enabled), ADFVR value in bits 1:0, CDAFVR value
 * in bits 3:2 (00 off, 01 1.024V, 10 2.048V, 11 4.096V). */
#define PIC_FVRCON_FVREN        EPIC_BIT(7)
#define PIC_FVRCON_FVRRDY       EPIC_BIT(6)
#define PIC_FVRCON_CDAFVR_SHIFT 2U
#define PIC_FVRCON_CDAFVR_MASK  0x0CU
#define PIC_FVRCON_ADFVR_MASK   0x03U
/* DAC1CON0 bits (Register 25-1): EN 7, OE1 5 (DAC1OUT1 pin, RA2), OE2
 * 4 (DAC1OUT2 pin, RB7), PSS value in bits 3:2 (00 VDD, 01 VREF+ pin,
 * 10 FVR), NSS 0 (1: VREF- pin, 0: VSS). DAC1R value in bits 4:0. */
#define PIC_DAC1CON0_EN         EPIC_BIT(7)
#define PIC_DAC1CON0_OE1        EPIC_BIT(5)
#define PIC_DAC1CON0_OE2        EPIC_BIT(4)
#define PIC_DAC1CON0_PSS_SHIFT  2U
#define PIC_DAC1CON0_PSS_MASK   0x0CU
#define PIC_DAC1CON0_NSS        EPIC_BIT(0)
#define PIC_DAC1CON1_DAC1R_MASK 0x1FU
/* CMxCON0 bits (Register 18-1): ON 7, OUT 6 (read-only), POL 4, HYS 1,
 * SYNC 0. CMxCON1 (Register 18-2): INTP 1, INTN 0. CMxNSEL (Register
 * 18-3): NCH value in bits 2:0 (0..3 pins, 6 FVR buffer 2, 7 AVSS).
 * CMxPSEL (Register 18-4): PCH value in bits 2:0 (0..1 pins, 5 DAC,
 * 6 FVR buffer 2, 7 AVSS). CMOUT (Register 18-5): MCxOUT mirrors.
 * CM1CON0/CM1CON1 names carry both instances (one comparator module in
 * the EDC, positions identical, the audit checks them once). */
#define PIC_CM1CON0_ON          EPIC_BIT(7)
#define PIC_CM1CON0_OUT         EPIC_BIT(6)
#define PIC_CM1CON0_POL         EPIC_BIT(4)
#define PIC_CM1CON0_HYS         EPIC_BIT(1)
#define PIC_CM1CON1_INTP        EPIC_BIT(1)
#define PIC_CM1CON1_INTN        EPIC_BIT(0)
#define PIC_CM1NSEL_NCH_MASK    0x07U
#define PIC_CM1PSEL_PCH_MASK    0x07U
#define PIC_CM2CON0_ON          EPIC_BIT(7)
#define PIC_CM2CON0_OUT         EPIC_BIT(6)
#define PIC_CM2CON0_POL         EPIC_BIT(4)
#define PIC_CM2CON0_HYS         EPIC_BIT(1)
#define PIC_CM2CON1_INTP        EPIC_BIT(1)
#define PIC_CM2CON1_INTN        EPIC_BIT(0)
#define PIC_CM2NSEL_NCH_MASK    0x07U
#define PIC_CM2PSEL_PCH_MASK    0x07U
#define PIC_CMOUT_MC1OUT        EPIC_BIT(0)
#define PIC_CMOUT_MC2OUT        EPIC_BIT(1)

/* PPSLOCK bit 0 (PPSLOCKED): set locks the PPS registers against writes. */
#define PIC_PPSLOCK_PPSLOCKED   EPIC_BIT(0)

/* PIR7/PIE7 bits: NCO1 overflow bit 4, NVM operation done bit 5 (impl
 * 0xF7; the other bits belong to CWG, CRC and the scanner, unused here).
 * NVMIF is set by hardware when an NVM write or erase completes and must
 * be cleared by software. */
#define PIC_PIR7_NCO1IF         EPIC_BIT(4)
#define PIC_PIR7_NVMIF          EPIC_BIT(5)
#define PIC_PIE7_NCO1IE         EPIC_BIT(4)
#define PIC_PIE7_NVMIE          EPIC_BIT(5)

/* NVMCON1 bits (DS40001802D Register 10-5): NVMREGS 6, LWLO 5, FREE 4,
 * WRERR 3, WREN 2, WR 1, RD 0 (impl 0x7F). NVMREGS=1 selects the data
 * EEPROM, user ID and configuration space. WR and RD can only be set by
 * software; hardware clears them on completion. WRERR is cleared by
 * software only. */
#define PIC_NVMCON1_NVMREGS     EPIC_BIT(6)
#define PIC_NVMCON1_LWLO        EPIC_BIT(5)
#define PIC_NVMCON1_FREE        EPIC_BIT(4)
#define PIC_NVMCON1_WRERR       EPIC_BIT(3)
#define PIC_NVMCON1_WREN        EPIC_BIT(2)
#define PIC_NVMCON1_WR          EPIC_BIT(1)
#define PIC_NVMCON1_RD          EPIC_BIT(0)

/* WDTCON0 (DS40001802D Register 9-1): WDTPS value in bits 5:1, SWDTEN
 * bit 0 (the datasheet's SEN; impl 0x3F). WDTPS is writable only while
 * CONFIG3 WDTCPS=11111. SWDTEN is honoured only with WDTE=01. */
#define PIC_WDTCON0_WDTPS_SHIFT 1U
#define PIC_WDTCON0_WDTPS_MASK  0x3EU
#define PIC_WDTCON0_SWDTEN      EPIC_BIT(0)

/* PCON0 bits (DS40001802D Register 5-2), all active low except the stack
 * flags: STKOVF 7, STKUNF 6, WDTWV 5, RWDT 4, RMCLR 3, RI 2, POR 1,
 * BOR 0. Hardware clears POR/BOR on the event; software sets them back. */
#define PIC_PCON0_nPOR          EPIC_BIT(1)
#define PIC_PCON0_nBOR          EPIC_BIT(0)

/* CPUDOZE bit 7 (IDLEN, DS40001802D Register 8-2): with it set, SLEEP
 * stops the CPU clock only (Idle); clear, SLEEP is full Sleep. */
#define PIC_CPUDOZE_IDLEN       EPIC_BIT(7)

/* NCO1CON bits (DS40001802D Register 24-1): N1EN 7, N1OUT 5 (read-only
 * live output), N1POL 4, N1PFM 0 (impl 0xB1). */
#define PIC_NCO1CON_N1EN        EPIC_BIT(7)
#define PIC_NCO1CON_N1OUT       EPIC_BIT(5)
#define PIC_NCO1CON_N1POL       EPIC_BIT(4)
#define PIC_NCO1CON_N1PFM       EPIC_BIT(0)
#define PIC_NCO1CON_WRITE_MASK  0x91U

/* NCO1CLK (Register 24-2): N1PWS value in bits 7:5, N1CKS value in bits
 * 2:0 (impl 0xE7). */
#define PIC_NCO1CLK_N1PWS_SHIFT 5U
#define PIC_NCO1CLK_N1PWS_MASK  0xE0U
#define PIC_NCO1CLK_N1CKS_MASK  0x07U

/* PIR6/PIE6 carry the CCP1..CCP5 flags and enables in bits 0..4 (impl
 * 0x1F, bits 7:5 unimplemented). */
#define PIC_PIR6_CCP1IF       EPIC_BIT(0)
#define PIC_PIR6_CCP2IF       EPIC_BIT(1)
#define PIC_PIR6_CCP3IF       EPIC_BIT(2)
#define PIC_PIR6_CCP4IF       EPIC_BIT(3)
#define PIC_PIR6_CCP5IF       EPIC_BIT(4)
#define PIC_PIE6_CCP1IE       EPIC_BIT(0)
#define PIC_PIE6_CCP2IE       EPIC_BIT(1)
#define PIC_PIE6_CCP3IE       EPIC_BIT(2)
#define PIC_PIE6_CCP4IE       EPIC_BIT(3)
#define PIC_PIE6_CCP5IE       EPIC_BIT(4)

/* CCPxCON bits: EN bit 7, OUT bit 5 (read-only), FMT bit 4 (PWM duty
 * alignment), MODE value in bits 3:0 (impl 0xBF, bit 6 unimplemented).
 * CCP1..CCP5 share one module (EDC _modsrc DOS-02493_ccp_v4), so the
 * CCP1CON names carry every instance and the audit checks them once. */
#define PIC_CCP1CON_EN          EPIC_BIT(7)
#define PIC_CCP1CON_OUT         EPIC_BIT(5)
#define PIC_CCP1CON_FMT         EPIC_BIT(4)
#define PIC_CCP1CON_MODE_MASK   0x0FU

/* CCPxCAP CTS capture input select in bits 2:0 (impl 0x07). */
#define PIC_CCPCAP_CTS_MASK     0x07U

/* CCPTMRS0 holds C1TSEL..C4TSEL in bit pairs 1:0, 3:2, 5:4 and 7:6;
 * CCPTMRS1 holds C5TSEL (1:0), P6TSEL (3:2) and P7TSEL (5:4). A pair
 * selects the timer couple: 01 Timer1/Timer2, 10 Timer3/Timer4, 11
 * Timer5/Timer6; 00 is reserved. POR 0x55 and 0x15. */
#define PIC_CCPTMRS_TSEL_MASK   0x03U
#define PIC_CCPTMRS0_POR_VALUE  0x55U
#define PIC_CCPTMRS1_POR_VALUE  0x15U

/* PWMxCON bits: EN bit 7, OUT bit 5 (read-only), POL bit 4 (set: active
 * low), impl 0xB0. PWMxDCL carries duty bits 1:0 in bits 7:6 (impl
 * 0xC0), PWMxDCH duty bits 9:2. PWM6 and PWM7 share one module (EDC
 * _modsrc DOS-02719_pwm_10bit_v3_8Bit), so PWM6CON carries both. */
#define PIC_PWM6CON_EN          EPIC_BIT(7)
#define PIC_PWM6CON_OUT         EPIC_BIT(5)
#define PIC_PWM6CON_POL         EPIC_BIT(4)
#define PIC_PWMDCL_DC_SHIFT     6U

#endif /* PIC16F188XX_SFR_H */
