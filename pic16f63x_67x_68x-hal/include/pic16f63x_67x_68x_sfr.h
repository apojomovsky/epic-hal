/* SFR address map for the PIC16F63x/67x/68x family (16F631 on this
 * ticket). Every address and bit position is 1-to-1 from the DFP proc
 * header (Microchip.PIC16Fxxx_DFP, xc8/pic/include/proc/pic16f631.h,
 * cross-checked against edc/PIC16F631.PIC); layout per DS40001262F
 * §2.0 (memory map), §4.0 (I/O), §5.0 (Timer0), §6.0 (Timer1), §14.0
 * (interrupts). Register addresses are regenerated from the EDC by
 * scripts/gen-sfr.py; bit positions and POR values are hand-maintained
 * against the DFP header. The #152 siblings extend the map; their
 * registers are absent here, not stubbed. */

#ifndef PIC16F63X_67X_68X_SFR_H
#define PIC16F63X_67X_68X_SFR_H

#include "pic16f63x_67x_68x_hal.h"

/* Bank 0, core SFRs. */

#define PIC_REG_INDF          0x00U
#define PIC_REG_TMR0          0x01U
#define PIC_REG_PCL           0x02U
#define PIC_REG_STATUS        0x03U
#define PIC_REG_FSR           0x04U

/* I/O ports: PORTA RA0..RA5, PORTB RB4..RB7, PORTC RC0..RC7, 18 I/O
 * (DS40001262F Table 1, pin summary). */
#define PIC_REG_PORTA         0x05U
#define PIC_REG_PORTB         0x06U
#define PIC_REG_PORTC         0x07U

/* Core CPU control. */
#define PIC_REG_PCLATH        0x0AU
#define PIC_REG_INTCON        0x0BU
#define PIC_REG_PIR1          0x0CU
#define PIC_REG_PIR2          0x0DU

/* Timer1. */
#define PIC_REG_TMR1L         0x0EU
#define PIC_REG_TMR1H         0x0FU
#define PIC_REG_T1CON         0x10U

/* Bank 1. */
#define PIC_REG_OPTION        0x81U
/* Alias under the DFP's full name: the shared drivers' banked macros
 * paste the SFR token (EPIC_BANK1_READ8(OPTION_REG, ...) wants
 * PIC_REG_OPTION_REG on builds where those macros are token-pasting
 * forms). */
#define PIC_REG_OPTION_REG    PIC_REG_OPTION
#define PIC_REG_TRISA         0x85U
#define PIC_REG_TRISB         0x86U
#define PIC_REG_TRISC         0x87U
#define PIC_REG_PIE1          0x8CU
#define PIC_REG_PIE2          0x8DU
#define PIC_REG_PCON          0x8EU
#define PIC_REG_OSCCON        0x8FU
#define PIC_REG_OSCTUNE       0x90U
#define PIC_REG_WPUA          0x95U
#define PIC_REG_IOCA          0x96U
#define PIC_REG_WDTCON        0x97U

/* Bank 2. */
#define PIC_REG_EEDATA        0x10CU
#define PIC_REG_EEADR         0x10DU
#define PIC_REG_WPUB          0x115U
#define PIC_REG_IOCB          0x116U
#define PIC_REG_VRCON         0x118U
#define PIC_REG_CM1CON0       0x119U
#define PIC_REG_CM2CON0       0x11AU
#define PIC_REG_CM2CON1       0x11BU
#define PIC_REG_ANSEL         0x11EU
#define PIC_REG_ANSELH        0x11FU

/* Bank 3. */
#define PIC_REG_EECON1        0x18CU
#define PIC_REG_EECON2        0x18DU
#define PIC_REG_SRCON         0x19EU

/* STATUS register bits. */

#define PIC_STATUS_C          EPIC_BIT(0)
#define PIC_STATUS_DC         EPIC_BIT(1)
#define PIC_STATUS_Z          EPIC_BIT(2)
#define PIC_STATUS_PD         EPIC_BIT(3)
#define PIC_STATUS_TO         EPIC_BIT(4)
#define PIC_STATUS_RP0        EPIC_BIT(5)
#define PIC_STATUS_RP1        EPIC_BIT(6)
#define PIC_STATUS_IRP        EPIC_BIT(7)

/* INTCON register bits (DS40001262F §14.0, Register 14-1). The silicon
 * names the PORTA/B-change flag RABIF/RABIE; RBIF/RBIE are compat
 * aliases for the shared core, which spells them the 87XA way. */

