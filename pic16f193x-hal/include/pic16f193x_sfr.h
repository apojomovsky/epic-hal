/**
 * @file    pic16f193x_sfr.h
 * @brief   Special Function Register (SFR) address map for the
 *          PIC16F193X family.
 *
 * @details
 *   Every address here is taken 1-to-1 from the DS41364B data memory map
 *   (Table 2-4, banks 0-7; Table 2-5, banks 8-15). The Enhanced Mid-range
 *   core banks data memory with the BSR (DS41364B §2.2): each bank is 128
 *   bytes, the 12 core registers (0x00-0x0B) and the 16 common registers
 *   (0x20-0x2F) mirror in every bank, SFRs occupy 0x0C-0x1F per bank. A
 *   given SFR's address is its physical 12-bit address; XC8 auto-selects
 *   the bank for a literal SFR access on this core, so drivers address
 *   SFRs by their `PIC_REG_*` token and let the compiler bank.
 *
 *   Bit masks are included for the core, interrupt, GPIO, Timer0 and
 *   WDT/Sleep registers now (verified against DS41364B register
 *   definitions). Peripheral register bit masks (T1CON, T2CON, CCPxCON,
 *   ADCON, SSPCON, TXSTA/RCSTA, etc.) are added by each peripheral phase
 *   as it is built and verified, not pre-declared from memory.
 *
 *   Address conflicts that are pin-count-dependent (PORTD/PORTE and their
 *   TRIS/LAT/ANSEL mirrors do not exist on 28-pin parts) are guarded with
 *   PIC16F193X_FAMILY_HAS_* macros so the same header compiles for any of
 *   the six parts.
 */

#ifndef PIC16F193X_SFR_H
#define PIC16F193X_SFR_H

#include "pic16f193x.h"

/* ───────────────────────── Core registers (0x00-0x0B, every bank) ── */

/* DS41364B §2.2, Table 2-4. The 12 core registers mirror in every bank. */
#define PIC_REG_INDF0        0x00U
#define PIC_REG_INDF1        0x01U
#define PIC_REG_PCL           0x02U
#define PIC_REG_STATUS        0x03U
#define PIC_REG_FSR0L         0x04U
#define PIC_REG_FSR0H         0x05U
#define PIC_REG_FSR1L         0x06U
#define PIC_REG_FSR1H         0x07U
#define PIC_REG_BSR           0x08U
#define PIC_REG_WREG          0x09U
#define PIC_REG_PCLATH         0x0AU
#define PIC_REG_INTCON        0x0BU

/* ───────────────────────── Bank 0 SFRs (0x0C-0x1F) ──────────────── */

/* I/O ports, DS41364B §6.0. PORTD/PORTE are 40/44-pin only. */
#define PIC_REG_PORTA         0x0CU
#define PIC_REG_PORTB         0x0DU
#define PIC_REG_PORTC         0x0EU
#if PIC16F193X_FAMILY_HAS_PORTD
#define PIC_REG_PORTD         0x0FU
#endif
#if PIC16F193X_FAMILY_HAS_PORTE
#define PIC_REG_PORTE         0x10U
#endif

/* Interrupt flags, DS41364B §4.5. */
#define PIC_REG_PIR1          0x11U
#define PIC_REG_PIR2          0x12U
#define PIC_REG_PIR3          0x13U

/* Timer0, DS41364B §15.0. */
#define PIC_REG_TMR0          0x15U

/* Timer1, DS41364B §16.0. */
#define PIC_REG_TMR1L         0x16U
#define PIC_REG_TMR1H         0x17U
#define PIC_REG_T1CON         0x18U
#define PIC_REG_T1GCON        0x19U

/* Timer2, DS41364B §17.0. */
#define PIC_REG_TMR2          0x1AU
#define PIC_REG_PR2           0x1BU
#define PIC_REG_T2CON         0x1CU

/* Capacitive Sensing, DS41364B §18.0. */
#define PIC_REG_CPSCON0       0x1EU
#define PIC_REG_CPSCON1       0x1FU

/* ───────────────────────── Bank 1 SFRs (0x8C-0x9F) ───────────────── */

/* I/O direction, DS41364B §6.0. */
#define PIC_REG_TRISA         0x8CU
#define PIC_REG_TRISB         0x8DU
#define PIC_REG_TRISC         0x8EU
#if PIC16F193X_FAMILY_HAS_PORTD
#define PIC_REG_TRISD         0x8FU
#endif
#if PIC16F193X_FAMILY_HAS_PORTE
#define PIC_REG_TRISE         0x90U
#endif

/* Interrupt enables, DS41364B §4.5. */
#define PIC_REG_PIE1          0x91U
#define PIC_REG_PIE2          0x92U
#define PIC_REG_PIE3          0x93U

