/*
 * SFR address map for PIC18F1320 (foundation subset: core, GPIO,
 * interrupts, Timer0; grows per ticket). Every address, mask, and
 * reset value is 1-to-1 from DS39605F and the DFP header; addresses
 * match pic18fxx5x-hal's map (same Access Bank shape, far smaller GPR
 * window). Only PORTA/PORTB exist; no MSSP, CCP2, comparator, BAUDCON.
 */

#ifndef PIC18F1320_SFR_H
#define PIC18F1320_SFR_H

#include "pic18f1320.h"

/* Core CPU status, DS39605F Register 5-2. */
#define PIC_REG_STATUS        0xFD8U   /**< ALU status (N/OV/Z/DC/C).       */
#define PIC_REG_BSR           0xFE0U   /**< Bank Select Register (low nibble). */
#define PIC_REG_RCON          0xFD0U   /**< Reset Control (IPEN/TO/PD/POR/BOR). */

/* I/O ports, DS39605F §5.0/§6.0, Table 5-1.
 * PORTx is the pin input sample; LATx is the output latch; TRISx is the
 * data-direction register. PIC18 exposes LATx as its own mapped register,
 * so GPIO writes go through LATx, not PORTx. Both PORTA and PORTB are
 * full 8-bit on this part (no PORTC/D/E, confirmed absent from the DFP
 * header). */
#define PIC_REG_PORTA         0xF80U
#define PIC_REG_PORTB         0xF81U
#define PIC_REG_LATA          0xF89U
#define PIC_REG_LATB          0xF8AU
#define PIC_REG_TRISA         0xF92U
#define PIC_REG_TRISB         0xF93U

/* Interrupt control, DS39605F §9.0, Register 9-1/9-2/9-3. */
#define PIC_REG_INTCON        0xFF2U   /**< GIEH/GIEL/TMR0IE/INT0IE/RBIE + flags. */
#define PIC_REG_INTCON2       0xFF1U   /**< Pull-ups, INT edge, TMR0IP, RBIP.   */
#define PIC_REG_INTCON3       0xFF0U   /**< INT1/INT2 enable, flag, priority.   */

/* Peripheral interrupt flag/enable/priority, DS39605F Register 9-5/9-6/9-8.
 * Bit 3 (SSPIF/IE/IP, no MSSP) and bit 7 (SPPIF/IE/IP, no SPP) are
 * unimplemented on this part, confirmed against the DFP header's
 * PIR1bits_t/PIE1bits_t layout. */
#define PIC_REG_PIR1          0xF9EU   /**< Peripheral Interrupt Flag Register 1. */
#define PIC_REG_PIE1          0xF9DU   /**< Peripheral Interrupt Enable Reg 1.    */
#define PIC_REG_IPR1          0xF9FU   /**< Peripheral Interrupt Priority Reg 1.  */
/* PIR2 layout differs from pic18fxx5x: bit0 (CCP2IF) and bit3 (BCLIF, no
 * MSSP) are unimplemented, bit2 is LVDIF (not HLVDIF) and bit5/6 (USBIF,
 * CMIF) are unimplemented (no USB, no comparator), per the DFP header's
 * PIR2bits_t. TMR3IF (bit1), EEIF (bit4), OSCFIF (bit7) share 4550's bit
 * positions. */
#define PIC_REG_PIR2          0xFA1U   /**< Peripheral Interrupt Flag Register 2. */
#define PIC_REG_PIE2          0xFA0U   /**< Peripheral Interrupt Enable Reg 2.    */
#define PIC_REG_IPR2          0xFA2U   /**< Peripheral Interrupt Priority Reg 2 (reset 0xFF, all high). */

/* Timer0, DS39605F §10.0, Register 10-1. */
#define PIC_REG_T0CON         0xFD5U   /**< Timer0 control (on/8-16bit/src/edge/PSA/PS). */
#define PIC_REG_TMR0L         0xFD6U   /**< Timer0 low byte (also 8-bit mode value). */
#define PIC_REG_TMR0H         0xFD7U   /**< Timer0 high byte (16-bit mode only).    */

