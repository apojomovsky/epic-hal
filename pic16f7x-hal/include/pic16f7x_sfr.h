/* SFR address map for the PIC16F7x family (DS30325 for the 16F72-77,
 * DS30498 for the 16F737-777). Every address, bit mask and reset value
 * is 1-to-1 from those datasheets' register tables; per-part registers
 * (PORTD/E, PSP, CCP2, USART on the absent 16F72) are guarded by
 * PIC16F7X_FAMILY_HAS_*. This family has NO data EEPROM and NO
 * comparator/Vref peripheral; the Bank-2 program-memory block
 * (PMDATA/PMADR/PMDATH/PMADRH/PMCON1) replaces the EEPROM registers. */

#ifndef PIC16F7X_SFR_H
#define PIC16F7X_SFR_H

#include "pic16f7x.h"

/* Bank 0, core SFRs (DS30325 Table 3-1 / DS30498 Table 3-1). */

/** Indirect address pointer. */
#define PIC_REG_INDF          0x00U
/** Timer0. */
#define PIC_REG_TMR0          0x01U
/** Program Counter low byte. */
#define PIC_REG_PCL           0x02U
/** Status register. */
#define PIC_REG_STATUS        0x03U
/** File Select Register (indirect addressing). */
#define PIC_REG_FSR           0x04U

/* I/O ports. */
#define PIC_REG_PORTA         0x05U
#define PIC_REG_PORTB         0x06U
#define PIC_REG_PORTC         0x07U
#define PIC_REG_PORTD         0x08U   /* 40-pin only. */
#define PIC_REG_PORTE         0x09U   /* 40-pin only. */

#define PIC_REG_TRISA         0x85U
#define PIC_REG_TRISB         0x86U
#define PIC_REG_TRISC         0x87U
#define PIC_REG_TRISD         0x88U   /* 40-pin only. */
#define PIC_REG_TRISE         0x89U   /* 40-pin only. */

/* Core CPU control (DS30325 §11.0 / DS30498 §13). */
#define PIC_REG_PCLATH        0x0AU
#define PIC_REG_INTCON        0x0BU
#define PIC_REG_PIR1          0x0CU
#define PIC_REG_PIR2          0x0DU
#define PIC_REG_PIE1          0x8CU   /* Bank 1. */
#define PIC_REG_PIE2          0x8DU   /* Bank 1. */
#define PIC_REG_PCON          0x8EU

/* Timer0. */
#define PIC_REG_TMR0L         0x0EU   /* (unused on the 7x, kept for naming). */

/* Timer1. */
#define PIC_REG_TMR1L         0x0EU
#define PIC_REG_TMR1H         0x0FU
#define PIC_REG_T1CON         0x10U

/* Timer2. */
#define PIC_REG_TMR2          0x11U
#define PIC_REG_T2CON         0x12U

/* SSP (SPI-only on the 7x; no I²C slave. Bank 0). */
#define PIC_REG_SSPBUF        0x13U
#define PIC_REG_SSPCON        0x14U

/* CCP (CCP1 on every part; CCP2 on every part except the 16F72). */
#define PIC_REG_CCP1RL        0x15U
#define PIC_REG_CCP1RH        0x16U
#define PIC_REG_CCP1CON       0x17U

/* USART (16F72 has none). */
#define PIC_REG_RCSTA         0x18U
#define PIC_REG_TXREG         0x19U
#define PIC_REG_RCREG         0x1AU
#define PIC_REG_CCPR2L        0x1BU   /* 40-pin & CCP2 parts only. */
#define PIC_REG_CCPR2H        0x1CU
#define PIC_REG_CCP2CON       0x1DU

/* ADC, single register result.
 * The DS30325 parts (16F72-77) carry one 8-bit result register ADRES;
 * the DS30498 parts (16F737-777) carry ADRESH/ADRESL. */
#if PIC16F7X_FAMILY_ADC_10BIT
#define PIC_REG_ADRESH        0x1EU
#define PIC_REG_ADRESL        0x9EU
#else
#define PIC_REG_ADRES         0x1EU
#endif
#define PIC_REG_ADCON0        0x1FU

/* Bank 1. */
#define PIC_REG_OPTION        0x81U
#define PIC_REG_PR2           0x92U
#define PIC_REG_SSPADD        0x93U
#define PIC_REG_SSPSTAT       0x94U
#define PIC_REG_TXSTA         0x98U
#define PIC_REG_SPBRG         0x99U
#define PIC_REG_ADCON1        0x9FU

/* Bank 2: program-memory block; the 7x has NO data EEPROM. */
#define PIC_REG_PMDATA        0x10CU
#define PIC_REG_PMADR         0x10DU
#define PIC_REG_PMDATH        0x10EU
#define PIC_REG_PMADRH        0x10FU

/* Bank 3. */
#define PIC_REG_PMCON1        0x18CU

