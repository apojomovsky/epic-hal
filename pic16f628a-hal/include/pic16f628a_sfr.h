/* SFR map for the PIC16F628A, 1-to-1 from DS40044G and cross-checked
 * against the DFP EDC (Microchip.PIC16Fxxx_DFP, edc/PIC16F628A.PIC);
 * bit positions match the EDC field order. Registers the part lacks
 * (PORTC/D/E, PIR2/PIE2, SSP, ADC, CCP2, EEDATH/H, SPBRGH/BAUDCTL,
 * OSCCON) are deliberately absent: referencing one must fail at
 * compile time. Regenerated via scripts/gen-sfr.py. */

#ifndef PIC16F628A_SFR_H
#define PIC16F628A_SFR_H

#include "pic16f628a_hal.h"

/* Bank 0, core SFRs. */

#define PIC_REG_INDF          0x00U
#define PIC_REG_OPTION        0x81U   /* Bank 1. */
#define PIC_REG_PCL           0x02U
#define PIC_REG_STATUS        0x03U
#define PIC_REG_FSR           0x04U

/* I/O ports: PORTA (RA0..RA7, RA5 input-only) + PORTB only. */
#define PIC_REG_PORTA         0x05U
#define PIC_REG_PORTB         0x06U

#define PIC_REG_TRISA         0x85U   /* Bank 1. */
#define PIC_REG_TRISB         0x86U   /* Bank 1. */

/* Core CPU control, DS40044G §14.0. */
#define PIC_REG_PCLATH        0x0AU
#define PIC_REG_INTCON        0x0BU
#define PIC_REG_PIR1          0x0CU
#define PIC_REG_PIE1          0x8CU   /* Bank 1. */
#define PIC_REG_PCON          0x8EU   /* Bank 1. */

/* Timer0. */
#define PIC_REG_TMR0          0x01U

/* Timer1. */
#define PIC_REG_TMR1L         0x0EU
#define PIC_REG_TMR1H         0x0FU
#define PIC_REG_T1CON         0x10U

/* Timer2. */
#define PIC_REG_TMR2          0x11U
#define PIC_REG_T2CON         0x12U

/* CCP1. */
#define PIC_REG_CCP1RL        0x15U
#define PIC_REG_CCP1RH        0x16U
#define PIC_REG_CCP1CON       0x17U

/* USART. */
#define PIC_REG_RCSTA         0x18U
#define PIC_REG_TXREG         0x19U
#define PIC_REG_RCREG         0x1AU

/* USART, Bank 1. */
#define PIC_REG_TXSTA         0x98U
#define PIC_REG_SPBRG         0x99U

/* Comparators, Bank 0 (unlike 87XA's Bank-1 CMCON). */
#define PIC_REG_CMCON         0x1FU

/* Voltage reference, Bank 1. */
#define PIC_REG_VRCON         0x9FU

/* Timer2 period register, Bank 1. */
#define PIC_REG_PR2           0x92U
/* Data EEPROM, Bank 1 (unlike 87XA's Banks 2/3). */
#define PIC_REG_EEDATA        0x9AU
#define PIC_REG_EEADR         0x9BU
#define PIC_REG_EECON1        0x9CU
#define PIC_REG_EECON2        0x9DU

/* STATUS register bits. */

#define PIC_STATUS_C          EPIC_BIT(0)
#define PIC_STATUS_DC         EPIC_BIT(1)
#define PIC_STATUS_Z          EPIC_BIT(2)
#define PIC_STATUS_PD         EPIC_BIT(3)
#define PIC_STATUS_TO         EPIC_BIT(4)
#define PIC_STATUS_RP0        EPIC_BIT(5)
#define PIC_STATUS_RP1        EPIC_BIT(6)
#define PIC_STATUS_IRP        EPIC_BIT(7)

/* INTCON register bits. */