/* Timer1, DS39605F §12.0, Register 12-1 (T1CON 0xFCD, same shape as 4550). */
#define PIC_REG_T1CON         0xFCDU   /**< Timer1 control (RD16/run/prescale/osc/sync/cs/on). */
#define PIC_REG_TMR1L         0xFCEU   /**< Timer1 low byte.                          */
#define PIC_REG_TMR1H         0xFCFU   /**< Timer1 high byte.                         */

/* Timer2, DS39605F §13.0, Register 13-1 (T2CON 0xFCA, same shape as 4550). */
#define PIC_REG_T2CON         0xFCAU   /**< Timer2 control (postscaler/on/prescaler). */
#define PIC_REG_PR2           0xFCBU   /**< Timer2 period register (Access Bank).    */
#define PIC_REG_TMR2          0xFCCU   /**< Timer2 counter.                          */

/* Timer3, DS39605F §14.0, Register 14-1 (T3CON 0xFB1, same shape as 4550). */
#define PIC_REG_T3CON         0xFB1U   /**< Timer3 control (RD16/CCP-sel/prescale/sync/cs/on). */
#define PIC_REG_TMR3L         0xFB2U   /**< Timer3 low byte.                          */
#define PIC_REG_TMR3H         0xFB3U   /**< Timer3 high byte.                         */

/* Enhanced Capture/Compare/PWM (ECCP1), DS39605F §15.0, Register 15-1/15-2/15-3.
 * Enhanced ECCP1 (P1M multi-output + auto-shutdown), same shape as 4550's
 * ECCP1; no CCP2 on this part, so only the single CCP1CON/CCPR1 pair. */
#define PIC_REG_ECCPAS        0xFB6U   /**< ECCP1 auto-shutdown control/status.    */
#define PIC_REG_PWM1CON       0xFB7U   /**< ECCP1 PWM config (PDC dead-band, PRSEN). */
#define PIC_REG_CCP1CON       0xFBDU   /**< ECCP1 control (P1M/DC1B/CCP1M).         */
#define PIC_REG_CCPR1L        0xFBEU   /**< ECCP1 low byte (capture/compare/PWM LSBs). */
#define PIC_REG_CCPR1H        0xFBFU   /**< ECCP1 high byte.                       */

/* EUSART, DS39605F §16.0. RCSTA/TXSTA/TXREG/RCREG/SPBRG/SPBRGH share the
 * 4550 shape; the baud control register is BAUDCTL (0xFAA, DS39605F
 * Register 16-3), not BAUDCON, and BAUDCTL has no ABDOVF bit (bit 7 is
 * unimplemented on this part, confirmed in Register 16-3 and the DFP). */
#define PIC_REG_BAUDCTL       0xFAAU   /**< EUSART baud-rate control (BRG16/ABDEN/WUE/...). */
#define PIC_REG_RCSTA         0xFABU   /**< EUSART receive control/status.        */
#define PIC_REG_TXSTA         0xFACU   /**< EUSART transmit control/status.        */
#define PIC_REG_TXREG         0xFADU   /**< EUSART transmit data.                 */
#define PIC_REG_RCREG         0xFAEU   /**< EUSART receive data.                  */
#define PIC_REG_SPBRG         0xFAFU   /**< EUSART baud-rate divisor, low byte.    */
#define PIC_REG_SPBRGH        0xFB0U   /**< EUSART baud-rate divisor, high byte (BRG16=1). */

/* Data EEPROM, DS39605F §7.0 (256 bytes). Same Access Bank addresses and
 * EECON1 bit layout as the 4550. */
#define PIC_REG_EECON1        0xFA6U   /**< EEPROM/Flash control (RD/WR/WREN/WRERR/EEPGD). */
#define PIC_REG_EECON2        0xFA7U   /**< EEPROM unlock (write 0x55 then 0xAA).       */
#define PIC_REG_EEDATA        0xFA8U   /**< EEPROM data register.                      */
#define PIC_REG_EEADR         0xFA9U   /**< EEPROM address register (8-bit, 0..255).   */

