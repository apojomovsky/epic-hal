/*
 * SFR map, foundation subset: every address/mask/reset 1-to-1 from
 * DS39631E Table 5-1, cross-checked against the DFP header. All in the
 * Access Bank (0xF60-0xFFF), no BSR. No LATD/E or TRISD/E (RE3 only),
 * no USB/SPP (Table 1-1). Timer1-3/CCP/MSSP/EUSART/ADC/EEPROM SFRs
 * land with their drivers in phases 2-4.
 */

#ifndef PIC18F2520_SFR_H
#define PIC18F2520_SFR_H

#include "pic18f2520_hal.h"

/* Access Bank SFR addresses. */
/* DS39631E Table 5-1, SFR Map. All in the Access Bank (0xF60-0xFFF). */

/* Core CPU status, DS39631E Register 5-2. */
#define PIC_REG_STATUS        0xFD8U   /**< ALU status (N/OV/Z/DC/C).       */
#define PIC_REG_BSR           0xFE0U   /**< Bank Select Register (low nibble). */
#define PIC_REG_RCON          0xFD0U   /**< Reset Control (IPEN/TO/PD/POR/BOR). */

/* I/O ports, DS39631E §10.0, Table 10-1..10-3.
 * PORTx is the pin input sample; LATx is the output latch; TRISx is the
 * data-direction register. PIC18 exposes LATx as its own mapped register
 * (§10.0), so GPIO writes go through LATx, not PORTx. The 28-pin 2520
 * has PORTA/B/C (RA0..RA7, RB0..RB7, RC0..RC7) plus a single RE3/MCLR
 * input on PORTE bit 3 (DS39631E Table 1-1, Table 10-3) with no LAT/TRIS
 * register for it; there is no PORTD. */
#define PIC_REG_PORTA         0xF80U
#define PIC_REG_PORTB         0xF81U
#define PIC_REG_PORTC         0xF82U
#define PIC_REG_PORTE         0xF84U   /* RE3/MCLR input only, no LAT/TRIS. */

#define PIC_REG_LATA          0xF89U
#define PIC_REG_LATB          0xF8AU
#define PIC_REG_LATC          0xF8BU

#define PIC_REG_TRISA         0xF92U
#define PIC_REG_TRISB         0xF93U
#define PIC_REG_TRISC         0xF94U

/* Interrupt control, DS39631E §9.0, Register 9-1/9-2/9-3. */
#define PIC_REG_INTCON        0xFF2U   /**< GIE/GIEL/TMR0IE/INT0IE/RBIE + flags. */
#define PIC_REG_INTCON2       0xFF1U   /**< Pull-ups, INT edge, TMR0IP, RBIP.   */
#define PIC_REG_INTCON3       0xFF0U   /**< INT1/INT2 enable, flag, priority.   */

/* Peripheral interrupt flag/enable/priority, DS39631E Register 9-5/9-6/9-8. */
#define PIC_REG_PIR1          0xF9EU   /**< Peripheral Interrupt Flag Register 1. */
#define PIC_REG_PIE1          0xF9DU   /**< Peripheral Interrupt Enable Reg 1.    */
#define PIC_REG_IPR1          0xF9FU   /**< Peripheral Interrupt Priority Reg 1.  */
#define PIC_REG_PIR2          0xFA1U   /**< Peripheral Interrupt Flag Register 2 (TMR3IF, CCP2IF, ...). */
#define PIC_REG_PIE2          0xFA0U   /**< Peripheral Interrupt Enable Reg 2.    */
#define PIC_REG_IPR2          0xFA2U   /**< Peripheral Interrupt Priority Reg 2 (reset 0xFF, all high). */

/* Timer0, DS39631E §11.0, Register 11-1. */
#define PIC_REG_T0CON         0xFD5U   /**< Timer0 control (on/8-16bit/src/edge/PSA/PS). */
#define PIC_REG_TMR0L         0xFD6U   /**< Timer0 low byte (also 8-bit mode value). */
#define PIC_REG_TMR0H         0xFD7U   /**< Timer0 high byte (16-bit mode only).    */

/* Timer1, DS39631E §12.0, Register 12-1 (T1CON 0xFCD, same shape as 4550). */
#define PIC_REG_T1CON         0xFCDU   /**< Timer1 control (RD16/run/prescale/osc/sync/cs/on). */
#define PIC_REG_TMR1L         0xFCEU   /**< Timer1 low byte.                          */
#define PIC_REG_TMR1H         0xFCFU   /**< Timer1 high byte.                         */

