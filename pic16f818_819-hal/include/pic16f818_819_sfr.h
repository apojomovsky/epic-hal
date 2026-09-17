/* SFR map for the PIC16F818/819 family, 1-to-1 from DS39598F (Figures
 * 2-3/2-4 and Registers 2-1 .. 11-1), cross-checked against the DFP EDC
 * (edc/PIC16F818.PIC, edc/PIC16F819.PIC; the two parts carry a
 * byte-identical map). Addresses are regenerated from the EDC by
 * scripts/gen-sfr.py; bit positions and POR values are hand-maintained
 * between the DS and the DFP. Registers the die lacks (USART, CCP2,
 * SSPCON2, SSPMSK, CMCON, CVRCON, ANSEL, WDTCON, PORTC/D/E) are
 * deliberately absent: referencing one must fail at compile time. */

#ifndef PIC16F818_819_SFR_H
#define PIC16F818_819_SFR_H

#include "pic16f818_819_hal.h"

/* Bank 0, core SFRs. */

/** Indirect address pointer.                    DS39598F §2.2, addr 00h. */
#define PIC_REG_INDF          0x00U
/** Timer0 counter.                              DS39598F §6.0, addr 01h. */
#define PIC_REG_TMR0          0x01U
/** Program Counter low byte.                    DS39598F §2.2, addr 02h. */
#define PIC_REG_PCL           0x02U
/** Status register.                             DS39598F Register 2-1. */
#define PIC_REG_STATUS        0x03U
/** File Select Register (indirect addressing).  DS39598F §2.2, addr 04h. */
#define PIC_REG_FSR           0x04U

/* I/O ports, DS39598F §5.0. */
#define PIC_REG_PORTA         0x05U
#define PIC_REG_PORTB         0x06U

/** Option register, Bank 1.                      DS39598F Register 2-2. */
#define PIC_REG_OPTION        0x81U
/* The shared drivers paste the SFR token into the banked-access macros
 * (EPIC_BANK1_READ8(OPTION_REG, ...)), whose host forms index
 * PIC_REG_##sfr_name, so the token needs its own define. */
#define PIC_REG_OPTION_REG    PIC_REG_OPTION
#define PIC_REG_TRISA         0x85U
#define PIC_REG_TRISB         0x86U

/* Core CPU control, DS39598F Registers 2-3 .. 2-8. */
#define PIC_REG_PCLATH        0x0AU
#define PIC_REG_INTCON        0x0BU
#define PIC_REG_PIR1          0x0CU
#define PIC_REG_PIR2          0x0DU
#define PIC_REG_PIE1          0x8CU   /* Bank 1. */
#define PIC_REG_PIE2          0x8DU   /* Bank 1. */
#define PIC_REG_PCON          0x8EU   /* Bank 1. */
#define PIC_REG_OSCCON        0x8FU   /* Bank 1. */
#define PIC_REG_OSCTUNE       0x90U   /* Bank 1. */

/* Timer1, DS39598F Register 7-1. */
#define PIC_REG_TMR1L         0x0EU
#define PIC_REG_TMR1H         0x0FU
#define PIC_REG_T1CON         0x10U

/* Timer2, DS39598F Register 8-1. */
#define PIC_REG_TMR2          0x11U
#define PIC_REG_T2CON         0x12U
#define PIC_REG_PR2           0x92U   /* Bank 1. */

/* SSP, DS39598F Registers 10-1/10-2. */
#define PIC_REG_SSPBUF        0x13U
#define PIC_REG_SSPCON        0x14U
#define PIC_REG_SSPADD        0x93U   /* Bank 1. */
#define PIC_REG_SSPSTAT       0x94U   /* Bank 1. */

/* CCP1, DS39598F Register 9-1. */
#define PIC_REG_CCP1RL        0x15U
#define PIC_REG_CCP1RH        0x16U
#define PIC_REG_CCP1CON       0x17U

/* ADC, DS39598F Registers 11-1/11-2. */
#define PIC_REG_ADRESH        0x1EU
#define PIC_REG_ADCON0        0x1FU
#define PIC_REG_ADRESL        0x9EU   /* Bank 1. */
#define PIC_REG_ADCON1        0x9FU   /* Bank 1. */

/* Bank 2, EEPROM data pair plus the high address bytes, DS39598F
 * Register 3-1 and Table 3-1. The control pair is in Bank 3. */
