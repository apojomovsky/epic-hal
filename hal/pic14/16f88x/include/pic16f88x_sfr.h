/* SFR address map for the PIC16F88X family. Every address, bit mask
 * and reset value is 1-to-1 from DS40001291H (Figures 2-4..2-6, Tables
 * 2-1..2-4, 3-1..3-14, 4-1..4-2, 5-1, 6-1, 7-1, 8-1..8-5, 9-1..9-2,
 * 10-1..10-5, 11-1..11-5, 12-1..12-3, 13-1..13-4, 14-1..14-3);
 * device-dependent registers (PORTD/E on 40/44-pin parts) are guarded
 * by PIC16F88X_FAMILY_HAS_*. */

#ifndef PIC16F88X_SFR_H
#define PIC16F88X_SFR_H

#include "pic16f88x.h"

/* Bank 0, core SFRs. */

/** Indirect address pointer.                    DS40001291H §2.2, addr 00h. */
#define PIC_REG_INDF          0x00U
/** Option register (Bank 1).                     DS40001291H §2.2, addr 81h. */
#define PIC_REG_OPTION        0x81U
/** Program Counter low byte.                    DS40001291H §2.2, addr 02h. */
#define PIC_REG_PCL           0x02U
/** Status register.                             DS40001291H §2.2, addr 03h. */
#define PIC_REG_STATUS        0x03U
/** File Select Register (indirect addressing).  DS40001291H §2.2, addr 04h. */
#define PIC_REG_FSR           0x04U

/* I/O ports, Tables 3-1..3-14. */
#define PIC_REG_PORTA         0x05U
#define PIC_REG_PORTB         0x06U
#define PIC_REG_PORTC         0x07U
#define PIC_REG_PORTD         0x08U   /* 40/44-pin only. */
#define PIC_REG_PORTE         0x09U   /* RE<2:0> 40/44-pin only, RE3 all. */

#define PIC_REG_TRISA         0x85U
#define PIC_REG_TRISB         0x86U
#define PIC_REG_TRISC         0x87U
#define PIC_REG_TRISD         0x88U   /* 40/44-pin only. */
#define PIC_REG_TRISE         0x89U   /* RE<2:0> 40/44-pin only, RE3 all. */

/* Core CPU control, DS40001291H §2.2.2, Table 14-4. */
#define PIC_REG_PCLATH        0x0AU
#define PIC_REG_INTCON        0x0BU
#define PIC_REG_PIR1          0x0CU
#define PIC_REG_PIR2          0x0DU
#define PIC_REG_PIE1          0x8CU   /* Bank 1, DS40001291H Table 14-4. */
#define PIC_REG_PIE2          0x8DU   /* Bank 1. */
#define PIC_REG_PCON          0x8EU

/* Timer0, DS40001291H §5.0. */
#define PIC_REG_TMR0          0x01U

/* Timer1, DS40001291H §6.0. */
#define PIC_REG_TMR1L         0x0EU
#define PIC_REG_TMR1H         0x0FU
#define PIC_REG_T1CON         0x10U

/* Timer2, DS40001291H §7.0. */
#define PIC_REG_TMR2          0x11U
#define PIC_REG_T2CON         0x12U

/* MSSP, DS40001291H §13.0. */
#define PIC_REG_SSPBUF        0x13U
#define PIC_REG_SSPCON        0x14U

/* CCP, DS40001291H §11.0. */
#define PIC_REG_CCP1RL        0x15U
#define PIC_REG_CCP1RH        0x16U
#define PIC_REG_CCP1CON       0x17U

/* EUSART, DS40001291H §12.0. */
#define PIC_REG_RCSTA         0x18U
#define PIC_REG_TXREG         0x19U
#define PIC_REG_RCREG         0x1AU
#define PIC_REG_CCPR2L        0x1BU
#define PIC_REG_CCPR2H        0x1CU
#define PIC_REG_CCP2CON       0x1DU

/* ADC, DS40001291H §9.0. */
#define PIC_REG_ADRESH        0x1EU
#define PIC_REG_ADCON0        0x1FU

/* Bank 1. */

/* MSSP, DS40001291H §13.0, Bank 1. */
#define PIC_REG_SSPCON2       0x91U
#define PIC_REG_PR2           0x92U
#define PIC_REG_SSPADD        0x93U   /* SSPMSK alias when SSPM=1001. */
#define PIC_REG_SSPSTAT       0x94U