/* Core control, DS41364B §3.0 / §8.0 / §14.0. */
#define PIC_REG_OPTION        0x95U   /* OPTION_REG. */
#define PIC_REG_PCON          0x96U
#define PIC_REG_WDTCON        0x97U
#define PIC_REG_OSCTUNE       0x98U
#define PIC_REG_OSCCON        0x99U
#define PIC_REG_OSCSTAT       0x9AU

/* ADC, DS41364B §11.0. */
#define PIC_REG_ADRESL        0x9BU
#define PIC_REG_ADRESH        0x9CU
#define PIC_REG_ADCON0        0x9DU
#define PIC_REG_ADCON1        0x9EU

/* ───────────────────────── Bank 2 SFRs (0x10C-0x11F) ────────────── */

/* I/O latches, DS41364B §6.0 (LATx, the write target on this core). */
#define PIC_REG_LATA          0x10CU
#define PIC_REG_LATB          0x10DU
#define PIC_REG_LATC          0x10EU
#if PIC16F193X_FAMILY_HAS_PORTD
#define PIC_REG_LATD          0x10FU
#endif
#if PIC16F193X_FAMILY_HAS_PORTE
#define PIC_REG_LATE          0x110U
#endif

/* Comparator / DAC / FVR / SR latch / analog pin mux, DS41364B §9-§14. */
#define PIC_REG_CM1CON0       0x111U
#define PIC_REG_CM1CON1       0x112U
#define PIC_REG_CM2CON0       0x113U
#define PIC_REG_CM2CON1       0x114U
#define PIC_REG_CMOUT         0x115U
#define PIC_REG_BORCON        0x116U
#define PIC_REG_FVRCON        0x117U
#define PIC_REG_DACCON0       0x118U
#define PIC_REG_DACCON1       0x119U
#define PIC_REG_SRCON0        0x11AU
#define PIC_REG_SRCON1        0x11BU
#define PIC_REG_APFCON        0x11DU

/* ───────────────────────── Bank 3 SFRs (0x18C-0x19F) ────────────── */

/* Analog select, DS41364B §6.0 (ANSELx, per-pin analog/digital). */
#define PIC_REG_ANSELA        0x18CU
#define PIC_REG_ANSELB        0x18DU
#if PIC16F193X_FAMILY_HAS_PORTD
#define PIC_REG_ANSELD        0x18FU
#endif
#if PIC16F193X_FAMILY_HAS_PORTE
#define PIC_REG_ANSELE        0x190U
#endif

/* Data EEPROM + Flash self-write, DS41364B §23.0. */
#define PIC_REG_EEADRL        0x191U
#define PIC_REG_EEADRH        0x192U
#define PIC_REG_EEDATL        0x193U
#define PIC_REG_EEDATH        0x194U
#define PIC_REG_EECON1        0x195U
#define PIC_REG_EECON2        0x196U

/* EUSART, DS41364B §20.0. */
#define PIC_REG_RCREG         0x199U
#define PIC_REG_TXREG         0x19AU
#define PIC_REG_SPBRGL        0x19BU
#define PIC_REG_SPBRGH        0x19CU
#define PIC_REG_RCSTA         0x19DU
#define PIC_REG_TXSTA         0x19EU
#define PIC_REG_BAUDCON       0x19FU

/* ───────────────────────── Bank 4 SFRs (0x20C-0x21F) ────────────── */

/* Weak pull-ups, DS41364B §6.0 (WPUB per-pin PORTB, WPUE for PORTE). */
#define PIC_REG_WPUB          0x20DU
#define PIC_REG_WPUE          0x210U

/* MSSP, DS41364B §22.0. */
#define PIC_REG_SSPBUF        0x211U
#define PIC_REG_SSPADD        0x212U
#define PIC_REG_SSPMSK        0x213U
#define PIC_REG_SSPSTAT       0x214U
#define PIC_REG_SSPCON1       0x215U
#define PIC_REG_SSPCON2       0x216U
#define PIC_REG_SSPCON3       0x217U

/* ───────────────────────── Bank 5 SFRs (0x28C-0x29F) ────────────── */

/* CCP1/2 (ECCP), DS41364B §19.0. */
#define PIC_REG_CCPR1L        0x291U
#define PIC_REG_CCPR1H        0x292U
#define PIC_REG_CCP1CON       0x293U
#define PIC_REG_PWM1CON       0x294U
#define PIC_REG_CCP1AS        0x295U
#define PIC_REG_PSTR1CON      0x296U
#define PIC_REG_CCPR2L        0x298U
#define PIC_REG_CCPR2H        0x299U
#define PIC_REG_CCP2CON       0x29AU
#define PIC_REG_PWM2CON       0x29BU
#define PIC_REG_CCP2AS        0x29CU
#define PIC_REG_PSTR2CON      0x29DU
#define PIC_REG_CCPTMRS0      0x29EU
#define PIC_REG_CCPTMRS1      0x29FU