/* A/D Converter, DS39605F §17.0 (10-bit, 7 channels AN0-6). The 1320's ADC
 * has a genuinely different register layout from the 4550: VCFG1:VCFG0 live
 * in ADCON0 bits 7:6 (not ADCON1), CHS is 3 bits (2:0 in ADCON0 bits 4:2,
 * 7 channels), and ADCON1 carries a 7-bit per-pin PCFG (bits 6:0) with no
 * VCFG. ADCON2 matches the 4550 (ADFM/ACQT/ADCS). */
#define PIC_REG_ADCON2        0xFC0U   /**< A/D control 2 (ADFM/ACQT/ADCS).       */
#define PIC_REG_ADCON1        0xFC1U   /**< A/D control 1 (PCFG6:PCFG0).          */
#define PIC_REG_ADCON0        0xFC2U   /**< A/D control 0 (VCFG/CHS/GO-DONE/ADON). */
#define PIC_REG_ADRESL        0xFC3U   /**< A/D result low byte.                  */
#define PIC_REG_ADRESH        0xFC4U   /**< A/D result high byte.                 */

/* STATUS bits (Register 5-2). */
#define PIC_STATUS_N          EPIC_BIT(4)   /**< Negative / borrow complement. */
#define PIC_STATUS_OV         EPIC_BIT(3)   /**< Overflow.                     */
#define PIC_STATUS_Z          EPIC_BIT(2)   /**< Zero.                         */
#define PIC_STATUS_DC         EPIC_BIT(1)   /**< Digit carry/borrow.           */
#define PIC_STATUS_C          EPIC_BIT(0)   /**< Carry/borrow.                 */

/* RCON bits (Register 4-1). Bit 6 (SBOREN on pic18fxx5x-hal's 4550/2455)
 * is unimplemented on this part: confirmed absent from the DFP header's
 * own _RCON_*_POSN macros (only bits 0-4 and 7 exist), not assumed. */
#define PIC_RCON_IPEN         EPIC_BIT(7)   /**< Interrupt Priority Enable.    */
#define PIC_RCON_RI           EPIC_BIT(4)   /**< RESET instruction flag.       */
#define PIC_RCON_TO           EPIC_BIT(3)   /**< WDT time-out flag (1=not).    */
#define PIC_RCON_PD           EPIC_BIT(2)   /**< Power-down (Sleep) flag (1=not). */
#define PIC_RCON_POR          EPIC_BIT(1)   /**< Power-on Reset status.        */
#define PIC_RCON_BOR          EPIC_BIT(0)   /**< Brown-out Reset status.       */

/* INTCON bits (Register 9-1). */
#define PIC_INTCON_GIE        EPIC_BIT(7)   /**< Global Int Enable / high-priority GIE. */
#define PIC_INTCON_GIEH       PIC_INTCON_GIE
#define PIC_INTCON_PEIE       EPIC_BIT(6)   /**< Peripheral Int Enable / low-priority GIEL. */
#define PIC_INTCON_GIEL       PIC_INTCON_PEIE
#define PIC_INTCON_TMR0IE     EPIC_BIT(5)   /**< Timer0 overflow interrupt enable. */
#define PIC_INTCON_INT0IE     EPIC_BIT(4)   /**< INT0 external interrupt enable.   */
#define PIC_INTCON_RBIE       EPIC_BIT(3)   /**< RB<7:4> change interrupt enable.  */
#define PIC_INTCON_TMR0IF     EPIC_BIT(2)   /**< Timer0 overflow interrupt flag.   */
#define PIC_INTCON_INT0IF     EPIC_BIT(1)   /**< INT0 external interrupt flag.     */
#define PIC_INTCON_RBIF       EPIC_BIT(0)   /**< RB<7:4> change interrupt flag.    */

/* INTCON2 bits (Register 9-2). */
#define PIC_INTCON2_RBPU      EPIC_BIT(7)   /**< PORTB pull-up enable (active-low). */
#define PIC_INTCON2_INTEDG0   EPIC_BIT(6)   /**< INT0 edge select (1=rising).       */
#define PIC_INTCON2_INTEDG1   EPIC_BIT(5)   /**< INT1 edge select.                  */
#define PIC_INTCON2_INTEDG2   EPIC_BIT(4)   /**< INT2 edge select.                  */
#define PIC_INTCON2_TMR0IP    EPIC_BIT(2)   /**< Timer0 overflow priority (1=high). */
#define PIC_INTCON2_RBIP      EPIC_BIT(0)   /**< RB change priority (1=high).       */