/* GPIO, DS40001291H §3.0, Bank 1. */
#define PIC_REG_WPUB          0x95U
#define PIC_REG_IOCB          0x96U

/* Comparator voltage reference, DS40001291H §8.10, Register 8-5. */
#define PIC_REG_VRCON         0x97U

/* EUSART, DS40001291H §12.0, Bank 1. */
#define PIC_REG_TXSTA         0x98U
#define PIC_REG_SPBRG         0x99U
#define PIC_REG_SPBRGH        0x9AU

/* ECCP, DS40001291H §11.0, Bank 1. */
#define PIC_REG_PWM1CON       0x9BU
#define PIC_REG_ECCPAS        0x9CU
#define PIC_REG_PSTRCON       0x9DU

/* ADC, DS40001291H §9.0, Bank 1. */
#define PIC_REG_ADRESL        0x9EU
#define PIC_REG_ADCON1        0x9FU

/* Bank 2. */

/* WDT, DS40001291H §14.5, Register 14-3. */
#define PIC_REG_WDTCON        0x105U

/* Comparators, DS40001291H §8.0, Registers 8-1..8-3. */
#define PIC_REG_CM1CON0       0x107U
#define PIC_REG_CM2CON0       0x108U
#define PIC_REG_CM2CON1       0x109U

/* EEPROM, DS40001291H §10.0. */
#define PIC_REG_EEDATA        0x10CU
#define PIC_REG_EEADR         0x10DU
#define PIC_REG_EEDATH        0x10EU
#define PIC_REG_EEADRH        0x10FU

/* Bank 3. */

/* SR latch, DS40001291H §8.9, Register 8-4. */
#define PIC_REG_SRCON         0x185U

/* EUSART, DS40001291H §12.0, Register 12-3. */
#define PIC_REG_BAUDCTL       0x187U

/* Oscillator, DS40001291H §4.2, Registers 4-1/4-2 (Bank 1). */
#define PIC_REG_OSCCON        0x8FU
#define PIC_REG_OSCTUNE       0x90U

/* Analog select, DS40001291H §3.0, Registers 3-3/3-4. */
#define PIC_REG_ANSEL         0x188U
#define PIC_REG_ANSELH        0x189U

/* EEPROM, DS40001291H §10.0, Bank 3. */
#define PIC_REG_EECON1        0x18CU
#define PIC_REG_EECON2        0x18DU

/* STATUS register bits. */

/** Carry / Borrow.                              DS40001291H §2.2 / Register 2-1. */
#define PIC_STATUS_C          EPIC_BIT(0)
/** Digit Carry.                                  DS40001291H Register 2-1. */
#define PIC_STATUS_DC         EPIC_BIT(1)
/** Zero.                                         DS40001291H Register 2-1. */
#define PIC_STATUS_Z          EPIC_BIT(2)
/** Power-down.                                   DS40001291H §14.6, Register 2-1. */
#define PIC_STATUS_PD         EPIC_BIT(3)
/** Time-out.                                     DS40001291H §14.5, Register 2-1. */
#define PIC_STATUS_TO         EPIC_BIT(4)
/** Register Page Select bits (RP0:RP1).          DS40001291H Register 2-1. */
#define PIC_STATUS_RP0        EPIC_BIT(5)
#define PIC_STATUS_RP1        EPIC_BIT(6)
/** IRP (indirect bank select).                  DS40001291H Register 2-1. */
#define PIC_STATUS_IRP        EPIC_BIT(7)

/* INTCON register bits. */

/** RB port change interrupt flag.               DS40001291H §3.4.3, Reg 2-3. */
#define PIC_INTCON_RBIF       EPIC_BIT(0)
/** RB0/INT external interrupt flag.             DS40001291H §14.11.1, Reg 2-3. */
#define PIC_INTCON_INTF       EPIC_BIT(1)
/** TMR0 overflow interrupt flag.                DS40001291H §14.11.2, Reg 2-3. */
#define PIC_INTCON_TMR0IF     EPIC_BIT(2)
/** RB port change interrupt enable.             DS40001291H §3.4.3, Reg 2-3. */
#define PIC_INTCON_RBIE       EPIC_BIT(3)
/** RB0/INT external interrupt enable.           DS40001291H §14.11.1, Reg 2-3. */
#define PIC_INTCON_INTE       EPIC_BIT(4)
/** TMR0 overflow interrupt enable.              DS40001291H §14.11.2, Reg 2-3. */
#define PIC_INTCON_TMR0IE     EPIC_BIT(5)
/** Peripheral interrupt enable.                 DS40001291H §14.11, Reg 2-3. */
#define PIC_INTCON_PEIE       EPIC_BIT(6)
/** Global interrupt enable.                     DS40001291H §14.11, Reg 2-3. */
#define PIC_INTCON_GIE        EPIC_BIT(7)