#define PIC_INTCON_RABIF      EPIC_BIT(0)
#define PIC_INTCON_INTF       EPIC_BIT(1)
#define PIC_INTCON_T0IF       EPIC_BIT(2)
#define PIC_INTCON_RABIE      EPIC_BIT(3)
#define PIC_INTCON_INTE       EPIC_BIT(4)
#define PIC_INTCON_T0IE       EPIC_BIT(5)
#define PIC_INTCON_PEIE       EPIC_BIT(6)
#define PIC_INTCON_GIE        EPIC_BIT(7)
#define PIC_INTCON_RBIF       PIC_INTCON_RABIF
#define PIC_INTCON_RBIE       PIC_INTCON_RABIE
#define PIC_INTCON_TMR0IF     PIC_INTCON_T0IF
#define PIC_INTCON_TMR0IE     PIC_INTCON_T0IE

/* PIR1 / PIE1: Timer1 overflow only (DS40001262F §14.0). */

#define PIC_PIR1_TMR1IF       EPIC_BIT(0)
#define PIC_PIE1_TMR1IE       EPIC_BIT(0)

/* PIR2 / PIE2: EEPROM, comparators C1/C2, oscillator fail
 * (DS40001262F §14.0). */

#define PIC_PIR2_EEIF         EPIC_BIT(4)
#define PIC_PIR2_C1IF         EPIC_BIT(5)
#define PIC_PIR2_C2IF         EPIC_BIT(6)
#define PIC_PIR2_OSFIF        EPIC_BIT(7)
#define PIC_PIE2_EEIE         EPIC_BIT(4)
#define PIC_PIE2_C1IE         EPIC_BIT(5)
#define PIC_PIE2_C2IE         EPIC_BIT(6)
#define PIC_PIE2_OSFIE        EPIC_BIT(7)

/* Reset values (POR), EDC por attributes, sanity-checked against the
 * DFP proc header defaults. */

#define PIC_STATUS_POR_VALUE     0x18U  /* 0001 1xxx: TO=1, PD=1. */
#define PIC_INTCON_POR_VALUE     0x00U
#define PIC_PIR1_POR_VALUE       0x00U
#define PIC_PIR2_POR_VALUE       0x00U
#define PIC_PIE1_POR_VALUE       0x00U
#define PIC_PIE2_POR_VALUE       0x00U
#define PIC_T1CON_POR_VALUE      0x00U
#define PIC_OPTION_POR_VALUE     0xFFU
#define PIC_TRISA_POR_VALUE      0x3FU  /* RA0..RA5 implemented. */
#define PIC_TRISB_POR_VALUE      0xF0U  /* RB4..RB7 implemented. */
#define PIC_TRISC_POR_VALUE      0xFFU
#define PIC_PCON_POR_VALUE       0x10U  /* --01 --qq: SBOREN=1. */
#define PIC_OSCCON_POR_VALUE     0x60U  /* -110 x000: IRCF=110. */
#define PIC_OSCTUNE_POR_VALUE    0x00U
#define PIC_WPUA_POR_VALUE       0x37U  /* WPUA0/1/2/4/5. */
#define PIC_WPUB_POR_VALUE       0xF0U  /* WPUB4..7. */
#define PIC_IOCA_POR_VALUE       0x00U
#define PIC_IOCB_POR_VALUE       0x00U
#define PIC_WDTCON_POR_VALUE     0x08U  /* ---0 1000: WDTPS=0100. */
#define PIC_VRCON_POR_VALUE      0x00U
#define PIC_CM1CON0_POR_VALUE    0x00U
#define PIC_CM2CON0_POR_VALUE    0x00U
#define PIC_CM2CON1_POR_VALUE    0x02U  /* ---- --10: T1GSS=1. */
#define PIC_ANSEL_POR_VALUE      0xF3U  /* ANS0/1/4/5/6/7 analog. */
#define PIC_ANSELH_POR_VALUE     0x0FU  /* ANS8..ANS11 analog (677; no ANSELH on 631). */
#define PIC_EECON1_POR_VALUE     0x00U
#define PIC_SRCON_POR_VALUE      0x00U

/* OPTION_REG bits (Timer0 + WDT prescaler + pull-up/INT edge,
 * DS40001262F §5.0, Register 5-1). Bit 7 is the PORTA/B pull-up
 * enable, active-low, named RABPU on this family. */