/* INTCON3 bits (Register 9-3). */
#define PIC_INTCON3_INT2IP    EPIC_BIT(7)   /**< INT2 priority (1=high).            */
#define PIC_INTCON3_INT1IP    EPIC_BIT(6)   /**< INT1 priority (1=high).            */
#define PIC_INTCON3_INT2IE    EPIC_BIT(4)   /**< INT2 external interrupt enable.    */
#define PIC_INTCON3_INT1IE    EPIC_BIT(3)   /**< INT1 external interrupt enable.    */
#define PIC_INTCON3_INT2IF    EPIC_BIT(1)   /**< INT2 external interrupt flag.      */
#define PIC_INTCON3_INT1IF    EPIC_BIT(0)   /**< INT1 external interrupt flag.      */

/* PIR1 / PIE1 / IPR1 bits (Reg 9-5/9-6/9-8). Bits 3 (SSP) and 7 (SPP) are
 * unimplemented on this part; no macro is defined for them. */
#define PIC_PIR1_ADIF         EPIC_BIT(6)   /**< A/D conversion complete flag.  */
#define PIC_PIR1_RCIF         EPIC_BIT(5)   /**< USART receive flag.            */
#define PIC_PIR1_TXIF         EPIC_BIT(4)   /**< USART transmit flag.           */
#define PIC_PIR1_CCP1IF       EPIC_BIT(2)   /**< ECCP1 flag.                    */
#define PIC_PIR1_TMR2IF       EPIC_BIT(1)   /**< Timer2 match flag.             */
#define PIC_PIR1_TMR1IF       EPIC_BIT(0)   /**< Timer1 overflow flag.          */
#define PIC_PIE1_ADIE         EPIC_BIT(6)
#define PIC_PIE1_RCIE         EPIC_BIT(5)
#define PIC_PIE1_TXIE         EPIC_BIT(4)
#define PIC_PIE1_CCP1IE       EPIC_BIT(2)
#define PIC_PIE1_TMR2IE       EPIC_BIT(1)
#define PIC_PIE1_TMR1IE       EPIC_BIT(0)
#define PIC_IPR1_ADIP         EPIC_BIT(6)
#define PIC_IPR1_RCIP         EPIC_BIT(5)
#define PIC_IPR1_TXIP         EPIC_BIT(4)
#define PIC_IPR1_CCP1IP       EPIC_BIT(2)
#define PIC_IPR1_TMR2IP       EPIC_BIT(1)
#define PIC_IPR1_TMR1IP       EPIC_BIT(0)

/* T0CON bits (Register 10-1). */
#define PIC_T0CON_TMR0ON      EPIC_BIT(7)   /**< Timer0 on/off.                  */
#define PIC_T0CON_T08BIT      EPIC_BIT(6)   /**< 1=8-bit, 0=16-bit mode.         */
#define PIC_T0CON_T0CS        EPIC_BIT(5)   /**< 0=internal Fosc/4, 1=T0CKI pin. */
#define PIC_T0CON_T0SE        EPIC_BIT(4)   /**< Counter mode edge select.       */
#define PIC_T0CON_PSA         EPIC_BIT(3)   /**< 0=prescaler assigned, 1=not.    */
#define PIC_T0CON_T0PS_MASK   0x07U         /**< T0PS2:T0PS0, prescaler ratio.   */

/* T1CON bits (Register 12-1, identical to 4550). */
#define PIC_T1CON_RD16        EPIC_BIT(7)   /**< 16-bit read/write mode enable.   */
#define PIC_T1CON_T1RUN       EPIC_BIT(6)   /**< Timer1 system clock status (RO). */
#define PIC_T1CON_T1CKPS_MASK 0x30U         /**< T1CKPS1:T1CKPS0 at bits 5:4.     */
#define PIC_T1CON_T1OSCEN     EPIC_BIT(3)   /**< Timer1 oscillator enable.       */
#define PIC_T1CON_T1SYNC      EPIC_BIT(2)   /**< External clock sync (1=async).  */
#define PIC_T1CON_TMR1CS      EPIC_BIT(1)   /**< 0=Fosc/4, 1=external/T1OSC.    */
#define PIC_T1CON_TMR1ON      EPIC_BIT(0)   /**< Timer1 on/off.                  */
#define PIC_T1CON_POR_VALUE   0x00U         /**< Power-on reset value.           */