/* STATUS register bits (DS30325 Register 2-1). */
#define PIC_STATUS_C          EPIC_BIT(0)
#define PIC_STATUS_DC         EPIC_BIT(1)
#define PIC_STATUS_Z          EPIC_BIT(2)
#define PIC_STATUS_PD         EPIC_BIT(3)
#define PIC_STATUS_TO         EPIC_BIT(4)
#define PIC_STATUS_RP0        EPIC_BIT(5)
#define PIC_STATUS_RP1        EPIC_BIT(6)
#define PIC_STATUS_IRP        EPIC_BIT(7)

/* INTCON register bits (DS30325 Register 11-1 / DS30498 Register 13-1). */
#define PIC_INTCON_RBIF       EPIC_BIT(0)
#define PIC_INTCON_INTF       EPIC_BIT(1)
#define PIC_INTCON_TMR0IF     EPIC_BIT(2)
#define PIC_INTCON_RBIE       EPIC_BIT(3)
#define PIC_INTCON_INTE       EPIC_BIT(4)
#define PIC_INTCON_TMR0IE     EPIC_BIT(5)
#define PIC_INTCON_PEIE       EPIC_BIT(6)
#define PIC_INTCON_GIE        EPIC_BIT(7)

/* PIR1 / PIE1 (DS30325 Register 11-2 / DS30498 Register 13-2). */
#define PIC_PIR1_TMR1IF       EPIC_BIT(0)
#define PIC_PIR1_TMR2IF       EPIC_BIT(1)
#define PIC_PIR1_CCP1IF       EPIC_BIT(2)
#define PIC_PIR1_SSPIF        EPIC_BIT(3)
#define PIC_PIR1_TXIF         EPIC_BIT(4)
#define PIC_PIR1_RCIF         EPIC_BIT(5)
#define PIC_PIR1_ADIF         EPIC_BIT(6)
#define PIC_PIR1_PSPIF        EPIC_BIT(7)   /* 40-pin only. */

#define PIC_PIE1_TMR1IE       EPIC_BIT(0)
#define PIC_PIE1_TMR2IE       EPIC_BIT(1)
#define PIC_PIE1_CCP1IE       EPIC_BIT(2)
#define PIC_PIE1_SSPIE        EPIC_BIT(3)
#define PIC_PIE1_TXIE         EPIC_BIT(4)
#define PIC_PIE1_RCIE         EPIC_BIT(5)
#define PIC_PIE1_ADIE         EPIC_BIT(6)
#define PIC_PIE1_PSPIE        EPIC_BIT(7)

/* PIR2 / PIE2. The 16F77 carries only CCP2IF in PIR2 (DS30325
 * Register 11-3); the DS30498 parts add a couple more. No BCL on the
 * 7x: its SSP is SPI-only with no bus-collision path. */
#define PIC_PIR2_CCP2IF       EPIC_BIT(0)
#define PIC_PIE2_CCP2IE       EPIC_BIT(0)

/* Reset values (POR). */
#define PIC_STATUS_POR_VALUE     0x18U
#define PIC_PCON_POR_VALUE       0x0FU   /* BOR and POR flags unknown. */
#define PIC_INTCON_POR_VALUE     0x00U
#define PIC_PIR1_POR_VALUE       0x00U
#define PIC_PIR2_POR_VALUE       0x00U
#define PIC_PIE1_POR_VALUE       0x00U
#define PIC_PIE2_POR_VALUE       0x00U
#define PIC_T1CON_POR_VALUE      0x00U
#define PIC_T2CON_POR_VALUE      0x00U
#define PIC_ADCON0_POR_VALUE     0x00U
#define PIC_ADCON1_POR_VALUE     0x00U

/* OPTION_REG bits (Timer0 + WDT). */
#define PIC_OPTION_RBPU         EPIC_BIT(7)
#define PIC_OPTION_INTEDG       EPIC_BIT(6)
#define PIC_OPTION_T0CS         EPIC_BIT(5)
#define PIC_OPTION_T0SE         EPIC_BIT(4)
#define PIC_OPTION_PSA          EPIC_BIT(3)
#define PIC_OPTION_PS_MASK      0x07U

/* T1CON bits (Timer1). */
#define PIC_T1CON_TMR1ON        EPIC_BIT(0)
#define PIC_T1CON_TMR1CS        EPIC_BIT(1)
#define PIC_T1CON_T1SYNC        EPIC_BIT(2)
#define PIC_T1CON_T1OSCEN       EPIC_BIT(3)
#define PIC_T1CON_T1CKPS0       EPIC_BIT(4)
#define PIC_T1CON_T1CKPS1       EPIC_BIT(5)

/* T2CON bits (Timer2). */
#define PIC_T2CON_T2CKPS_MASK   0x03U
#define PIC_T2CON_TMR2ON        EPIC_BIT(2)
#define PIC_T2CON_TOUTPS_MASK   0x78U
#define PIC_T2CON_TOUTPS_POS    3U