/* Timer2, DS39631E §13.0, Register 13-1 (T2CON 0xFCA, same shape as 4550). */
#define PIC_REG_T2CON         0xFCAU   /**< Timer2 control (postscaler/on/prescaler). */
#define PIC_REG_PR2           0xFCBU   /**< Timer2 period register (Access Bank).    */
#define PIC_REG_TMR2          0xFCCU   /**< Timer2 counter.                          */

/* Timer3, DS39631E §12.0, Register 12-1 (T3CON 0xFB1, same shape as 4550). */
#define PIC_REG_T3CON         0xFB1U   /**< Timer3 control (RD16/CCP-sel/prescale/sync/cs/on). */
#define PIC_REG_TMR3L         0xFB2U   /**< Timer3 low byte.                          */
#define PIC_REG_TMR3H         0xFB3U   /**< Timer3 high byte.                         */

/* ECCP1/CCP2, DS39631E §15.0. 2520 has Enhanced CCP1 (auto-shutdown
 * only, no P1M/PDC/PSSBD bridge hardware like the 4550) + plain CCP2. */
#define PIC_REG_ECCP1AS       0xFB6U   /**< ECCP1 auto-shutdown control/status. */
#define PIC_REG_PWM1CON       0xFB7U   /**< ECCP1 auto-restart (PRSEN only).    */
#define PIC_REG_CCP2CON       0xFBAU   /**< CCP2 control (mode + duty LSBs).    */
#define PIC_REG_CCPR2L        0xFBBU   /**< CCP2 low byte.                      */
#define PIC_REG_CCPR2H        0xFBCU   /**< CCP2 high byte.                     */
#define PIC_REG_CCP1CON       0xFBDU   /**< ECCP1 control (mode + duty LSBs).   */
#define PIC_REG_CCPR1L        0xFBEU   /**< ECCP1 low byte.                     */
#define PIC_REG_CCPR1H        0xFBFU   /**< ECCP1 high byte.                    */

/* MSSP, DS39631E §17.0 (SSPCON1 0xFC6, same shape as 4550). */
#define PIC_REG_SSPCON2       0xFC5U   /**< MSSP control 2 (I2C master: SEN/PEN/etc). */
#define PIC_REG_SSPCON1       0xFC6U   /**< MSSP control 1 (mode/SSPEN/CKP/WCOL/SSPOV).*/
#define PIC_REG_SSPSTAT       0xFC7U   /**< MSSP status (SMP/CKE/D-A/P/S/R-W/UA/BF).  */
#define PIC_REG_SSPADD        0xFC8U   /**< MSSP address (I2C slave) / baud (master). */
#define PIC_REG_SSPBUF        0xFC9U   /**< MSSP data buffer.                          */

/* EUSART, DS39631E §18.0 (same shape as 4550). */
#define PIC_REG_BAUDCON       0xFB8U   /**< EUSART baud-rate control (BRG16/ABDEN/WUE/...). */
#define PIC_REG_RCSTA         0xFABU   /**< EUSART receive control/status.            */
#define PIC_REG_TXSTA         0xFACU   /**< EUSART transmit control/status.            */
#define PIC_REG_TXREG         0xFADU   /**< EUSART transmit data.                     */
#define PIC_REG_RCREG         0xFAEU   /**< EUSART receive data.                      */
#define PIC_REG_SPBRG         0xFAFU   /**< EUSART baud-rate divisor, low byte.        */
#define PIC_REG_SPBRGH        0xFB0U   /**< EUSART baud-rate divisor, high byte (BRG16=1). */

/* Comparator, DS39631E §20.0 (two comparators, same as 4550). */
#define PIC_REG_CMCON         0xFB4U   /**< Comparator control (mode/inputs/outputs). */
#define PIC_REG_CVRCON        0xFB5U   /**< Comparator voltage reference control.     */

/* Data EEPROM, DS39631E §7.0 (256 bytes, EEADR alone, same as 4550). */
#define PIC_REG_EECON1        0xFA6U   /**< EEPROM/Flash control (RD/WR/WREN/WRERR/EEPGD). */
#define PIC_REG_EECON2        0xFA7U   /**< EEPROM unlock (write 0x55 then 0xAA).       */
#define PIC_REG_EEDATA        0xFA8U   /**< EEPROM data register.                      */
#define PIC_REG_EEADR         0xFA9U   /**< EEPROM address register (8-bit, 0..255).    */