#define PIC_REG_EEDATA        0x10CU
#define PIC_REG_EEADR         0x10DU
#define PIC_REG_EEDATH        0x10EU
#define PIC_REG_EEADRH        0x10FU
#define PIC_REG_EECON1        0x18CU
#define PIC_REG_EECON2        0x18DU

/* STATUS register bits. */

/** Carry / Borrow.                              DS39598F Register 2-1. */
#define PIC_STATUS_C          EPIC_BIT(0)
/** Digit Carry.                                 DS39598F Register 2-1. */
#define PIC_STATUS_DC         EPIC_BIT(1)
/** Zero.                                        DS39598F Register 2-1. */
#define PIC_STATUS_Z          EPIC_BIT(2)
/** Power-down.                                  DS39598F Register 2-1. */
#define PIC_STATUS_PD         EPIC_BIT(3)
/** Time-out.                                    DS39598F Register 2-1. */
#define PIC_STATUS_TO         EPIC_BIT(4)
/** Register bank select bits (RP0:RP1).         DS39598F §2.2, Register 2-1. */
#define PIC_STATUS_RP0        EPIC_BIT(5)
#define PIC_STATUS_RP1        EPIC_BIT(6)
/** IRP: bank-select for indirect addressing, 1 = Banks 2/3.
 *  DS39598F Register 2-1, §2.2 (FSR + IRP form the 9-bit address). */
#define PIC_STATUS_IRP        EPIC_BIT(7)

/* INTCON register bits, DS39598F Register 2-3. */

#define PIC_INTCON_RBIF       EPIC_BIT(0)   /* RB<7:4> change flag.   */
#define PIC_INTCON_INTF       EPIC_BIT(1)   /* External INT flag.     */
#define PIC_INTCON_TMR0IF     EPIC_BIT(2)   /* TMR0 overflow flag.    */
#define PIC_INTCON_RBIE       EPIC_BIT(3)   /* RB<7:4> change enable. */
#define PIC_INTCON_INTE       EPIC_BIT(4)   /* External INT enable.   */
#define PIC_INTCON_TMR0IE     EPIC_BIT(5)   /* TMR0 overflow enable.  */
#define PIC_INTCON_PEIE       EPIC_BIT(6)   /* Peripheral int enable. */
#define PIC_INTCON_GIE        EPIC_BIT(7)   /* Global int enable.     */

/* PIR1 / PIE1, DS39598F Registers 2-4/2-5. ADIF/ADIE sit at bit 6:
 * bits 4 and 5 are the USART pair this die does not have. */

#define PIC_PIR1_TMR1IF       EPIC_BIT(0)
#define PIC_PIR1_TMR2IF       EPIC_BIT(1)
#define PIC_PIR1_CCP1IF       EPIC_BIT(2)
#define PIC_PIR1_SSPIF        EPIC_BIT(3)
#define PIC_PIR1_ADIF         EPIC_BIT(6)

#define PIC_PIE1_TMR1IE       EPIC_BIT(0)
#define PIC_PIE1_TMR2IE       EPIC_BIT(1)
#define PIC_PIE1_CCP1IE       EPIC_BIT(2)
#define PIC_PIE1_SSPIE        EPIC_BIT(3)
#define PIC_PIE1_ADIE         EPIC_BIT(6)

/* PIR2 / PIE2, DS39598F Registers 2-6/2-7. Only the EEPROM event is
 * implemented: every other bit of the pair reads as 0. */

#define PIC_PIR2_EEIF         EPIC_BIT(4)
#define PIC_PIE2_EEIE         EPIC_BIT(4)

/* PCON register bits, DS39598F Register 2-8. Only nBOR/nPOR exist. */

#define PIC_PCON_BOR          EPIC_BIT(0)   /* Brown-out Reset status. */
#define PIC_PCON_POR          EPIC_BIT(1)   /* Power-on Reset status.  */

/* OSCCON register bits, DS39598F Register 4-2. IRCF selects one of the
 * eight INTOSC frequencies; IOFS reports the INTOSC stability. */

#define PIC_OSCCON_IRCF_MASK  0x70U         /* IRCF2:IRCF0, bits 6:4. */
#define PIC_OSCCON_IRCF_POS   4U
#define PIC_OSCCON_IOFS       EPIC_BIT(2)   /* INTOSC frequency stable, read-only. */
/* OSCTUNE register bits, DS39598F Register 4-1. */