#define PIC_INTCON_RBIF       EPIC_BIT(0)
#define PIC_INTCON_INTF       EPIC_BIT(1)
#define PIC_INTCON_TMR0IF     EPIC_BIT(2)
#define PIC_INTCON_RBIE       EPIC_BIT(3)
#define PIC_INTCON_INTE       EPIC_BIT(4)
#define PIC_INTCON_TMR0IE     EPIC_BIT(5)
#define PIC_INTCON_PEIE       EPIC_BIT(6)
#define PIC_INTCON_GIE        EPIC_BIT(7)

/* PIR1 / PIE1 (no PIR2/PIE2 on this part). */

#define PIC_PIR1_TMR1IF       EPIC_BIT(0)
#define PIC_PIR1_TMR2IF       EPIC_BIT(1)
#define PIC_PIR1_CCP1IF       EPIC_BIT(2)
#define PIC_PIR1_TXIF         EPIC_BIT(4)
#define PIC_PIR1_RCIF         EPIC_BIT(5)
#define PIC_PIR1_CMIF         EPIC_BIT(6)
#define PIC_PIR1_EEIF         EPIC_BIT(7)

#define PIC_PIE1_TMR1IE       EPIC_BIT(0)
#define PIC_PIE1_TMR2IE       EPIC_BIT(1)
#define PIC_PIE1_CCP1IE       EPIC_BIT(2)
#define PIC_PIE1_TXIE         EPIC_BIT(4)
#define PIC_PIE1_RCIE         EPIC_BIT(5)
#define PIC_PIE1_CMIE         EPIC_BIT(6)
#define PIC_PIE1_EEIE         EPIC_BIT(7)

/* Reset values (POR). */

#define PIC_STATUS_POR_VALUE     0x18U
#define PIC_PCON_POR_VALUE       0x0FU  /* BOR and POR flags unknown. */
#define PIC_INTCON_POR_VALUE     0x00U
#define PIC_PIR1_POR_VALUE       0x00U
#define PIC_PIE1_POR_VALUE       0x00U
#define PIC_T1CON_POR_VALUE      0x00U
#define PIC_T2CON_POR_VALUE      0x00U

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

/* CCP1CON bits. */

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

/* CMCON bits (Comparator, Bank 0). */

#define PIC_CMCON_CM_MASK      0x07U
#define PIC_CMCON_CIS          EPIC_BIT(3)
#define PIC_CMCON_C1INV        EPIC_BIT(4)
#define PIC_CMCON_C2INV        EPIC_BIT(5)
#define PIC_CMCON_C1OUT        EPIC_BIT(6)
#define PIC_CMCON_C2OUT        EPIC_BIT(7)

/* VRCON bits (Vref, Bank 1). VRR=1 selects the low range. */

#define PIC_VRCON_VR_MASK    0x0FU
#define PIC_VRCON_VRR         EPIC_BIT(5)
#define PIC_VRCON_VROE        EPIC_BIT(6)
#define PIC_VRCON_VREN        EPIC_BIT(7)

/* EECON1 bits (EEPROM). */

#define PIC_EECON1_RD          EPIC_BIT(0)
#define PIC_EECON1_WR          EPIC_BIT(1)
#define PIC_EECON1_WREN        EPIC_BIT(2)
#define PIC_EECON1_WRERR       EPIC_BIT(3)
/* No EEIF in EECON1: the EEPROM write-done flag is PIR1<EEIF>
 * (PIC_PIR1_EEIF). */

/* Bank-selection helper (RP1:RP0 in STATUS). A macro, not a static
 * inline: a call boundary corrupts Bank 1 writes under XC8 v4.00. */
#define pic_select_bank(bank)                                          \
    do {                                                               \
        uint8_t pic_select_bank_status_ = EPIC_REG8(PIC_REG_STATUS);   \
        pic_select_bank_status_ &=                                     \
            (uint8_t)~(PIC_STATUS_RP0 | PIC_STATUS_RP1);               \
        pic_select_bank_status_ |= (uint8_t)(((bank) & 0x03U) << 5);   \
        EPIC_REG8(PIC_REG_STATUS) = pic_select_bank_status_;           \
    } while (0)

#endif /* PIC16F628A_SFR_H */
