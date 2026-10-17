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

/* PPSLOCK bit 0 (PPSLOCKED): set locks the PPS registers against writes. */
#define PIC_PPSLOCK_PPSLOCKED   EPIC_BIT(0)

#endif /* PIC16F188XX_SFR_H */