/* T2CON bits (Register 13-1, identical to 4550). */
#define PIC_T2CON_TOUTPS_MASK 0x78U         /**< T2OUTPS3:T2OUTPS0 at bits 6:3 (1:(N+1)). */
#define PIC_T2CON_TMR2ON      EPIC_BIT(2)   /**< Timer2 on/off.                  */
#define PIC_T2CON_T2CKPS_MASK 0x03U         /**< T2CKPS1:T2CKPS0 at bits 1:0.    */
#define PIC_T2CON_POR_VALUE   0x00U         /**< Power-on reset value.           */
#define PIC_PR2_POR_VALUE     0xFFU         /**< PR2 power-on reset value.       */

/* T3CON bits (Register 14-1). Bit 6 (T3CCP2 on 4550/2520) is
 * unimplemented on this part: no CCP2 (single ECCP1), so the 1320's
 * T3CON<6> reads 0 regardless, per Register 14-1's "Unimplemented, read
 * as 0" cell (DS39605F §14.0). The register value 0x40 written via the
 * 4550-shaped PIC_T3CON_T3CCP2 macro is harmless but defined out here. */
#define PIC_T3CON_T3CCP1      EPIC_BIT(3)   /**< CCP1 Timer1/Timer3 select.      */
#define PIC_T3CON_RD16        EPIC_BIT(7)   /**< 16-bit read/write mode enable.  */
#define PIC_T3CON_T3CKPS_MASK 0x30U         /**< T3CKPS1:T3CKPS0 at bits 5:4.    */
#define PIC_T3CON_T3SYNC      EPIC_BIT(2)   /**< External clock sync (alias).    */
#define PIC_T3CON_TMR3CS      EPIC_BIT(1)   /**< 0=Fosc/4, 1=external.           */
#define PIC_T3CON_TMR3ON      EPIC_BIT(0)   /**< Timer3 on/off.                  */
#define PIC_T3CON_POR_VALUE   0x00U         /**< Power-on reset value.           */

/* CCP1CON (ECCP1) bits, DS39605F Register 15-1. */
#define PIC_CCP1_P1M_MASK     0xC0U    /**< P1M1:P1M0 at bits 7:6 (output mode). */
#define PIC_CCP1_DC1B_MASK    0x30U    /**< DC1B1:DC1B0 at bits 5:4 (duty LSBs). */
#define PIC_CCP1_M_MASK       0x0FU    /**< CCP1M3:CCP1M0 at bits 3:0 (mode).    */
#define PIC_CCP1CON_POR_VALUE 0x00U

/* PWM1CON bits, DS39605F Register 15-2 (the 1320's ECCP1 dead-band +
 * auto-restart register, named PWM1CON not ECCP1DEL). */
#define PIC_PWM1CON_PRSEN     EPIC_BIT(7)  /**< PWM restart enable (auto-restart). */
#define PIC_PWM1CON_PDC_MASK  0x7FU        /**< PDC6:PDC0, dead-band delay (6:0).  */
#define PIC_PWM1CON_POR_VALUE 0x00U

/* ECCPAS bits, DS39605F Register 15-3. */
#define PIC_ECCPAS_ECCPASE    EPIC_BIT(7)  /**< Auto-shutdown event status (RO once active). */
#define PIC_ECCPAS_SRC_MASK   0x70U        /**< ECCPAS2:ECCPAS0 at bits 6:4 (source). */
#define PIC_ECCPAS_PSSAC_MASK 0x0CU        /**< PSSAC1:PSSAC0 at bits 3:2 (P1A/P1C state). */
#define PIC_ECCPAS_PSSBD_MASK 0x03U        /**< PSSBD1:PSSBD0 at bits 1:0 (P1B/P1D state). */
#define PIC_ECCPAS_POR_VALUE  0x00U