/* PIR1 / PIE1. */

/* DS40001291H §14.11, Registers 2-4/2-6. */
#define PIC_PIR1_TMR1IF       EPIC_BIT(0)
#define PIC_PIR1_TMR2IF       EPIC_BIT(1)
#define PIC_PIR1_CCP1IF       EPIC_BIT(2)
#define PIC_PIR1_SSPIF        EPIC_BIT(3)
#define PIC_PIR1_TXIF         EPIC_BIT(4)
#define PIC_PIR1_RCIF         EPIC_BIT(5)
#define PIC_PIR1_ADIF         EPIC_BIT(6)
/* PIR1 bit 7 is unimplemented on the 88X (no PSP). */

#define PIC_PIE1_TMR1IE       EPIC_BIT(0)
#define PIC_PIE1_TMR2IE       EPIC_BIT(1)
#define PIC_PIE1_CCP1IE       EPIC_BIT(2)
#define PIC_PIE1_SSPIE        EPIC_BIT(3)
#define PIC_PIE1_TXIE         EPIC_BIT(4)
#define PIC_PIE1_RCIE         EPIC_BIT(5)
#define PIC_PIE1_ADIE         EPIC_BIT(6)
/* PIE1 bit 7 is unimplemented on the 88X (no PSP). */

/* PIR2 / PIE2. */

/* DS40001291H §14.11, Registers 2-5/2-7. */
#define PIC_PIR2_CCP2IF       EPIC_BIT(0)
#define PIC_PIR2_ULPWUIF      EPIC_BIT(2)
#define PIC_PIR2_BCLIF        EPIC_BIT(3)
#define PIC_PIR2_EEIF         EPIC_BIT(4)
#define PIC_PIR2_C1IF         EPIC_BIT(5)
#define PIC_PIR2_C2IF         EPIC_BIT(6)
#define PIC_PIR2_OSFIF        EPIC_BIT(7)

#define PIC_PIE2_CCP2IE       EPIC_BIT(0)
#define PIC_PIE2_ULPWUIE      EPIC_BIT(2)
#define PIC_PIE2_BCLIE        EPIC_BIT(3)
#define PIC_PIE2_EEIE         EPIC_BIT(4)
#define PIC_PIE2_C1IE         EPIC_BIT(5)
#define PIC_PIE2_C2IE         EPIC_BIT(6)
#define PIC_PIE2_OSFIE        EPIC_BIT(7)

/* Reset values (POR). */