/* A/D Converter, DS39631E §19.0 (10-bit, AN0-4 + AN8-12 on 28-pin). */
#define PIC_REG_ADCON2        0xFC0U   /**< A/D control 2 (ADCS/ACQT/ADFM).            */
#define PIC_REG_ADCON1        0xFC1U   /**< A/D control 1 (PCFG/VCFG).                 */
#define PIC_REG_ADCON0        0xFC2U   /**< A/D control 0 (CHS/GO-DONE/ADON).          */
#define PIC_REG_ADRESL        0xFC3U   /**< A/D result low byte.                       */
#define PIC_REG_ADRESH        0xFC4U   /**< A/D result high byte.                       */

/* STATUS bits (Register 5-2). */
#define PIC_STATUS_N          EPIC_BIT(4)   /**< Negative / borrow complement. */
#define PIC_STATUS_OV         EPIC_BIT(3)   /**< Overflow.                     */
#define PIC_STATUS_Z          EPIC_BIT(2)   /**< Zero.                         */
#define PIC_STATUS_DC         EPIC_BIT(1)   /**< Digit carry/borrow.           */
#define PIC_STATUS_C          EPIC_BIT(0)   /**< Carry/borrow.                 */

/* RCON bits (Register 4-1 / 9-10). */
#define PIC_RCON_IPEN         EPIC_BIT(7)   /**< Interrupt Priority Enable.    */
#define PIC_RCON_SBOREN       EPIC_BIT(6)   /**< BOR software enable.          */
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

/* PIR1 / PIE1 / IPR1 bits (Reg 9-5/9-6/9-8). */
/* Same bit layout across the three registers: flag / enable / priority.
 * PIR1 has no SPPIF on this part (no SPP); ADIF is the top bit. */
#define PIC_PIR1_ADIF         EPIC_BIT(6)   /**< A/D conversion complete flag.  */
#define PIC_PIR1_RCIF         EPIC_BIT(5)   /**< EUSART receive flag.           */
#define PIC_PIR1_TXIF         EPIC_BIT(4)   /**< EUSART transmit flag.          */
#define PIC_PIR1_SSPIF        EPIC_BIT(3)   /**< MSSP flag.                     */
#define PIC_PIR1_CCP1IF       EPIC_BIT(2)   /**< CCP1 flag.                     */
#define PIC_PIR1_TMR2IF       EPIC_BIT(1)   /**< Timer2 match flag.             */
#define PIC_PIR1_TMR1IF       EPIC_BIT(0)   /**< Timer1 overflow flag.          */
#define PIC_PIE1_ADIE         EPIC_BIT(6)
#define PIC_PIE1_RCIE         EPIC_BIT(5)
#define PIC_PIE1_TXIE         EPIC_BIT(4)
#define PIC_PIE1_SSPIE        EPIC_BIT(3)
#define PIC_PIE1_CCP1IE       EPIC_BIT(2)
#define PIC_PIE1_TMR2IE       EPIC_BIT(1)
#define PIC_PIE1_TMR1IE       EPIC_BIT(0)
#define PIC_IPR1_ADIP         EPIC_BIT(6)
#define PIC_IPR1_RCIP         EPIC_BIT(5)
#define PIC_IPR1_TXIP         EPIC_BIT(4)
#define PIC_IPR1_SSPIP        EPIC_BIT(3)
#define PIC_IPR1_CCP1IP       EPIC_BIT(2)
#define PIC_IPR1_TMR2IP       EPIC_BIT(1)
#define PIC_IPR1_TMR1IP       EPIC_BIT(0)

/* T0CON bits (Register 11-1). */
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
#define PIC_T1CON_TMR1CS      EPIC_BIT(1)   /**< 0=Fosc/4, 1=external/T1OSC.     */
#define PIC_T1CON_TMR1ON      EPIC_BIT(0)   /**< Timer1 on/off.                   */
#define PIC_T1CON_POR_VALUE   0x00U         /**< Power-on reset value.            */

/* T2CON bits (Register 13-1, identical to 4550). */
#define PIC_T2CON_TOUTPS_MASK 0x78U         /**< T2OUTPS3:T2OUTPS0 at bits 6:3 (1:(N+1)). */
#define PIC_T2CON_TMR2ON      EPIC_BIT(2)   /**< Timer2 on/off.                   */
#define PIC_T2CON_T2CKPS_MASK 0x03U         /**< T2CKPS1:T2CKPS0 at bits 1:0.     */
#define PIC_T2CON_POR_VALUE   0x00U         /**< Power-on reset value.            */
#define PIC_PR2_POR_VALUE     0xFFU         /**< PR2 power-on reset value.        */