#define PIC_OSCTUNE_TUN_MASK  0x3FU         /* TUN5:TUN0, bits 5:0. */

/* OPTION_REG bits (Timer0 prescaler, WDT postscaler, INT edge, PORTB
 * pull-ups), DS39598F Register 2-2. */

#define PIC_OPTION_RBPU       EPIC_BIT(7)   /* PORTB pull-up enable (active-low). */
#define PIC_OPTION_INTEDG     EPIC_BIT(6)   /* INT edge select.                   */
#define PIC_OPTION_T0CS       EPIC_BIT(5)   /* TMR0 clock source.                 */
#define PIC_OPTION_T0SE       EPIC_BIT(4)   /* TMR0 source edge.                  */
#define PIC_OPTION_PSA        EPIC_BIT(3)   /* Prescaler assignment.              */
#define PIC_OPTION_PS_MASK    0x07U         /* PS2:PS0 prescaler ratio.           */

/* T1CON bits, DS39598F Register 7-1. No TMR1GE/T1GINV: this Timer1 has
 * no gate input, which is why PIC14MIDRANGE_HAS_TMR1_GATE is 0. */

#define PIC_T1CON_TMR1ON      EPIC_BIT(0)
#define PIC_T1CON_TMR1CS      EPIC_BIT(1)
#define PIC_T1CON_T1SYNC      EPIC_BIT(2)   /* DS spells it T1SYNC; the DFP adds T1INSYNC. */
#define PIC_T1CON_T1OSCEN     EPIC_BIT(3)
#define PIC_T1CON_T1CKPS0     EPIC_BIT(4)
#define PIC_T1CON_T1CKPS1     EPIC_BIT(5)

/* T2CON bits, DS39598F Register 8-1. */

#define PIC_T2CON_T2CKPS_MASK 0x03U         /* T2CKPS1:T2CKPS0, bits 1:0. */
#define PIC_T2CON_TMR2ON      EPIC_BIT(2)
#define PIC_T2CON_TOUTPS_MASK 0x78U         /* TOUTPS3:TOUTPS0, bits 6:3. */
#define PIC_T2CON_TOUTPS_POS  3U

/* CCP1CON bits, DS39598F Register 9-1. The two LSBs of the 10-bit PWM
 * duty cycle live in CCP1CON<5:4> (CCP1X:CCP1Y). */

#define PIC_CCP_CCPX_M0       EPIC_BIT(0)
#define PIC_CCP_CCPX_M1       EPIC_BIT(1)
#define PIC_CCP_CCPX_M2       EPIC_BIT(2)
#define PIC_CCP_CCPX_M3       EPIC_BIT(3)
#define PIC_CCP_CCPX_Y        EPIC_BIT(4)
#define PIC_CCP_CCPX_X        EPIC_BIT(5)

/* SSPCON / SSPSTAT bits, DS39598F Registers 10-1/10-2. SSPCON<3:0>
 * selects the mode in both SPI and I2C operation. */

#define PIC_SSPCON_SSPM_MASK  0x0FU         /* SSPM3:SSPM0 mode select. */
#define PIC_SSPCON_CKP        EPIC_BIT(4)   /* Clock polarity.          */
#define PIC_SSPCON_SSPEN      EPIC_BIT(5)   /* SSP enable.              */
#define PIC_SSPCON_SSPOV      EPIC_BIT(6)   /* Receive overflow.        */
#define PIC_SSPCON_WCOL       EPIC_BIT(7)   /* Write collision.         */

#define PIC_SSPSTAT_BF        EPIC_BIT(0)   /* Buffer full.            */
#define PIC_SSPSTAT_UA        EPIC_BIT(1)   /* Update address (I2C).   */
#define PIC_SSPSTAT_RW        EPIC_BIT(2)   /* Read/write (I2C).       */
#define PIC_SSPSTAT_S         EPIC_BIT(3)   /* Start (I2C).            */
#define PIC_SSPSTAT_P         EPIC_BIT(4)   /* Stop (I2C).             */
#define PIC_SSPSTAT_DA        EPIC_BIT(5)   /* Data/address (I2C).     */
#define PIC_SSPSTAT_CKE       EPIC_BIT(6)   /* Clock edge (SPI).       */
#define PIC_SSPSTAT_SMP       EPIC_BIT(7)   /* Sample bit (SPI).       */