/* ───────────────────────── Bank 6 SFRs (0x30C-0x31F) ────────────── */

/* CCP3 (ECCP), CCP4, CCP5, DS41364B §19.0. */
#define PIC_REG_CCPR3L        0x311U
#define PIC_REG_CCPR3H        0x312U
#define PIC_REG_CCP3CON       0x313U
#define PIC_REG_PWM3CON       0x314U
#define PIC_REG_CCP3AS        0x315U
#define PIC_REG_PSTR3CON      0x316U
#define PIC_REG_CCPR4L        0x318U
#define PIC_REG_CCPR4H        0x319U
#define PIC_REG_CCP4CON       0x31AU
#define PIC_REG_CCPR5L        0x31CU
#define PIC_REG_CCPR5H        0x31DU
#define PIC_REG_CCP5CON       0x31EU

/* ───────────────────────── Bank 7 SFRs (0x38C-0x39F) ────────────── */

/* Interrupt-on-change (PORTB, per-pin), DS41364B §7.0. */
#define PIC_REG_IOCBP         0x394U
#define PIC_REG_IOCBN         0x395U
#define PIC_REG_IOCBF         0x396U

/* ───────────────────────── Bank 8 SFRs (0x40C-0x41F) ────────────── */

/* Timer4/Timer6, DS41364B §17.0. NOT documented in
 * docs/pic16f193x-plan.md §2's bank-map table (that table only covers
 * banks 0-7; banks 8-15 were assumed GPR/linear-only, which is wrong
 * for this bank, see docs/superpowers/plans/2026-08-04-pic16f193x-timer246.md's
 * "Known documentation gap" note). Confirmed via the installed DFP header
 * (xc8/pic/include/proc/pic16f1937.h): TMR4/PR4/T4CON and TMR6/PR6/T6CON
 * both live in bank 8, distinct from Timer2's bank-0 placement. */
#define PIC_REG_TMR4          0x415U
#define PIC_REG_PR4           0x416U
#define PIC_REG_T4CON         0x417U
#define PIC_REG_TMR6          0x41CU
#define PIC_REG_PR6           0x41DU
#define PIC_REG_T6CON         0x41EU

/* ───────────────────────── STATUS register bits ────────────────── */

/* DS41364B §2.2, Register 2-1. Enhanced Mid-range has no RP0/RP1/IRP
 * (BSR replaces them), so only C/DC/Z/PD/TO are defined. */
#define PIC_STATUS_C          PIC8_BIT(0)
#define PIC_STATUS_DC         PIC8_BIT(1)
#define PIC_STATUS_Z          PIC8_BIT(2)
#define PIC_STATUS_PD         PIC8_BIT(3)
#define PIC_STATUS_TO         PIC8_BIT(4)

/* ───────────────────────── INTCON register bits ────────────────── */

/* DS41364B §4.0, Register 4-1. IOCIF/IOCIE replace classic RBIF/RBIE. */
#define PIC_INTCON_IOCIF      PIC8_BIT(0)
#define PIC_INTCON_INTF       PIC8_BIT(1)
#define PIC_INTCON_TMR0IF     PIC8_BIT(2)
#define PIC_INTCON_IOCIE      PIC8_BIT(3)
#define PIC_INTCON_INTE       PIC8_BIT(4)
#define PIC_INTCON_TMR0IE     PIC8_BIT(5)
#define PIC_INTCON_PEIE       PIC8_BIT(6)
#define PIC_INTCON_GIE        PIC8_BIT(7)

/* ───────────────────────── OPTION_REG bits (Timer0 + WDT) ───────── */

/* DS41364B §2.2, Register 2-2. WPUEN (bit 7) is the global weak-pull-up
 * enable (active-low), replacing classic RBPU. */
#define PIC_OPTION_WPUEN      PIC8_BIT(7)
#define PIC_OPTION_INTEDG     PIC8_BIT(6)
#define PIC_OPTION_T0CS       PIC8_BIT(5)
#define PIC_OPTION_T0SE       PIC8_BIT(4)
#define PIC_OPTION_PSA        PIC8_BIT(3)
#define PIC_OPTION_PS_MASK    0x07U          /* PS2:PS0, prescaler ratio. */

/* ───────────────────────── PIR1 / PIE1 ─────────────────────────── */

/* DS41364B §4.5, Registers 4-2 (PIE1) / 4-5 (PIR1). */
#define PIC_PIR1_TMR1IF       PIC8_BIT(0)
#define PIC_PIR1_TMR2IF       PIC8_BIT(1)
#define PIC_PIR1_CCP1IF       PIC8_BIT(2)
#define PIC_PIR1_SSPIF       PIC8_BIT(3)
#define PIC_PIR1_TXIF         PIC8_BIT(4)
#define PIC_PIR1_RCIF         PIC8_BIT(5)
#define PIC_PIR1_ADIF         PIC8_BIT(6)
#define PIC_PIR1_TMR1GIF      PIC8_BIT(7)