/* EUSART bits (Registers 16-1 / 16-2 / 16-3), same layout as pic18fxx5x
 * (the shared PIC_TXSTA_* / PIC_RCSTA_* names resolve). BAUDCTL has no
 * ABDOVF bit on this part (Register 16-3, bit 7 unimplemented). */
#define PIC_TXSTA_TX9D         EPIC_BIT(0)  /**< 9th bit of TX data.                  */
#define PIC_TXSTA_TRMT         EPIC_BIT(1)  /**< TSR empty (read-only).               */
#define PIC_TXSTA_BRGH         EPIC_BIT(2)  /**< High baud rate.                      */
#define PIC_TXSTA_SYNC         EPIC_BIT(4)  /**< Sync mode.                           */
#define PIC_TXSTA_TXEN         EPIC_BIT(5)  /**< TX enable.                           */
#define PIC_TXSTA_TX9          EPIC_BIT(6)  /**< 9-bit TX.                            */
#define PIC_TXSTA_CSRC         EPIC_BIT(7)  /**< Clock source (sync).                 */
#define PIC_RCSTA_RX9D         EPIC_BIT(0)  /**< 9th bit of RX data.                  */
#define PIC_RCSTA_OERR         EPIC_BIT(1)  /**< Overrun error.                       */
#define PIC_RCSTA_FERR         EPIC_BIT(2)  /**< Framing error.                       */
#define PIC_RCSTA_ADDEN        EPIC_BIT(3)  /**< Address detect (9-bit).              */
#define PIC_RCSTA_CREN         EPIC_BIT(4)  /**< Continuous receive.                  */
#define PIC_RCSTA_SREN         EPIC_BIT(5)  /**< Single receive.                      */
#define PIC_RCSTA_RX9          EPIC_BIT(6)  /**< 9-bit RX.                            */
#define PIC_RCSTA_SPEN         EPIC_BIT(7)  /**< Serial port enable.                  */
#define PIC_BAUDCTL_ABDEN      EPIC_BIT(0)  /**< Auto-baud detect enable.            */
#define PIC_BAUDCTL_WUE        EPIC_BIT(1)  /**< Wake-up enable (async).             */
#define PIC_BAUDCTL_BRG16      EPIC_BIT(3)  /**< 16-bit baud generator (else 8-bit). */
#define PIC_BAUDCTL_SCKP       EPIC_BIT(4)  /**< Sync: TX clock polarity.            */
#define PIC_BAUDCTL_RCIDL      EPIC_BIT(6)  /**< Receiver idle (read-only).           */

/* A/D bits, DS39605F Registers 17-1/17-2/17-3. ADCON2 matches the 4550;
 * ADCON0 has VCFG at 7:6 and a 3-bit CHS at 4:2; ADCON1 is a 7-bit
 * per-pin PCFG with no VCFG. */
#define PIC_ADCON0_ADON        EPIC_BIT(0)  /**< A/D module on.                       */
#define PIC_ADCON0_GO_DONE     EPIC_BIT(1)  /**< Conversion status (1 = in progress). */
#define PIC_ADCON0_CHS_MASK    0x1CU        /**< CHS2:CHS0 at bits 4:2.               */
#define PIC_ADCON0_CHS_POS     2            /**< CHS field shift.                     */
#define PIC_ADCON0_VCFG0       EPIC_BIT(6)  /**< Vref+ source: 1=AN3, 0=VDD.          */
#define PIC_ADCON0_VCFG1       EPIC_BIT(7)  /**< Vref- source: 1=AN2, 0=VSS.          */
#define PIC_ADCON0_POR_VALUE   0x00U

#define PIC_ADCON1_PCFG_MASK   0x7FU        /**< PCFG6:PCFG0 at bits 6:0 (7 pins).     */
#define PIC_ADCON1_POR_VALUE   0x00U

#define PIC_ADCON2_ADCS_MASK   0x07U        /**< ADCS2:ADCS0 at bits 2:0.             */
#define PIC_ADCON2_ACQT_MASK   0x38U        /**< ACQT2:ACQT0 at bits 5:3.             */
#define PIC_ADCON2_ACQT_POS    3            /**< ACQT field shift.                     */
#define PIC_ADCON2_ADFM        EPIC_BIT(7)  /**< 1 = right justified, 0 = left.       */
#define PIC_ADCON2_POR_VALUE   0x00U