#define PIC_T3CON_T3CCP1      EPIC_BIT(3)   /**< CCP1 Timer1/Timer3 select.       */
#define PIC_T3CON_RD16        EPIC_BIT(7)   /**< 16-bit read/write mode enable.   */
#define PIC_T3CON_T3CCP2      EPIC_BIT(6)   /**< CCP2 Timer1/Timer3 select.       */
#define PIC_T3CON_T3CKPS_MASK 0x30U         /**< T3CKPS1:T3CKPS0 at bits 5:4.     */
#define PIC_T3CON_T3SYNC      EPIC_BIT(2)   /**< External clock sync (alias).     */
#define PIC_T3CON_TMR3CS      EPIC_BIT(1)   /**< 0=Fosc/4, 1=external.            */
#define PIC_T3CON_TMR3ON      EPIC_BIT(0)   /**< Timer3 on/off.                   */
#define PIC_T3CON_POR_VALUE   0x00U         /**< Power-on reset value.            */

/* CCP1CON/CCP2CON bits (Register 15-1, same as 4550 for mode/duty). */
#define PIC_CCPxCON_CCPxM_MASK 0x0FU        /**< CCPxM3:CCPxM0 mode bits.        */
#define PIC_CCPxCON_DCxB_MASK  0x30U        /**< DCxB1:DCxB0 duty LSBs (bits 5:4). */
#define PIC_CCPxCON_POR_VALUE  0x00U        /**< Power-on reset value.           */

/* ECCP1AS bits (Register 15-2, reduced: PSSAC only, no PSSBD). */
#define PIC_ECCP1AS_ECCPASE    EPIC_BIT(7)  /**< Auto-shutdown status/trigger.  */
#define PIC_ECCP1AS_ECCPAS_MASK 0x70U       /**< ECCPAS2:ECCPAS0 source (bits 6:4). */
#define PIC_ECCP1AS_PSSAC_MASK 0x0CU        /**< PSSAC1:PSSAC0 P1A/C state (bits 3:2). */
#define PIC_ECCP1AS_POR_VALUE  0x00U        /**< Power-on reset value.           */

/* PWM1CON bits (Register 15-3, PRSEN only, no PDC on 2520). */
#define PIC_PWM1CON_PRSEN      EPIC_BIT(7)  /**< Auto-restart after shutdown.   */
#define PIC_PWM1CON_POR_VALUE  0x00U        /**< Power-on reset value.           */

/* MSSP bits (Register 17-1/17-2/17-3, same as 4550). */
#define PIC_SSPCON1_SSPM_MASK  0x0FU       /**< SSPM3:SSPM0 at bits 3:0 (mode select). */
#define PIC_SSPCON1_CKP        EPIC_BIT(4)  /**< Clock polarity (SPI).                */
#define PIC_SSPCON1_SSPEN      EPIC_BIT(5)  /**< MSSP enable.                          */
#define PIC_SSPCON1_SSPOV      EPIC_BIT(6)  /**< Receive overflow.                    */
#define PIC_SSPCON1_WCOL       EPIC_BIT(7)  /**< Write collision.                     */
#define PIC_SSPCON1_POR_VALUE  0x00U        /**< Power-on reset value.                */
#define PIC_SSPCON2_SEN        EPIC_BIT(0)  /**< Start condition enable (I2C master). */
#define PIC_SSPCON2_RSEN       EPIC_BIT(1)  /**< Repeated start enable.               */
#define PIC_SSPCON2_PEN        EPIC_BIT(2)  /**< Stop condition enable.               */
#define PIC_SSPCON2_RCEN       EPIC_BIT(3)  /**< Receive enable.                      */
#define PIC_SSPCON2_ACKEN      EPIC_BIT(4)  /**< Acknowledge sequence enable.         */
#define PIC_SSPCON2_ACKDT      EPIC_BIT(5)  /**< Acknowledge data (ACK/NACK value).    */
#define PIC_SSPCON2_ACKSTAT    EPIC_BIT(6)  /**< Acknowledge status (from slave).      */
#define PIC_SSPCON2_GCEN       EPIC_BIT(7)  /**< General call enable (I2C slave).      */
#define PIC_SSPSTAT_BF         EPIC_BIT(0)  /**< Buffer full.                          */
#define PIC_SSPSTAT_UA         EPIC_BIT(1)  /**< Update address (10-bit).              */
#define PIC_SSPSTAT_RW         EPIC_BIT(2)  /**< Read/write (I2C).                     */
#define PIC_SSPSTAT_S          EPIC_BIT(3)  /**< Start (I2C).                          */
#define PIC_SSPSTAT_P          EPIC_BIT(4)  /**< Stop (I2C).                           */
#define PIC_SSPSTAT_DA         EPIC_BIT(5)  /**< Data/address (I2C).                   */
#define PIC_SSPSTAT_CKE        EPIC_BIT(6)  /**< Clock edge (SPI).                     */
#define PIC_SSPSTAT_SMP        EPIC_BIT(7)  /**< Sample phase (SPI master).             */