#define PIC_PIE1_TMR1IE       PIC8_BIT(0)
#define PIC_PIE1_TMR2IE       PIC8_BIT(1)
#define PIC_PIE1_CCP1IE       PIC8_BIT(2)
#define PIC_PIE1_SSPIE        PIC8_BIT(3)
#define PIC_PIE1_TXIE         PIC8_BIT(4)
#define PIC_PIE1_RCIE         PIC8_BIT(5)
#define PIC_PIE1_ADIE         PIC8_BIT(6)
#define PIC_PIE1_TMR1GIE      PIC8_BIT(7)

/* ───────────────────────── PIR2 / PIE2 ─────────────────────────── */

/* DS41364B §4.5, Registers 4-3 (PIE2) / 4-6 (PIR2). */
#define PIC_PIR2_CCP2IF       PIC8_BIT(0)
#define PIC_PIR2_LCDIF        PIC8_BIT(2)
#define PIC_PIR2_BCLIF        PIC8_BIT(3)
#define PIC_PIR2_EEIF         PIC8_BIT(4)
#define PIC_PIR2_C1IF         PIC8_BIT(5)
#define PIC_PIR2_C2IF         PIC8_BIT(6)
#define PIC_PIR2_OSFIF        PIC8_BIT(7)

#define PIC_PIE2_CCP2IE       PIC8_BIT(0)
#define PIC_PIE2_LCDIE        PIC8_BIT(2)
#define PIC_PIE2_BCLIE        PIC8_BIT(3)
#define PIC_PIE2_EEIE        PIC8_BIT(4)
#define PIC_PIE2_C1IE         PIC8_BIT(5)
#define PIC_PIE2_C2IE         PIC8_BIT(6)
#define PIC_PIE2_OSFIE        PIC8_BIT(7)

/* ───────────────────────── PIR3 / PIE3 ─────────────────────────── */

/* DS41364B §4.5, Registers 4-4 (PIE3) / 4-7 (PIR3). */
#define PIC_PIR3_TMR4IF       PIC8_BIT(1)
#define PIC_PIR3_TMR6IF       PIC8_BIT(3)
#define PIC_PIR3_CCP3IF       PIC8_BIT(4)
#define PIC_PIR3_CCP4IF       PIC8_BIT(5)
#define PIC_PIR3_CCP5IF       PIC8_BIT(6)

#define PIC_PIE3_TMR4IE       PIC8_BIT(1)
#define PIC_PIE3_TMR6IE       PIC8_BIT(3)
#define PIC_PIE3_CCP3IE       PIC8_BIT(4)
#define PIC_PIE3_CCP4IE       PIC8_BIT(5)
#define PIC_PIE3_CCP5IE       PIC8_BIT(6)

/* ───────────────────────── PCON bits (reset flags) ─────────────── */

/* DS41364B §3.0, Register 3-3. */
#define PIC_PCON_BOR          PIC8_BIT(0)
#define PIC_PCON_POR          PIC8_BIT(1)
#define PIC_PCON_RI           PIC8_BIT(2)
#define PIC_PCON_RMCLR        PIC8_BIT(3)
#define PIC_PCON_STKUNF       PIC8_BIT(6)
#define PIC_PCON_STKOVF       PIC8_BIT(7)

/* ───────────────────────── WDTCON bits ────────────────────────── */

/* DS41364B §24.0, WDTCON register. SWDTEN (bit 0) is the software WDT
 * enable; WDTPS<4:0> (bits 5:1) the period select. */
#define PIC_WDTCON_SWDTEN     PIC8_BIT(0)
#define PIC_WDTCON_WDTPS_MASK 0x3EU          /* WDTPS4:WDTPS0, bits 5:1. */
#define PIC_WDTCON_WDTPS_POS  1U

/* ───────────────────────── T1CON bits (Timer1) ──────────────────── */

/* DS41364B Register 16-1. Verify each bit position and the POR
 * value against the datasheet before relying on them; the §4 gate
 * will catch any wrong literal.
 *
 * TMR1CS<1:0> (bits 7:6) selects the clock source (00 = FOSC/4,
 * 01 = FOSC, 10 = T1CKI pin/T1OSC, 11 = CAPOSC); bit 1 is
 * unimplemented. Everything but FOSC/4 (external clock, T1OSC,
 * CAPOSC) is out of scope for this phase (MANUAL.md §11 "Not in
 * this phase"), so no TMR1CS bit mask is defined here yet, and
 * HAL_TIMER1_Init/Start reject any ClockSource other than
 * TIMER1_CLOCK_INTERNAL. */