/* DS40001291H §14, Table 14-4. */
#define PIC_STATUS_POR_VALUE     0x18U  /* 0001 1xxx, IRP=0,RP1=0,RP0=0,TO=1,PD=1,... */
#define PIC_PCON_POR_VALUE       0x10U  /* --01 --0x: ULPWUE=0, SBOREN=1, POR=0 (a POR occurred), BOR=x. */
#define PIC_INTCON_POR_VALUE     0x00U
#define PIC_PIR1_POR_VALUE       0x00U
#define PIC_PIR2_POR_VALUE       0x00U
#define PIC_PIE1_POR_VALUE       0x00U
#define PIC_PIE2_POR_VALUE       0x00U
#define PIC_T1CON_POR_VALUE      0x00U
#define PIC_T2CON_POR_VALUE      0x00U
#define PIC_ADCON0_POR_VALUE     0x00U
#define PIC_ADCON1_POR_VALUE     0x00U
#define PIC_OSCCON_POR_VALUE     0x70U  /* -110 q000: IRCF=110 (4 MHz), OSTS=1, HTS/LTS=0, SCS=0. */
#define PIC_OSCTUNE_POR_VALUE    0x00U
#define PIC_WDTCON_POR_VALUE     0x08U  /* ---0 1000: WDTPS=0100 (1:512), SWDTEN=0. */
#define PIC_ANSEL_POR_VALUE      0xFFU
#define PIC_ANSELH_POR_VALUE     0x3FU  /* --11 1111: ANS<13:8> all analog. */
#define PIC_WPUB_POR_VALUE       0xFFU
#define PIC_IOCB_POR_VALUE       0x00U
#define PIC_VRCON_POR_VALUE      0x00U
#define PIC_CM1CON0_POR_VALUE    0x00U
#define PIC_CM2CON0_POR_VALUE    0x00U
#define PIC_CM2CON1_POR_VALUE    0x02U  /* 0000 --10: T1GSS=1, C2SYNC=0. */
#define PIC_SRCON_POR_VALUE      0x00U
#define PIC_BAUDCTL_POR_VALUE    0x40U  /* 01-0 0-00: ABDOVF=0, RCIDL=1 (hardware), SCKP=0, BRG16=0, WUE=0, ABDEN=0. */
#define PIC_PWM1CON_POR_VALUE    0x00U
#define PIC_ECCPAS_POR_VALUE     0x00U
#define PIC_PSTRCON_POR_VALUE    0x01U  /* ---0 0001: STRSYNC=0, STRD/C/B=0, STRA=1. */
#define PIC_TXSTA_POR_VALUE      0x02U  /* 0000 0010: TRMT=1. */
#define PIC_RCSTA_POR_VALUE      0x00U
#define PIC_SSPCON_POR_VALUE     0x00U
#define PIC_SSPCON2_POR_VALUE    0x00U
#define PIC_SSPSTAT_POR_VALUE    0x00U
#define PIC_SSPADD_POR_VALUE     0x00U
#define PIC_PR2_POR_VALUE        0xFFU
#define PIC_T2CON_POR_VALUE      0x00U

/* OPTION_REG bits (Timer0 + WDT). */

/* DS40001291H §5.3, Register 5-1. */
#define PIC_OPTION_RBPU         EPIC_BIT(7)   /* PORTB pull-up enable (active-low). */
#define PIC_OPTION_INTEDG       EPIC_BIT(6)   /* INT edge select.                */
#define PIC_OPTION_T0CS         EPIC_BIT(5)   /* TMR0 clock source.              */
#define PIC_OPTION_T0SE         EPIC_BIT(4)   /* TMR0 source edge.               */
#define PIC_OPTION_PSA          EPIC_BIT(3)   /* Prescaler assignment.           */
#define PIC_OPTION_PS_MASK      0x07U         /* PS2:PS0, prescaler ratio.       */

/* PCON bits. */

/* DS40001291H §14.2.6, Register 2-8. */
#define PIC_PCON_BOR           EPIC_BIT(0)    /* Brown-out Reset status.  */
#define PIC_PCON_POR           EPIC_BIT(1)    /* Power-on Reset status.   */
#define PIC_PCON_SBOREN        EPIC_BIT(4)    /* Software BOR enable.     */
#define PIC_PCON_ULPWUE        EPIC_BIT(5)    /* Ultra low-power wake-up. */

/* OSCCON bits. */

/* DS40001291H §4.2, Register 4-1. */
#define PIC_OSCCON_SCS         EPIC_BIT(0)    /* System clock select.        */
#define PIC_OSCCON_LTS         EPIC_BIT(1)    /* LFINTOSC stable.            */
#define PIC_OSCCON_HTS         EPIC_BIT(2)    /* HFINTOSC stable.            */
#define PIC_OSCCON_OSTS        EPIC_BIT(3)    /* Osc start-up time-out.      */
#define PIC_OSCCON_IRCF_MASK   0x70U          /* IRCF2:IRCF0, bits 6:4.      */
#define PIC_OSCCON_IRCF_POS    4U

/* OSCTUNE bits. */

/* DS40001291H §4.5.4, Register 4-2. */
#define PIC_OSCTUNE_TUN_MASK   0x1FU          /* TUN4:TUN0, bits 4:0.        */

/* T1CON bits (Timer1). */

