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