#define PIC_T1CON_TMR1ON        PIC8_BIT(0)   /* T1CON<0>. */
#define PIC_T1CON_T1CKPS0       PIC8_BIT(4)   /* T1CON<4>. */
#define PIC_T1CON_T1CKPS1       PIC8_BIT(5)   /* T1CON<5>. */

/* POR value of T1CON, DS41364B Register 16-1 POR column. */
#define PIC_T1CON_POR_VALUE      0x00U

/* ───────────────── T2CON / T4CON / T6CON bits (Timer2/4/6) ──────── */

/* DS41364B §17.0 (one register template documents Timer2/4/6 as a
 * group; the physical registers are T2CON/T4CON/T6CON, one instance
 * each). Cross-checked against the installed DFP header's
 * T2CON_T2CKPS_POSN/_MASK, T2CON_TMR2ON_POSN/_MASK,
 * T2CON_T2OUTPS_POSN/_MASK macros (and the T4CON/T6CON equivalents,
 * identical shape). Re-verify against DS41364B directly before relying
 * on these; the §4 gate is the actual verification floor, not this
 * comment.
 *
 * Layout, identical for all three registers:
 *   bit 7      unimplemented, reads 0
 *   bits 6:3   T*OUTPS<3:0>  postscaler select, 1:1 through 1:16, N+1
 *   bit 2      TMR*ON        timer enable
 *   bits 1:0   T*CKPS<1:0>   prescaler select: 00=1:1, 01=1:4, 1x=1:16
 * POR value: 0x00 for all three (DS41364B Register 17-1 POR column). */
#define PIC_T2CON_T2CKPS_MASK   0x03U         /* T2CON<1:0>. */
#define PIC_T2CON_TMR2ON        PIC8_BIT(2)   /* T2CON<2>. */
#define PIC_T2CON_TOUTPS_MASK   0x78U         /* T2CON<6:3>. */
#define PIC_T2CON_TOUTPS_POS    3U
#define PIC_T2CON_POR_VALUE     0x00U

#define PIC_T4CON_T4CKPS_MASK   0x03U         /* T4CON<1:0>. */
#define PIC_T4CON_TMR4ON        PIC8_BIT(2)   /* T4CON<2>. */
#define PIC_T4CON_TOUTPS_MASK   0x78U         /* T4CON<6:3>. */
#define PIC_T4CON_TOUTPS_POS    3U
#define PIC_T4CON_POR_VALUE     0x00U

#define PIC_T6CON_T6CKPS_MASK   0x03U         /* T6CON<1:0>. */
#define PIC_T6CON_TMR6ON        PIC8_BIT(2)   /* T6CON<2>. */
#define PIC_T6CON_TOUTPS_MASK   0x78U         /* T6CON<6:3>. */
#define PIC_T6CON_TOUTPS_POS    3U
#define PIC_T6CON_POR_VALUE     0x00U

/* ───────────────── CCP1/CCP2 bits (DS41364B §15.0) ──────────────── */

/* Both instances are Enhanced CCP on this device. CCPxCON layout
 * (DS41364B Register 15-1/15-2): bits 3:0 = CCPxM mode select, bits
 * 5:4 = DCxB PWM duty LSBs (PWM-only this phase), bits 7:6 = PxE
 * enhanced PWM output config (PWM-only this phase). Cross-checked
 * against the DFP header's _CCP1CON_CCP1M_POSN/_MASK (bits 3:0,
 * mask 0xF), _CCP1CON_DC1B_POSN/_MASK (bits 5:4, mask 0x30),
 * _CCP1CON_P1M_POSN/_MASK (bits 7:6, mask 0xC0). */
#define PIC_CCP1CON_CCPM_MASK   0x0FU   /* CCP1CON<3:0>. */
#define PIC_CCP1CON_DCB_MASK    0x30U   /* CCP1CON<5:4>. */
#define PIC_CCP1CON_PM_MASK     0xC0U   /* CCP1CON<7:6>. */
#define PIC_CCP2CON_CCPM_MASK   0x0FU
#define PIC_CCP2CON_DCB_MASK    0x30U
#define PIC_CCP2CON_PM_MASK     0xC0U

#define PIC_CCP3CON_CCPM_MASK   0x0FU
#define PIC_CCP3CON_DCB_MASK    0x30U
#define PIC_CCP3CON_PM_MASK     0xC0U

#define PIC_CCP4CON_CCPM_MASK   0x0FU
#define PIC_CCP4CON_DCB_MASK    0x30U

#define PIC_CCP5CON_CCPM_MASK   0x0FU
#define PIC_CCP5CON_DCB_MASK    0x30U

/* PWM1CON/PWM2CON, CCP1AS/CCP2AS, PSTR1CON/PSTR2CON: PWM-only this
 * phase; macros defined for completeness and the CCP3/4/5 follow-up. */