/* DS40001291H §6.0, Register 6-1. */
#define PIC_T1CON_TMR1ON        EPIC_BIT(0)
#define PIC_T1CON_TMR1CS        EPIC_BIT(1)
#define PIC_T1CON_T1SYNC        EPIC_BIT(2)
#define PIC_T1CON_T1OSCEN       EPIC_BIT(3)
#define PIC_T1CON_T1CKPS0       EPIC_BIT(4)
#define PIC_T1CON_T1CKPS1       EPIC_BIT(5)
#define PIC_T1CON_TMR1GE        EPIC_BIT(6)    /* Timer1 gate enable. */
#define PIC_T1CON_T1GINV        EPIC_BIT(7)    /* Timer1 gate invert. */

/* T2CON bits (Timer2). */

/* DS40001291H §7.0, Register 7-1. */
#define PIC_T2CON_T2CKPS_MASK   0x03U          /* T2CKPS1:T2CKPS0, bits 0..1. */
#define PIC_T2CON_TMR2ON        EPIC_BIT(2)
#define PIC_T2CON_TOUTPS_MASK   0x78U          /* TOUTPS3:TOUTPS0, bits 3..6. */
#define PIC_T2CON_TOUTPS_POS    3U

/* CCP1CON bits (ECCP). */

/* DS40001291H §11.0, Register 11-1. */
#define PIC_CCP1_CCP1M0         EPIC_BIT(0)    /* CCP1M0, mode bit 0.        */
#define PIC_CCP1_CCP1M1         EPIC_BIT(1)    /* CCP1M1, mode bit 1.        */
#define PIC_CCP1_CCP1M2         EPIC_BIT(2)    /* CCP1M2, mode bit 2.        */
#define PIC_CCP1_CCP1M3         EPIC_BIT(3)    /* CCP1M3, mode bit 3.        */
#define PIC_CCP1_DC1B0          EPIC_BIT(4)    /* PWM duty LSB bit 0.        */
#define PIC_CCP1_DC1B1          EPIC_BIT(5)    /* PWM duty LSB bit 1.        */
#define PIC_CCP1_P1M0           EPIC_BIT(6)    /* PWM output config bit 0.   */
#define PIC_CCP1_P1M1           EPIC_BIT(7)    /* PWM output config bit 1.   */

/* CCP2CON bits (CCP2). */

/* DS40001291H §11.0, Register 11-2. */
#define PIC_CCP2_CCP2M0         EPIC_BIT(0)
#define PIC_CCP2_CCP2M1         EPIC_BIT(1)
#define PIC_CCP2_CCP2M2         EPIC_BIT(2)
#define PIC_CCP2_CCP2M3         EPIC_BIT(3)
#define PIC_CCP2_DC2B0          EPIC_BIT(4)
#define PIC_CCP2_DC2B1          EPIC_BIT(5)

/* ECCPAS bits (auto-shutdown). */

/* DS40001291H §11.6.4, Register 11-3. */
#define PIC_ECCPAS_PSSBD_MASK   0x03U          /* PSSBD1:PSSBD0, bits 1:0.    */
#define PIC_ECCPAS_PSSAC_MASK   0x0CU          /* PSSAC1:PSSAC0, bits 3:2.    */
#define PIC_ECCPAS_ECCPAS_MASK  0x70U          /* ECCPAS2:ECCPAS0, bits 6:4.  */
#define PIC_ECCPAS_ECCPAS_POS   4U
#define PIC_ECCPAS_ECCPASE      EPIC_BIT(7)    /* Auto-shutdown event status. */

/* PWM1CON bits (dead-time). */

/* DS40001291H §11.6.5, Register 11-4. */
#define PIC_PWM1CON_PDC_MASK    0x7FU          /* PDC6:PDC0, bits 6:0.        */
#define PIC_PWM1CON_PRSEN       EPIC_BIT(7)    /* PWM restart enable.         */

/* PSTRCON bits (pulse steering). */

/* DS40001291H §11.6.7, Register 11-5. */
#define PIC_PSTRCON_STRA        EPIC_BIT(0)
#define PIC_PSTRCON_STRB        EPIC_BIT(1)
#define PIC_PSTRCON_STRC        EPIC_BIT(2)
#define PIC_PSTRCON_STRD        EPIC_BIT(3)
#define PIC_PSTRCON_STRSYNC     EPIC_BIT(4)

/* TXSTA bits (EUSART). */