#define PIC_OPTION_PSA          EPIC_BIT(3)
#define PIC_OPTION_T0SE         EPIC_BIT(4)
#define PIC_OPTION_T0CS         EPIC_BIT(5)
#define PIC_OPTION_INTEDG       EPIC_BIT(6)
#define PIC_OPTION_RABPU        EPIC_BIT(7)
#define PIC_OPTION_PS_MASK      0x07U

/* PCON bits (DS40001262F §14.0). */

#define PIC_PCON_BOR            EPIC_BIT(0)
#define PIC_PCON_POR            EPIC_BIT(1)
#define PIC_PCON_SBOREN         EPIC_BIT(4)
#define PIC_PCON_ULPWUE         EPIC_BIT(5)

/* OSCCON bits (DS40001262F §3.0, Register 3-1). */

#define PIC_OSCCON_SCS          EPIC_BIT(0)
#define PIC_OSCCON_LTS          EPIC_BIT(1)
#define PIC_OSCCON_HTS          EPIC_BIT(2)
#define PIC_OSCCON_OSTS         EPIC_BIT(3)
#define PIC_OSCCON_IRCF_MASK    0x70U
#define PIC_OSCCON_IRCF_POS     4U

/* OSCTUNE bits (DS40001262F §3.0, Register 3-2). */

#define PIC_OSCTUNE_TUN_MASK    0x1FU

/* T1CON bits (Timer1, DS40001262F §6.0, Register 6-1). The datasheet
 * names the sync bit T1SYNC; the DFP spells it nT1SYNC. */

#define PIC_T1CON_TMR1ON        EPIC_BIT(0)
#define PIC_T1CON_TMR1CS        EPIC_BIT(1)
#define PIC_T1CON_T1SYNC        EPIC_BIT(2)
#define PIC_T1CON_T1OSCEN       EPIC_BIT(3)
#define PIC_T1CON_T1CKPS0       EPIC_BIT(4)
#define PIC_T1CON_T1CKPS1       EPIC_BIT(5)
#define PIC_T1CON_TMR1GE        EPIC_BIT(6)
#define PIC_T1CON_T1GINV        EPIC_BIT(7)

/* WDTCON bits (DS40001262F §14.0, Register 14-2). Bank 1, unlike the
 * 88X's Bank-2 WDTCON; the shared driver selects the bank per family. */

#define PIC_WDTCON_SWDTEN       EPIC_BIT(0)
#define PIC_WDTCON_WDTPS_MASK   0x1EU
#define PIC_WDTCON_WDTPS_POS    1U

/* WPUA/WPUB bits (DS40001262F §4.0, Registers 4-1/4-2). No pull-up on
 * RA3 (input-only MCLR pin). */

#define PIC_WPUA_WPUA0          EPIC_BIT(0)
#define PIC_WPUA_WPUA1          EPIC_BIT(1)
#define PIC_WPUA_WPUA2          EPIC_BIT(2)
#define PIC_WPUA_WPUA4          EPIC_BIT(4)
#define PIC_WPUA_WPUA5          EPIC_BIT(5)
#define PIC_WPUB_WPUB4          EPIC_BIT(4)
#define PIC_WPUB_WPUB5          EPIC_BIT(5)
#define PIC_WPUB_WPUB6          EPIC_BIT(6)
#define PIC_WPUB_WPUB7          EPIC_BIT(7)

/* IOCA/IOCB bits (DS40001262F §4.0, Registers 4-3/4-4). */

/* Port-change enable bits, one per implemented pin. */
#define PIC_IOCA_IOCA0          EPIC_BIT(0)
#define PIC_IOCA_IOCA1          EPIC_BIT(1)
#define PIC_IOCA_IOCA2          EPIC_BIT(2)
#define PIC_IOCA_IOCA3          EPIC_BIT(3)
#define PIC_IOCA_IOCA4          EPIC_BIT(4)
#define PIC_IOCA_IOCA5          EPIC_BIT(5)
#define PIC_IOCB_IOCB4          EPIC_BIT(4)
#define PIC_IOCB_IOCB5          EPIC_BIT(5)
#define PIC_IOCB_IOCB6          EPIC_BIT(6)
#define PIC_IOCB_IOCB7          EPIC_BIT(7)

/* CM1CON0 / CM2CON0 bits (comparators, DS40001262F Comparator
 * module, Registers CMxCON0). C1OUT/C2OUT are read-only live outputs
 * (see adding-a-device.md §4 step 8: mask them out of comparisons). */