#define PIC_PWM1CON_P1DC_MASK   0x7FU
#define PIC_PWM1CON_P1RSEN      PIC8_BIT(7)
#define PIC_PWM2CON_P2DC_MASK   0x7FU
#define PIC_PWM2CON_P2RSEN      PIC8_BIT(7)
#define PIC_CCP1AS_PSS1BD_MASK  0x03U
#define PIC_CCP1AS_PSS1AC_MASK  0x0CU
#define PIC_CCP1AS_CCP1AS_MASK  0x70U
#define PIC_CCP1AS_ECCP1ASE     PIC8_BIT(7)
#define PIC_CCP2AS_PSS2BD_MASK  0x03U
#define PIC_CCP2AS_PSS2AC_MASK  0x0CU
#define PIC_CCP2AS_CCP2AS_MASK  0x70U
#define PIC_CCP2AS_ECCP2ASE     PIC8_BIT(7)
#define PIC_PSTR1CON_STR1A      PIC8_BIT(0)
#define PIC_PSTR1CON_STR1B      PIC8_BIT(1)
#define PIC_PSTR1CON_STR1C      PIC8_BIT(2)
#define PIC_PSTR1CON_STR1D      PIC8_BIT(3)
#define PIC_PSTR1CON_STR1SYNC   PIC8_BIT(4)
#define PIC_PSTR2CON_STR2A      PIC8_BIT(0)
#define PIC_PSTR2CON_STR2B      PIC8_BIT(1)
#define PIC_PSTR2CON_STR2C      PIC8_BIT(2)
#define PIC_PSTR2CON_STR2D      PIC8_BIT(3)
#define PIC_PSTR2CON_STR2SYNC   PIC8_BIT(4)

/* CCPTMRS0/1: PWM timebase select, PWM-only (also needed by CCP3/4/5). */
#define PIC_CCPTMRS0_C1TSEL_MASK 0x03U
#define PIC_CCPTMRS0_C2TSEL_MASK 0x0CU
#define PIC_CCPTMRS0_C3TSEL_MASK 0x30U
#define PIC_CCPTMRS0_C4TSEL_MASK 0xC0U
#define PIC_CCPTMRS1_C5TSEL_MASK 0x03U

/* ───────────────── EUSART bits (DS41364B §23.0) ─────────────────── */

/* RCSTA/TXSTA/BAUDCON, DS41364B Register 23-2/23-3/23-4. Cross-checked
 * against the DFP header: _RCSTA_SPEN_POSN=7, _RCSTA_CREN_POSN=4,
 * _TXSTA_TXEN_POSN=5, _TXSTA_BRGH_POSN=2, _TXSTA_TRMT_POSN=1. */
#define PIC_RCSTA_RX9D          PIC8_BIT(0)
#define PIC_RCSTA_OERR          PIC8_BIT(1)
#define PIC_RCSTA_FERR          PIC8_BIT(2)
#define PIC_RCSTA_ADDEN         PIC8_BIT(3)
#define PIC_RCSTA_CREN          PIC8_BIT(4)
#define PIC_RCSTA_SREN          PIC8_BIT(5)
#define PIC_RCSTA_RX9           PIC8_BIT(6)
#define PIC_RCSTA_SPEN          PIC8_BIT(7)

#define PIC_TXSTA_TX9D          PIC8_BIT(0)
#define PIC_TXSTA_TRMT          PIC8_BIT(1)
#define PIC_TXSTA_BRGH          PIC8_BIT(2)
#define PIC_TXSTA_SENDB         PIC8_BIT(3)
#define PIC_TXSTA_SYNC          PIC8_BIT(4)
#define PIC_TXSTA_TXEN          PIC8_BIT(5)
#define PIC_TXSTA_TX9           PIC8_BIT(6)
#define PIC_TXSTA_CSRC          PIC8_BIT(7)

#define PIC_BAUDCON_ABDEN       PIC8_BIT(0)
#define PIC_BAUDCON_WUE         PIC8_BIT(1)
#define PIC_BAUDCON_BRG16       PIC8_BIT(3)
#define PIC_BAUDCON_SCKP        PIC8_BIT(4)
#define PIC_BAUDCON_RCIDL       PIC8_BIT(6)
#define PIC_BAUDCON_ABDOVF      PIC8_BIT(7)

/* POR values: TXSTA=0x02 (TRMT=1), RCSTA=0x00, BAUDCON=0x40 (RCIDL=1).
 * DS41364B Register 23-2/23-3/23-4 POR columns. */
#define PIC_TXSTA_POR_VALUE     0x02U
#define PIC_RCSTA_POR_VALUE     0x00U
#define PIC_BAUDCON_POR_VALUE   0x40U