/* DS40001291H §12.0, Register 12-1. */
#define PIC_TXSTA_TX9D         EPIC_BIT(0)    /* 9th bit of TX data. */
#define PIC_TXSTA_TRMT         EPIC_BIT(1)    /* TSR empty. */
#define PIC_TXSTA_BRGH         EPIC_BIT(2)    /* High baud rate. */
#define PIC_TXSTA_SENDB        EPIC_BIT(3)    /* Send break char. */
#define PIC_TXSTA_SYNC         EPIC_BIT(4)    /* Sync mode. */
#define PIC_TXSTA_TXEN         EPIC_BIT(5)    /* TX enable. */
#define PIC_TXSTA_TX9          EPIC_BIT(6)    /* 9-bit TX. */
#define PIC_TXSTA_CSRC         EPIC_BIT(7)    /* Clock source (sync). */

/* RCSTA bits (EUSART). */

/* DS40001291H §12.0, Register 12-2. */
#define PIC_RCSTA_RX9D         EPIC_BIT(0)    /* 9th bit of RX data. */
#define PIC_RCSTA_OERR         EPIC_BIT(1)    /* Overrun error. */
#define PIC_RCSTA_FERR         EPIC_BIT(2)    /* Framing error. */
#define PIC_RCSTA_ADDEN        EPIC_BIT(3)    /* Address detect (9-bit). */
#define PIC_RCSTA_CREN         EPIC_BIT(4)    /* Continuous receive. */
#define PIC_RCSTA_SREN         EPIC_BIT(5)    /* Single receive. */
#define PIC_RCSTA_RX9          EPIC_BIT(6)    /* 9-bit RX. */
#define PIC_RCSTA_SPEN         EPIC_BIT(7)    /* Serial port enable. */

/* BAUDCTL bits (EUSART). */

/* DS40001291H §12.0, Register 12-3. */
#define PIC_BAUDCTL_ABDEN      EPIC_BIT(0)    /* Auto-baud detect enable. */
#define PIC_BAUDCTL_WUE        EPIC_BIT(1)    /* Wake-up enable. */
#define PIC_BAUDCTL_BRG16      EPIC_BIT(3)    /* 16-bit BRG. */
#define PIC_BAUDCTL_SCKP       EPIC_BIT(4)    /* Sync clock polarity. */
#define PIC_BAUDCTL_RCIDL      EPIC_BIT(6)    /* Receive idle (read-only). */
#define PIC_BAUDCTL_ABDOVF     EPIC_BIT(7)    /* Auto-baud overflow (read-only). */

/* SSPCON / SSPSTAT bits (MSSP). */

/* DS40001291H §13.0, Registers 13-1 (SSPSTAT), 13-2 (SSPCON),
 * 13-3 (SSPCON2). The same SSPCON<3:0> field selects mode in both
 * SPI and I²C operation. */
#define PIC_SSPCON_SSPM_MASK   0x0FU          /* SSPM3:SSPM0.       */
#define PIC_SSPCON_CKP         EPIC_BIT(4)    /* Clock polarity.    */
#define PIC_SSPCON_SSPEN       EPIC_BIT(5)    /* SSP enable.        */
#define PIC_SSPCON_SSPOV       EPIC_BIT(6)    /* Receive overflow.  */
#define PIC_SSPCON_WCOL        EPIC_BIT(7)    /* Write collision.   */

/* SSPCON2 (I²C only), Register 13-3. */
#define PIC_SSPCON2_SEN        EPIC_BIT(0)    /* Start condition enable. */
#define PIC_SSPCON2_RSEN       EPIC_BIT(1)    /* Repeated start enable.  */
#define PIC_SSPCON2_PEN        EPIC_BIT(2)    /* Stop condition enable.  */
#define PIC_SSPCON2_RCEN       EPIC_BIT(3)    /* Receive enable.         */
#define PIC_SSPCON2_ACKEN      EPIC_BIT(4)    /* Acknowledge sequence.   */
#define PIC_SSPCON2_ACKDT      EPIC_BIT(5)    /* Acknowledge data.       */
#define PIC_SSPCON2_ACKSTAT    EPIC_BIT(6)    /* Acknowledge status.     */
#define PIC_SSPCON2_GCEN       EPIC_BIT(7)    /* General call enable.    */

