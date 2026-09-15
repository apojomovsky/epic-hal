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

#endif /* PIC18F1320_SFR_H */