/* EEPROM control bits, DS39605F Register 7-1 (identical to 4550). */
#define PIC_EECON1_RD          EPIC_BIT(0)  /**< Read control (with EEPGD=0).         */
#define PIC_EECON1_WR          EPIC_BIT(1)  /**< Write control.                      */
#define PIC_EECON1_WREN        EPIC_BIT(2)  /**< Write-enable (must be set before WR). */
#define PIC_EECON1_WRERR       EPIC_BIT(3)  /**< Write-error flag.                   */
#define PIC_EECON1_FREE        EPIC_BIT(4)  /**< Flash erase/Program Free bit.        */
#define PIC_EECON1_EEPGD       EPIC_BIT(7)  /**< 1=Flash program memory, 0=Data EEPROM. */
#define PIC_EECON1_POR_VALUE   0x00U

/* PIR2 / PIE2 / IPR2 bits (Reg 9-5/9-7/9-9). Bits 0 (CCP2), 3 (BCLIE, no
 * MSSP), 5/6 (USBIE/CMIE, no USB/comparator) are unimplemented on this
 * part; no macro is defined for them. */
#define PIC_PIR2_OSCFIF       EPIC_BIT(7)   /**< Oscillator fail flag.            */
#define PIC_PIR2_EEIF         EPIC_BIT(4)   /**< EEPROM write done flag.          */
#define PIC_PIR2_LVDIF        EPIC_BIT(2)   /**< Low-voltage detect flag.         */
#define PIC_PIR2_TMR3IF       EPIC_BIT(1)   /**< Timer3 overflow flag.            */
#define PIC_PIE2_OSCFIE       EPIC_BIT(7)
#define PIC_PIE2_EEIE         EPIC_BIT(4)
#define PIC_PIE2_LVDIE        EPIC_BIT(2)
#define PIC_PIE2_TMR3IE       EPIC_BIT(1)
#define PIC_IPR2_OSCFIP       EPIC_BIT(7)
#define PIC_IPR2_EEIP         EPIC_BIT(4)
#define PIC_IPR2_LVDIP        EPIC_BIT(2)
#define PIC_IPR2_TMR3IP       EPIC_BIT(1)

/* Reset values (POR). DS39605F Table 5-1 + Register 4-1 reset notes.
 * RCON after POR: IPEN=0, bit 6 unimplemented, RI=0, TO=1, PD=1, POR=1,
 * BOR=1 (POR/BOR set per Register 4-1 Note 1). IPR1/IPR2 default to
 * all-high priority. T0CON resets to 0xFF (on, 8-bit, external, no
 * prescaler). */
#define PIC_STATUS_POR_VALUE     0x00U
#define PIC_BSR_POR_VALUE        0x00U
#define PIC_RCON_POR_VALUE       0x57U
#define PIC_INTCON_POR_VALUE     0x00U
#define PIC_INTCON2_POR_VALUE    0xFBU
#define PIC_INTCON3_POR_VALUE    0xC0U
#define PIC_PIR1_POR_VALUE       0x00U
#define PIC_PIE1_POR_VALUE       0x00U
#define PIC_IPR1_POR_VALUE       0xFFU
#define PIC_PIR2_POR_VALUE       0x00U
#define PIC_PIE2_POR_VALUE       0x00U
#define PIC_IPR2_POR_VALUE       0xFFU
#define PIC_T0CON_POR_VALUE      0xFFU
#define PIC_TRIS_POR_VALUE       0xFFU   /* All pins inputs after POR. */
#define PIC_LAT_POR_VALUE        0x00U   /* Output latches clear after POR. */
#define PIC_PORT_POR_VALUE       0x00U
#define PIC_TXSTA_POR_VALUE      0x02U   /* TRMT=1, TSR empty. */
#define PIC_RCSTA_POR_VALUE      0x00U
#define PIC_BAUDCTL_POR_VALUE    0x00U
#define PIC_SPBRG_POR_VALUE      0x00U
#define PIC_SPBRGH_POR_VALUE     0x00U

#endif /* PIC18F1320_SFR_H */