/* ───────────────── MSSP bits (DS41364B §22.0, SPI subset) ──────── */

/* SSPSTAT/SSPCON1, SPI-relevant subset only (SSPCON2/3 are I2C-only).
 * Cross-checked against DFP: _SSPSTAT_BF_POSN=0, _SSPSTAT_CKE_POSN=6,
 * _SSPSTAT_SMP_POSN=7, _SSPCON1_SSPM_POSN=0, _SSPCON1_CKP_POSN=4,
 * _SSPCON1_SSPEN_POSN=5, _SSPCON1_WCOL_POSN=7. */
#define PIC_SSPSTAT_BF          PIC8_BIT(0)
#define PIC_SSPSTAT_CKE         PIC8_BIT(6)
#define PIC_SSPSTAT_SMP         PIC8_BIT(7)

#define PIC_SSPCON1_SSPM_MASK   0x0FU
#define PIC_SSPCON1_CKP         PIC8_BIT(4)
#define PIC_SSPCON1_SSPEN       PIC8_BIT(5)
#define PIC_SSPCON1_SSPOV       PIC8_BIT(6)
#define PIC_SSPCON1_WCOL        PIC8_BIT(7)

/* ───────────────── ADC bits (DS41364B ADC chapter) ──────────────── */

/* ADCON0/ADCON1. Cross-checked against DFP: _ADCON0_ADON_POSN=0,
 * _ADCON0_GO_POSN=1, _ADCON0_CHS_POSN=2 (mask 0x7C, 5-bit),
 * _ADCON1_ADPREF_POSN=0 (mask 0x03), _ADCON1_ADNREF_POSN=2,
 * _ADCON1_ADCS_POSN=4 (mask 0x70), _ADCON1_ADFM_POSN=7. */
#define PIC_ADCON0_ADON         PIC8_BIT(0)
#define PIC_ADCON0_GO_NDONE     PIC8_BIT(1)
#define PIC_ADCON0_CHS_MASK     0x7CU
#define PIC_ADCON0_CHS_SHIFT    2U

#define PIC_ADCON1_ADPREF_MASK  0x03U
#define PIC_ADCON1_ADNREF       PIC8_BIT(2)
#define PIC_ADCON1_ADCS_MASK    0x70U
#define PIC_ADCON1_ADCS_SHIFT   4U
#define PIC_ADCON1_ADFM         PIC8_BIT(7)

/* ───────────────── Comparator bits (DS41364B §9.0) ──────────────── */

/* CM1CON0/CM2CON0: CxON(7), CxOE(5), CxPOL(4), CxHYS(1).
 * CM1CON1/CM2CON1: CxPCH(5:4, mask 0x30), CxNCH(1:0, mask 0x03),
 * CxINTN(6), CxINTP(7).
 * CMOUT: MC1OUT(0), MC2OUT(1). */
#define PIC_CM1CON0_C1HYS       PIC8_BIT(1)
#define PIC_CM1CON0_C1POL       PIC8_BIT(4)
#define PIC_CM1CON0_C1OE        PIC8_BIT(5)
#define PIC_CM1CON0_C1ON        PIC8_BIT(7)

#define PIC_CM1CON1_C1NCH_MASK  0x03U
#define PIC_CM1CON1_C1PCH_MASK  0x30U
#define PIC_CM1CON1_C1INTN      PIC8_BIT(6)
#define PIC_CM1CON1_C1INTP      PIC8_BIT(7)

#define PIC_CM2CON0_C2HYS       PIC8_BIT(1)
#define PIC_CM2CON0_C2POL       PIC8_BIT(4)
#define PIC_CM2CON0_C2OE        PIC8_BIT(5)
#define PIC_CM2CON0_C2ON        PIC8_BIT(7)

#define PIC_CM2CON1_C2NCH_MASK  0x03U
#define PIC_CM2CON1_C2PCH_MASK  0x30U
#define PIC_CM2CON1_C2INTN      PIC8_BIT(6)
#define PIC_CM2CON1_C2INTP      PIC8_BIT(7)

#define PIC_CMOUT_MC1OUT        PIC8_BIT(0)
#define PIC_CMOUT_MC2OUT        PIC8_BIT(1)

/* ───────────────── EEPROM bits (DS41364B §23.0) ──────────────────── */

#define PIC_EECON1_RD           PIC8_BIT(0)
#define PIC_EECON1_WR           PIC8_BIT(1)
#define PIC_EECON1_WREN         PIC8_BIT(2)
#define PIC_EECON1_WRERR        PIC8_BIT(3)
#define PIC_EECON1_FREE         PIC8_BIT(4)
#define PIC_EECON1_LWLO         PIC8_BIT(5)
#define PIC_EECON1_CFGS         PIC8_BIT(6)
#define PIC_EECON1_EEPGD        PIC8_BIT(7)