/* EUSART bits (Register 18-1/18-2/18-3, same as 4550). */
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
#define PIC_BAUDCON_ABDEN      EPIC_BIT(0)  /**< Auto-baud detect enable.            */
#define PIC_BAUDCON_WUE        EPIC_BIT(1)  /**< Wake-up enable (async).             */
#define PIC_BAUDCON_BRG16      EPIC_BIT(3)  /**< 16-bit baud generator (else 8-bit). */
#define PIC_BAUDCON_TXCKP      EPIC_BIT(4)  /**< Sync: TX clock polarity.            */
#define PIC_BAUDCON_RXDTP      EPIC_BIT(5)  /**< Sync: RX data polarity.             */
#define PIC_BAUDCON_RCIDL      EPIC_BIT(6)  /**< Receiver idle (read-only).          */
#define PIC_BAUDCON_ABDOVF     EPIC_BIT(7)  /**< Auto-baud overflow (read/clear).    */
#define PIC_TXSTA_POR_VALUE    0x02U        /**< TXSTA POR (TRMT=1, TSR empty).   */
#define PIC_RCSTA_POR_VALUE    0x00U        /**< RCSTA power-on reset value.        */
#define PIC_BAUDCON_POR_VALUE  0x00U        /**< BAUDCON power-on reset value.      */
#define PIC_SPBRG_POR_VALUE    0x00U        /**< SPBRG power-on reset value.        */
#define PIC_SPBRGH_POR_VALUE   0x00U        /**< SPBRGH power-on reset value.       */

/* ADC bits (Register 19-1/19-2/19-3, same as 4550). */
#define PIC_ADCON0_ADON        EPIC_BIT(0)  /**< A/D module on.                       */
#define PIC_ADCON0_GO_DONE     EPIC_BIT(1)  /**< Conversion status (1 = in progress). */
#define PIC_ADCON0_CHS_MASK    0x3CU        /**< CHS3:CHS0 at bits 5:2.               */
#define PIC_ADCON0_CHS_POS     2            /**< CHS field shift.                     */
#define PIC_ADCON0_POR_VALUE   0x00U
#define PIC_ADCON1_PCFG_MASK   0x0FU        /**< PCFG3:PCFG0 at bits 3:0.             */
#define PIC_ADCON1_VCFG0       EPIC_BIT(4)  /**< Vref+ source: 1=AN3, 0=VDD.          */
#define PIC_ADCON1_VCFG1       EPIC_BIT(5)  /**< Vref- source: 1=AN2, 0=VSS.           */
#define PIC_ADCON1_POR_VALUE   0x00U
#define PIC_ADCON2_ADCS_MASK   0x07U        /**< ADCS2:ADCS0 at bits 2:0.             */
#define PIC_ADCON2_ACQT_MASK   0x38U        /**< ACQT2:ACQT0 at bits 5:3.             */
#define PIC_ADCON2_ACQT_POS    3            /**< ACQT field shift.                     */
#define PIC_ADCON2_ADFM        EPIC_BIT(7)  /**< 1 = right justified, 0 = left.       */
#define PIC_ADCON2_POR_VALUE   0x00U

/* Comparator bits (Register 20-1, same as 4550). */
#define PIC_CMCON_CM_MASK      0x07U        /**< CM2:CM0 at bits 2:0 (mode select). */
#define PIC_CMCON_CIS          EPIC_BIT(3)  /**< Comparator input switch.            */
#define PIC_CMCON_C1INV        EPIC_BIT(4)  /**< C1 output inversion.                 */
#define PIC_CMCON_C2INV        EPIC_BIT(5)  /**< C2 output inversion.                 */
#define PIC_CMCON_C1OUT        EPIC_BIT(6)  /**< C1 output (read-only).              */
#define PIC_CMCON_C2OUT        EPIC_BIT(7)  /**< C2 output (read-only).              */
#define PIC_CMCON_POR_VALUE    0x07U        /**< POR: all comparators off (CM=111).  */