/* ADCON0 / ADCON1 bits, DS39598F Registers 11-1/11-2. */

#define PIC_ADCON0_ADON       EPIC_BIT(0)   /* A/D on.             */
#define PIC_ADCON0_GO_DONE    EPIC_BIT(2)   /* Start / status.     */
#define PIC_ADCON0_CHS_MASK   0x1CU         /* CHS2:CHS0, bits 5:3. */
#define PIC_ADCON0_CHS_POS    3U
#define PIC_ADCON0_ADCS_MASK  0xC0U         /* ADCS1:ADCS0, bits 7:6. */
#define PIC_ADCON0_ADCS_POS   6U

#define PIC_ADCON1_PCFG_MASK  0x0FU         /* PCFG3:PCFG0, bits 3:0. */
#define PIC_ADCON1_ADCS2      EPIC_BIT(6)   /* ADCS2, bit 6.          */
#define PIC_ADCON1_ADFM       EPIC_BIT(7)   /* Result format.         */

/* EECON1 bits, DS39598F Register 3-1. This part keeps no EEIF here:
 * the EEPROM write-done flag is PIR2<EEIF> (PIC_PIR2_EEIF). */

#define PIC_EECON1_RD         EPIC_BIT(0)   /* Read control.  */
#define PIC_EECON1_WR         EPIC_BIT(1)   /* Write control. */
#define PIC_EECON1_WREN       EPIC_BIT(2)   /* Write enable.  */
#define PIC_EECON1_WRERR      EPIC_BIT(3)   /* Write error.   */
#define PIC_EECON1_FREE       EPIC_BIT(4)   /* Flash row erase (self-write). */
#define PIC_EECON1_EEPGD      EPIC_BIT(7)   /* Program/Data EEPROM select.   */

/* Reset values (POR/BOR), DS39598F Table 2-1's per-register POR/BOR
 * column and the DFP EDC por fields. STATUS keeps TO/PD set and the
 * bank bits clear. */

#define PIC_STATUS_POR_VALUE     0x18U  /* 0001 1xxx. */
#define PIC_INTCON_POR_VALUE     0x00U
#define PIC_PIR1_POR_VALUE       0x00U
#define PIC_PIR2_POR_VALUE       0x00U
#define PIC_PIE1_POR_VALUE       0x00U
#define PIC_PIE2_POR_VALUE       0x00U
/* Both flags read as unknown after a POR, so both are modelled set
 * (neither reset cause is reported). */
#define PIC_PCON_POR_VALUE       0x03U
#define PIC_OSCCON_POR_VALUE     0x00U
#define PIC_OSCTUNE_POR_VALUE    0x00U
#define PIC_T1CON_POR_VALUE      0x00U
#define PIC_T2CON_POR_VALUE      0x00U
#define PIC_ADCON0_POR_VALUE     0x00U
#define PIC_ADCON1_POR_VALUE     0x00U
#define PIC_OPTION_POR_VALUE     0xFFU
#define PIC_TRISA_POR_VALUE      0xFFU
#define PIC_TRISB_POR_VALUE      0xFFU
#define PIC_PR2_POR_VALUE        0xFFU
#define PIC_EECON1_POR_VALUE     0x00U
#define PIC_SSPCON_POR_VALUE     0x00U
#define PIC_SSPSTAT_POR_VALUE    0x00U
#define PIC_CCP1CON_POR_VALUE    0x00U

/* Bank-selection helper. */

/* Set the bank-select bits RP1:RP0 in STATUS to access a given bank
 * (DS39598F §2.2). A macro, not a static inline: a call
 * boundary corrupts Bank 1 writes under XC8 v4.00 (see README.md, XC8
 * codegen gotchas), and combined calls can hang XC8's cgpic pass. */
#define pic_select_bank(bank)                                          \
    do {                                                               \
        uint8_t pic_select_bank_status_ = EPIC_REG8(PIC_REG_STATUS);   \
        pic_select_bank_status_ &=                                     \
            (uint8_t)~(PIC_STATUS_RP0 | PIC_STATUS_RP1);               \
        pic_select_bank_status_ |= (uint8_t)(((bank) & 0x03U) << 5);   \
        EPIC_REG8(PIC_REG_STATUS) = pic_select_bank_status_;           \
    } while (0)

#endif /* PIC16F818_819_SFR_H */