#define PIC_EEADRH_MASK         0x7FU
#define PIC_EEDATH_MASK         0x3FU

/* ───────────────── DAC bits (DS41364B §13.0) ───────────────────── */
#define PIC_DACCON0_DACNSS     PIC8_BIT(0)
#define PIC_DACCON0_DACPSS_MASK 0x0CU
#define PIC_DACCON0_DACOE      PIC8_BIT(5)
#define PIC_DACCON0_DACLPS     PIC8_BIT(6)
#define PIC_DACCON0_DACEN      PIC8_BIT(7)
#define PIC_DACCON1_DACR_MASK  0x1FU

/* ───────────────── FVR bits (DS41364B §12.0) ───────────────────── */
#define PIC_FVRCON_ADFVR_MASK   0x03U
#define PIC_FVRCON_CDAFVR_MASK  0x0CU
#define PIC_FVRCON_CDAFVR_SHIFT 2U
#define PIC_FVRCON_TSRNG       PIC8_BIT(4)
#define PIC_FVRCON_TSEN        PIC8_BIT(5)
#define PIC_FVRCON_FVRRDY      PIC8_BIT(6)
#define PIC_FVRCON_FVREN       PIC8_BIT(7)

/* ───────────────── SR latch bits (DS41364B §11.0) ──────────────── */
#define PIC_SRCON0_SRPR        PIC8_BIT(0)
#define PIC_SRCON0_SRPS        PIC8_BIT(1)
#define PIC_SRCON0_SRNQEN      PIC8_BIT(2)
#define PIC_SRCON0_SRQEN       PIC8_BIT(3)
#define PIC_SRCON0_SRCLK_MASK  0x70U
#define PIC_SRCON0_SRLEN       PIC8_BIT(7)
#define PIC_SRCON1_SRRC1E      PIC8_BIT(0)
#define PIC_SRCON1_SRRC2E      PIC8_BIT(1)
#define PIC_SRCON1_SRNQEN      PIC8_BIT(2)
#define PIC_SRCON1_SRQEN       PIC8_BIT(3)

/* ───────────────── CPS bits (DS41364B §18.0) ───────────────────── */
#define PIC_CPSCON0_T0XCS      PIC8_BIT(0)
#define PIC_CPSCON0_CPSOUT     PIC8_BIT(1)
#define PIC_CPSCON0_CPSRNG_MASK 0x0CU
#define PIC_CPSCON0_CPSON      PIC8_BIT(7)
#define PIC_CPSCON1_CPSCH_MASK 0x0FU

/* ───────────────────────── Reset values (POR) ──────────────────── */

/* DS41364B §3 reset values and the per-register POR columns of the
 * Table 2-4 register summary. */
#define PIC_STATUS_POR_VALUE     0x18U  /* ---1 1000: TO=1, PD=1. */
#define PIC_PCON_POR_VALUE       0x0CU  /* RMCLR=1, RI=1; POR=0, BOR unknown. */
#define PIC_INTCON_POR_VALUE     0x00U  /* 0000 000x; IOCIF unknown. */
#define PIC_OPTION_POR_VALUE     0xFFU
#define PIC_PIR1_POR_VALUE       0x00U
#define PIC_PIR2_POR_VALUE       0x00U
#define PIC_PIR3_POR_VALUE       0x00U
#define PIC_PIE1_POR_VALUE       0x00U
#define PIC_PIE2_POR_VALUE       0x00U
#define PIC_PIE3_POR_VALUE       0x00U

/* ───────────────────────── Bank-selection helper ───────────────── */

/**
 * @brief  Load the Bank Select Register (BSR) with `bank`. DS41364B §2.2.
 *
 * @details
 *   On the Enhanced Mid-range core XC8 auto-banks every literal SFR
 *   access (it knows each SFR's bank and emits the bank select), so
 *   drivers normally never call this. It is provided for the cases that
 *   do need an explicit bank: indirect/linear-data-memory setup via FSR,
 *   and any hand-written sequence that must hold a bank across several
 *   SFR touches. Writing BSR is a plain core-register write (BSR is in
 *   the 0x00-0x0B core region mirrored in every bank), no inline asm.
 *
 *   Whether an SFR access made while a C-level local/parameter is live
 *   misdirects on this core (the classic-PIC16 RP0/RP1 codegen failure,
 *   see pic16f87xa-hal/docs/ARCHITECTURE.md) is NOT assumed away here:
 *   it is verified by the §4 codegen probe before any driver relies on
 *   it, per docs/adding-a-device.md. Until then every SFR access stays a
 *   compile-time-constant `PIC_REG_*` token with no runtime dispatch.
 */
#define pic16f193x_select_bank(bank)   (PIC8_REG8(PIC_REG_BSR) = (uint8_t)(bank))

#endif /* PIC16F193X_SFR_H */