/* SSPSTAT, Register 13-1. */
#define PIC_SSPSTAT_BF         EPIC_BIT(0)    /* Buffer full.        */
#define PIC_SSPSTAT_UA         EPIC_BIT(1)    /* Update address.     */
#define PIC_SSPSTAT_RW         EPIC_BIT(2)    /* Read/write (I²C).    */
#define PIC_SSPSTAT_S          EPIC_BIT(3)    /* Start (I²C).         */
#define PIC_SSPSTAT_P          EPIC_BIT(4)    /* Stop (I²C).          */
#define PIC_SSPSTAT_DA         EPIC_BIT(5)    /* Data/address (I²C).  */
#define PIC_SSPSTAT_CKE        EPIC_BIT(6)    /* Clock edge (SPI).    */
#define PIC_SSPSTAT_SMP        EPIC_BIT(7)    /* Sample bit (SPI).    */

/* SSPMSK (I²C address mask). */

/* DS40001291H §13.0, Register 13-4. Accessible at 0x93 only when
 * SSPCON<SSPM3:SSPM0> = 1001. */
#define PIC_SSPMSK_MASK_ALL     0xFFU

/* ADCON0 / ADCON1 bits (A/D). */

/* DS40001291H §9.0, Registers 9-1 (ADCON0) and 9-2 (ADCON1). */
#define PIC_ADCON0_ADON        EPIC_BIT(0)    /* A/D on.        */
#define PIC_ADCON0_GO_DONE     EPIC_BIT(1)    /* Start / status. */
#define PIC_ADCON0_CHS_MASK    0x3CU          /* CHS3:CHS0, bits 5:2. */
#define PIC_ADCON0_CHS_POS     2U
#define PIC_ADCON0_ADCS_MASK   0xC0U          /* ADCS1:ADCS0, bits 7:6. */
#define PIC_ADCON0_ADCS_POS    6U

#define PIC_ADCON1_VCFG0       EPIC_BIT(4)    /* VREF+ source (VDD / VREF+ pin). */
#define PIC_ADCON1_VCFG1       EPIC_BIT(5)    /* VREF- source (VSS / VREF- pin). */
#define PIC_ADCON1_ADFM        EPIC_BIT(7)    /* Result format. */

/* ADC channel selectors (ADCON0<CHS3:CHS0>). */

/* DS40001291H §9.0, Register 9-1. */
#define PIC_ADC_CH_AN0          0x00U
#define PIC_ADC_CH_AN1          0x01U
#define PIC_ADC_CH_AN2          0x02U
#define PIC_ADC_CH_AN3          0x03U
#define PIC_ADC_CH_AN4          0x04U
#define PIC_ADC_CH_AN5          0x05U   /* 40/44-pin only. */
#define PIC_ADC_CH_AN6          0x06U   /* 40/44-pin only. */
#define PIC_ADC_CH_AN7          0x07U   /* 40/44-pin only. */
#define PIC_ADC_CH_AN8          0x08U
#define PIC_ADC_CH_AN9          0x09U
#define PIC_ADC_CH_AN10         0x0AU
#define PIC_ADC_CH_AN11         0x0BU
#define PIC_ADC_CH_AN12         0x0CU
#define PIC_ADC_CH_AN13         0x0DU
#define PIC_ADC_CH_CVREF        0x0EU
#define PIC_ADC_CH_VP6          0x0FU   /* Fixed 0.6 V reference. */

/* CM1CON0 / CM2CON0 bits (Comparators). */

/* DS40001291H §8.0, Registers 8-1/8-2. */
#define PIC_CMx_CxCH_MASK       0x03U   /* CxCH1:CxCH0, bits 1:0. */
#define PIC_CMx_CxR             EPIC_BIT(2)    /* Reference select. */
#define PIC_CMx_CxPOL           EPIC_BIT(4)    /* Output polarity. */
#define PIC_CMx_CxOE            EPIC_BIT(5)    /* Output enable. */
#define PIC_CMx_CxOUT           EPIC_BIT(6)    /* Output (read-only). */
#define PIC_CMx_CxON            EPIC_BIT(7)    /* Comparator on. */

/* CM2CON1 bits. */