/* Data EEPROM bits (Register 7-1, same as 4550). */
#define PIC_EECON1_RD          EPIC_BIT(0)  /**< Read control (strobe, self-clears). */
#define PIC_EECON1_WR          EPIC_BIT(1)  /**< Write control (strobe).             */
#define PIC_EECON1_WREN        EPIC_BIT(2)  /**< Write enable.                       */
#define PIC_EECON1_WRERR       EPIC_BIT(3)  /**< Write-error flag.                   */
#define PIC_EECON1_FREE        EPIC_BIT(4)  /**< Flash row erase enable.             */
#define PIC_EECON1_CFGS        EPIC_BIT(6)  /**< Config access (1) vs code/data (0). */
#define PIC_EECON1_EEPGD       EPIC_BIT(7)  /**< 0 = data EEPROM, 1 = program flash. */
#define PIC_EECON1_POR_VALUE   0x00U

/* PIR2 / PIE2 / IPR2 bits (Reg 9-5/9-7/9-9). */
/* Same bit layout across the three registers: flag / enable / priority.
 * PIR2 has no USBIF on this part (no USB); OSCFIF is the top bit. */
#define PIC_PIR2_OSCFIF       EPIC_BIT(7)   /**< Oscillator fail flag.            */
#define PIC_PIR2_CMIF         EPIC_BIT(6)   /**< Comparator flag.                 */
#define PIC_PIR2_EEIF         EPIC_BIT(4)   /**< EEPROM write done flag.          */
#define PIC_PIR2_BCLIF        EPIC_BIT(3)   /**< MSSP bus collision flag.         */
#define PIC_PIR2_HLVDIF       EPIC_BIT(2)   /**< High/Low-voltage detect flag.    */
#define PIC_PIR2_TMR3IF       EPIC_BIT(1)   /**< Timer3 overflow flag.            */
#define PIC_PIR2_CCP2IF       EPIC_BIT(0)   /**< CCP2 flag.                       */
#define PIC_PIE2_OSCFIE       EPIC_BIT(7)
#define PIC_PIE2_CMIE         EPIC_BIT(6)
#define PIC_PIE2_EEIE         EPIC_BIT(4)
#define PIC_PIE2_BCLIE        EPIC_BIT(3)
#define PIC_PIE2_HLVDIE       EPIC_BIT(2)
#define PIC_PIE2_TMR3IE       EPIC_BIT(1)
#define PIC_PIE2_CCP2IE       EPIC_BIT(0)
#define PIC_IPR2_OSCFIP       EPIC_BIT(7)
#define PIC_IPR2_CMIP         EPIC_BIT(6)
#define PIC_IPR2_EEIP         EPIC_BIT(4)
#define PIC_IPR2_BCLIP        EPIC_BIT(3)
#define PIC_IPR2_HLVDIP       EPIC_BIT(2)
#define PIC_IPR2_TMR3IP       EPIC_BIT(1)
#define PIC_IPR2_CCP2IP       EPIC_BIT(0)

/* Reset values (POR), DS39631E Table 5-1 + Register 4-1 notes. RCON
 * POR value 0x57 (IPEN=0, SBOREN=1, TO/PD/POR/BOR set; the sim uses this
 * image). IPR1/IPR2 default high. T0CON to 0xFF; INTCON2 to 0xFB;
 * INTCON3 to 0xC0 (INT1/2 priority). */
#define PIC_STATUS_POR_VALUE     0x00U
#define PIC_BSR_POR_VALUE        0x00U
#define PIC_RCON_POR_VALUE       0x57U
#define PIC_INTCON_POR_VALUE     0x00U
#define PIC_INTCON2_POR_VALUE    0xFBU
#define PIC_INTCON3_POR_VALUE    0xC0U
#define PIC_PIR1_POR_VALUE       0x00U
#define PIC_PIE1_POR_VALUE       0x00U
#define PIC_IPR1_POR_VALUE       0xFFU
#define PIC_T0CON_POR_VALUE      0xFFU
#define PIC_PIR2_POR_VALUE       0x00U
#define PIC_PIE2_POR_VALUE       0x00U
#define PIC_IPR2_POR_VALUE       0xFFU
#define PIC_TRIS_POR_VALUE       0xFFU   /* All pins inputs after POR. */
#define PIC_LAT_POR_VALUE        0x00U   /* Output latches clear after POR. */
#define PIC_PORT_POR_VALUE       0x00U

#endif /* PIC18F2520_SFR_H */