/* CCPxCON bits. */
#define PIC_CCP_CCPX_M0         EPIC_BIT(0)
#define PIC_CCP_CCPX_M1         EPIC_BIT(1)
#define PIC_CCP_CCPX_M2         EPIC_BIT(2)
#define PIC_CCP_CCPX_M3         EPIC_BIT(3)
#define PIC_CCP_CCPX_Y          EPIC_BIT(4)
#define PIC_CCP_CCPX_X          EPIC_BIT(5)

/* TXSTA bits (USART). */
#define PIC_TXSTA_TX9D         EPIC_BIT(0)
#define PIC_TXSTA_TRMT         EPIC_BIT(1)
#define PIC_TXSTA_BRGH         EPIC_BIT(2)
#define PIC_TXSTA_SYNC         EPIC_BIT(4)
#define PIC_TXSTA_TXEN         EPIC_BIT(5)
#define PIC_TXSTA_TX9          EPIC_BIT(6)
#define PIC_TXSTA_CSRC         EPIC_BIT(7)

/* RCSTA bits (USART). */
#define PIC_RCSTA_RX9D         EPIC_BIT(0)
#define PIC_RCSTA_OERR         EPIC_BIT(1)
#define PIC_RCSTA_FERR         EPIC_BIT(2)
#define PIC_RCSTA_ADDEN        EPIC_BIT(3)
#define PIC_RCSTA_CREN         EPIC_BIT(4)
#define PIC_RCSTA_SREN         EPIC_BIT(5)
#define PIC_RCSTA_RX9          EPIC_BIT(6)
#define PIC_RCSTA_SPEN         EPIC_BIT(7)

/* SSPCON / SSPSTAT bits (MSSP, SPI on the 7x). */
#define PIC_SSPCON_SSPM_MASK   0x0FU
#define PIC_SSPCON_CKP         EPIC_BIT(4)
#define PIC_SSPCON_SSPEN       EPIC_BIT(5)
#define PIC_SSPCON_SSPOV       EPIC_BIT(6)
#define PIC_SSPCON_WCOL        EPIC_BIT(7)

#define PIC_SSPSTAT_BF         EPIC_BIT(0)
#define PIC_SSPSTAT_UA         EPIC_BIT(1)
#define PIC_SSPSTAT_RW         EPIC_BIT(2)
#define PIC_SSPSTAT_S          EPIC_BIT(3)
#define PIC_SSPSTAT_P          EPIC_BIT(4)
#define PIC_SSPSTAT_DA         EPIC_BIT(5)
#define PIC_SSPSTAT_CKE        EPIC_BIT(6)
#define PIC_SSPSTAT_SMP        EPIC_BIT(7)

/* ADCON0 / ADCON1 bits (A/D). */
#define PIC_ADCON0_ADON        EPIC_BIT(0)
#define PIC_ADCON0_GO_DONE     EPIC_BIT(2)
#define PIC_ADCON0_CHS_MASK    0x1CU                /* CHS2:CHS0, bits 5:3. */
#define PIC_ADCON0_CHS_POS     3U
#define PIC_ADCON0_ADCS_MASK   0xC0U                /* ADCS1:ADCS0, bits 7:6. */
#define PIC_ADCON0_ADCS_POS    6U

#define PIC_ADCON1_PCFG_MASK   0x0FU                /* PCFG3:PCFG0 (DS30498). */
#define PIC_ADCON1_ADCS2       EPIC_BIT(6)    /* ADCS2 (DS30498). */
#define PIC_ADCON1_ADFM        EPIC_BIT(7)    /* Result format (DS30498). */

/* TRISE bits (PORT E and PSP). PSP is 40-pin only. */
#define PIC_TRISE_IBF          EPIC_BIT(7)
#define PIC_TRISE_OBF          EPIC_BIT(6)
#define PIC_TRISE_IBOV         EPIC_BIT(5)
#define PIC_TRISE_PSPMODE      EPIC_BIT(4)

/* Bank-selection helper. Set the bank-select bits RP1:RP0 in STATUS
 * to access a given bank (DS30325 §2.2, Table 2-1). A macro, not a
 * static inline: a call boundary corrupts Bank 1 writes under XC8
 * v4.00 (see README.md, XC8 codegen gotchas). */
#define pic_select_bank(bank)                                          \
    do {                                                               \
        uint8_t pic_select_bank_status_ = EPIC_REG8(PIC_REG_STATUS);   \
        pic_select_bank_status_ &=                                     \
            (uint8_t)~(PIC_STATUS_RP0 | PIC_STATUS_RP1);               \
        pic_select_bank_status_ |= (uint8_t)(((bank) & 0x03U) << 5);   \
        EPIC_REG8(PIC_REG_STATUS) = pic_select_bank_status_;           \
    } while (0)

#endif /* PIC16F7X_SFR_H */