/* DS40001291H §8.8, Register 8-3. */
#define PIC_CM2CON1_C2SYNC      EPIC_BIT(0)    /* C2 output sync to TMR1. */
#define PIC_CM2CON1_T1GSS       EPIC_BIT(1)    /* TMR1 gate source (T1G/SYNC C2OUT). */
#define PIC_CM2CON1_C2RSEL      EPIC_BIT(4)    /* C2 ref select (CVREF/0.6V). */
#define PIC_CM2CON1_C1RSEL      EPIC_BIT(5)    /* C1 ref select (CVREF/0.6V). */
#define PIC_CM2CON1_MC2OUT      EPIC_BIT(6)    /* Mirror C2OUT (read-only). */
#define PIC_CM2CON1_MC1OUT      EPIC_BIT(7)    /* Mirror C1OUT (read-only). */

/* SRCON bits (SR latch). */

/* DS40001291H §8.9, Register 8-4. */
#define PIC_SRCON_FVREN         EPIC_BIT(0)    /* Fixed 0.6 V ref enable. */
#define PIC_SRCON_PULSR         EPIC_BIT(2)    /* Pulse latch reset (self-clearing). */
#define PIC_SRCON_PULSS         EPIC_BIT(3)    /* Pulse latch set (self-clearing). */
#define PIC_SRCON_C2REN         EPIC_BIT(4)    /* C2 output resets latch. */
#define PIC_SRCON_C1SEN         EPIC_BIT(5)    /* C1 output sets latch. */
#define PIC_SRCON_SR0           EPIC_BIT(6)    /* C1OUT pin = latch Q. */
#define PIC_SRCON_SR1           EPIC_BIT(7)    /* C2OUT pin = latch Q. */

/* VRCON bits (CVREF). */

/* DS40001291H §8.10, Register 8-5. */
#define PIC_VRCON_VR_MASK       0x0FU          /* VR3:VR0, bits 3:0. */
#define PIC_VRCON_VRSS          EPIC_BIT(4)    /* Ref source (VREF+/VDD). */
#define PIC_VRCON_VRR           EPIC_BIT(5)    /* Range select. */
#define PIC_VRCON_VROE          EPIC_BIT(6)    /* Output to pin enable. */
#define PIC_VRCON_VREN          EPIC_BIT(7)    /* CVREF on. */

/* WPUB bits (PORTB pull-ups). */

/* DS40001291H §3.4.2, Register 3-7. */
#define PIC_WPUB_WPUB_MASK      0xFFU

/* IOCB bits (interrupt-on-change). */

/* DS40001291H §3.4.3, Register 3-8. */
#define PIC_IOCB_IOCB_MASK      0xFFU

/* ANSEL / ANSELH bits (analog select). */

/* DS40001291H §3.0, Registers 3-3/3-4. */
#define PIC_ANSEL_ANS_MASK      0x1FU          /* ANS<4:0>, all devices. */
#define PIC_ANSEL_ANS5          EPIC_BIT(5)    /* 40/44-pin only. */
#define PIC_ANSEL_ANS6          EPIC_BIT(6)    /* 40/44-pin only. */
#define PIC_ANSEL_ANS7          EPIC_BIT(7)    /* 40/44-pin only. */
#define PIC_ANSELH_ANS_MASK     0x3FU          /* ANS<13:8>, bits 5:0. */

/* EECON1 bits (EEPROM). */

/* DS40001291H §10.0, Register 10-5. */
#define PIC_EECON1_RD          EPIC_BIT(0)    /* Read control.   */
#define PIC_EECON1_WR          EPIC_BIT(1)    /* Write control.  */
#define PIC_EECON1_WREN        EPIC_BIT(2)    /* Write enable.   */
#define PIC_EECON1_WRERR       EPIC_BIT(3)    /* Write error.    */
#define PIC_EECON1_EEPGD       EPIC_BIT(7)    /* Program/data select. */

/* WDTCON bits (WDT). */

/* DS40001291H §14.5, Register 14-3. */
#define PIC_WDTCON_SWDTEN      EPIC_BIT(0)    /* Software WDT enable. */
#define PIC_WDTCON_WDTPS_MASK  0x1EU          /* WDTPS3:WDTPS0, bits 4:1. */
#define PIC_WDTCON_WDTPS_POS   1U

/* Bank-selection helper. */

/* Set the bank-select bits RP1:RP0 in STATUS to access a given bank
 * (DS40001291H §2.2, Table 2-1). A macro, not a static inline: a call
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

#endif /* PIC16F88X_SFR_H */