#define PIC_CMx_CxCH_MASK       0x03U
#define PIC_CMx_CxCH_POS        0U
#define PIC_CMx_CxR             EPIC_BIT(2)
#define PIC_CMx_CxPOL           EPIC_BIT(4)
#define PIC_CMx_CxOE            EPIC_BIT(5)
#define PIC_CMx_CxOUT           EPIC_BIT(6)
#define PIC_CMx_CxON            EPIC_BIT(7)
#define PIC_CM1CON0_C1CH_MASK   0x03U
#define PIC_CM1CON0_C1R         EPIC_BIT(2)
#define PIC_CM1CON0_C1POL       EPIC_BIT(4)
#define PIC_CM1CON0_C1OE        EPIC_BIT(5)
#define PIC_CM1CON0_C1OUT       EPIC_BIT(6)
#define PIC_CM1CON0_C1ON        EPIC_BIT(7)
#define PIC_CM2CON0_C2CH_MASK   0x03U
#define PIC_CM2CON0_C2R         EPIC_BIT(2)
#define PIC_CM2CON0_C2POL       EPIC_BIT(4)
#define PIC_CM2CON0_C2OE        EPIC_BIT(5)
#define PIC_CM2CON0_C2OUT       EPIC_BIT(6)
#define PIC_CM2CON0_C2ON        EPIC_BIT(7)

/* CM2CON1 bits (DS40001262F Comparator module, Register CM2CON1). No
 * CxRSEL bits on this family (the 88X has them); the reference select
 * is CxR in CMxCON0 plus CxVREN in VRCON. */

#define PIC_CM2CON1_C2SYNC      EPIC_BIT(0)
#define PIC_CM2CON1_T1GSS       EPIC_BIT(1)
#define PIC_CM2CON1_MC2OUT      EPIC_BIT(6)
#define PIC_CM2CON1_MC1OUT      EPIC_BIT(7)

/* VRCON bits (DS40001262F Comparator module, Register VRCON). */

#define PIC_VRCON_VR_MASK       0x0FU
#define PIC_VRCON_VP6EN         EPIC_BIT(4)
#define PIC_VRCON_VRR           EPIC_BIT(5)
#define PIC_VRCON_C2VREN        EPIC_BIT(6)
#define PIC_VRCON_C1VREN        EPIC_BIT(7)

/* ANSEL bits (DS40001262F §4.0, Register 4-5). ANS2/ANS3 do not exist:
 * the comparator inputs skip straight from AN1 to AN4. */

#define PIC_ANSEL_ANS0          EPIC_BIT(0)
#define PIC_ANSEL_ANS1          EPIC_BIT(1)
#define PIC_ANSEL_ANS4          EPIC_BIT(4)
#define PIC_ANSEL_ANS5          EPIC_BIT(5)
#define PIC_ANSEL_ANS6          EPIC_BIT(6)
#define PIC_ANSEL_ANS7          EPIC_BIT(7)

/* ANSELH bits (677 only; DS40001262F §4.0, Register 4-6). Absent on
 * the 631: the driver gates every ANSELH access on
 * PIC16F63X_67X_68X_FAMILY_HAS_ANSELH. */
#define PIC_ANSELH_ANS8         EPIC_BIT(0)
#define PIC_ANSELH_ANS9         EPIC_BIT(1)
#define PIC_ANSELH_ANS10        EPIC_BIT(2)
#define PIC_ANSELH_ANS11        EPIC_BIT(3)

/* EECON1 bits (DS40001262F Data EEPROM module, Register EECON1). EEIF
 * is PIR2<4> here, not EECON1<4> as on the PIR-less 83/84 parts. */

#define PIC_EECON1_RD           EPIC_BIT(0)
#define PIC_EECON1_WR           EPIC_BIT(1)
#define PIC_EECON1_WREN         EPIC_BIT(2)
#define PIC_EECON1_WRERR        EPIC_BIT(3)

/* Bank-selection helper (RP1:RP0 in STATUS). A macro, not a static
 * inline: a call boundary corrupts banked writes under XC8 v4.00. */

#define pic_select_bank(bank)                                          \
    do {                                                               \
        uint8_t pic_select_bank_status_ = EPIC_REG8(PIC_REG_STATUS);   \
        pic_select_bank_status_ &=                                     \
            (uint8_t)~(PIC_STATUS_RP0 | PIC_STATUS_RP1);               \
        pic_select_bank_status_ |= (uint8_t)(((bank) & 0x03U) << 5);   \
        EPIC_REG8(PIC_REG_STATUS) = pic_select_bank_status_;           \
    } while (0)

#endif /* PIC16F63X_67X_68X_SFR_H */
