# PIC16F88X HAL Manual

Datasheet-cited register and peripheral reference for the PIC16F88X
family (PIC16F882/883/884/886/887). Every register fact below cites
DS40001291H (the family datasheet, `new_part/40001291H.pdf`); silicon
errata DS80000302K and the Timer1 module errata DS80329B are cited
where they change driver behavior. The shared conventions (naming,
handle pattern, harness, interrupt model) live in
`epic-common/MANUAL.md`; this manual only covers what is per-family.

## Family identity

| Device | Flash | SRAM | EEPROM | I/O | ADC ch | EUSART | MSSP | ECCP/CCP | Comp | Timers 8/16 |
|---|---|---|---|---|---|---|---|---|---|---|
| PIC16F882  | 2048 w | 128 B | 128 B | 24 | 11 | 1 | 1 | 1/1 | 2 | 2/1 |
| PIC16F883  | 4096 w | 256 B | 256 B | 24 | 11 | 1 | 1 | 1/1 | 2 | 2/1 |
| PIC16F884  | 4096 w | 256 B | 256 B | 35 | 14 | 1 | 1 | 1/1 | 2 | 2/1 |
| PIC16F886  | 8192 w | 368 B | 256 B | 24 | 11 | 1 | 1 | 1/1 | 2 | 2/1 |
| PIC16F887  | 8192 w | 368 B | 256 B | 35 | 14 | 1 | 1 | 1/1 | 2 | 2/1 |

DS40001291H §1.0, Table "Family Types".

- 28-pin parts (882/883/886) have no PORTD/PORTE; the ECCP P1B/P1C/P1D
  outputs move to RB2/RB1/RB4. 40/44-pin parts (884/887) add PORTD and
  PORTE RE<2:0> plus ANSEL<7:5> (AN5..AN7).
- Classic mid-range core: 35 instructions, 4 data banks selected by
  RP0/RP1 (STATUS<6:5>), single interrupt vector at 0x0004, 8-level
  hardware stack, common RAM 0x70-0x7F (DS40001291H §2.2, Figures
  2-4..2-6).

## Core SFRs

### STATUS (0x03)

DS40001291H Register 2-1. IRP (bit 7) selects the indirect bank pair;
RP1:RP0 (bits 6:5) select the direct bank; TO/PD (bits 4:3) are the
WDT/Sleep status bits; Z/DC/C (bits 2:0) are the ALU flags. POR value
0001 1xxx.

### INTCON (0x0B)

DS40001291H Register 2-3. GIE (7), PEIE (6), T0IE (5), INTE (4), RBIE
(3), T0IF (2), INTF (1), RBIF (0). POR value 0000 000x.

### PIR1/PIE1 (0x0C/0x8C), PIR2/PIE2 (0x0D/0x8D)

DS40001291H Registers 2-4..2-7. PIR1: TMR1IF(0), TMR2IF(1), CCP1IF(2),
SSPIF(3), TXIF(4), RCIF(5), ADIF(6). PIR2: CCP2IF(0), ULPWUIF(2),
BCLIF(3), EEIF(4), C1IF(5), C2IF(6), OSFIF(7). PIE1/PIE2 mirror the
enables. Note the 88X has **separate** C1IF/C2IF flags (the 87XA's
single CMIF does not exist here) and adds ULPWUIF and OSFIF.

### PCON (0x8E)

DS40001291H §14.2.6, Register 2-8. ULPWUE(5), SBOREN(4), POR(1), BOR(0).
POR value --01 --0x: SBOREN=1, POR=0 (a Power-on Reset occurred).

### OPTION_REG (0x81)

DS40001291H Register 5-1. RBPU(7), INTEDG(6), T0CS(5), T0SE(4), PSA(3),
PS2:PS0(2:0). POR value 1111 1111.

## Oscillator (DS40001291H §4.0)

### OSCCON (0x8F), Register 4-1

IRCF2:IRCF0 (bits 6:4) select the internal frequency: 111=8 MHz,
110=4 MHz (POR default), 101=2 MHz, 100=1 MHz, 011=500 kHz, 010=250 kHz,
001=125 kHz, 000=31 kHz (LFINTOSC). OSTS(3) reports the clock source;
HTS(2)/LTS(1) report HFINTOSC/LFINTOSC stability; SCS(0) selects the
system clock (1=internal, 0=FOSC<2:0> config bits). POR value -110 q000.

### OSCTUNE (0x90), Register 4-2

TUN4:TUN0 (bits 4:0), 5-bit two's-complement frequency tuning; 0 = the
factory-calibrated frequency.

### Fail-Safe Clock Monitor

FCMEN (CONFIG1<11>) enables the monitor; on external-clock failure the
device switches to the internal oscillator and sets PIR2<OSFIF>
(DS40001291H §4.8). The driver arms PIE2<OSFIE> and the OSF_IRQHandler.

## GPIO (DS40001291H §3.0)

PORTA (0x05), PORTB (0x06), PORTC (0x07), PORTD (0x08, 40/44-pin),
PORTE (0x09, RE<2:0> 40/44-pin + RE3 input-only). TRISx mirrors at
0x85-0x89. There are no LAT registers on this family: writes to PORTx
are read-modify-write on the latch (DS40001291H §3.1).

### ANSEL (0x188) / ANSELH (0x189), Registers 3-3/3-4

Per-pin analog select, Bank 3. ANSEL<4:0> = AN0..AN4 (RA0..RA3, RA5);
ANSEL<7:5> = AN5..AN7 (RE0..RE2, 40/44-pin only); ANSELH<5:0> = AN8..
AN13 (RB2, RB3, RB1, RB4, RB0, RB5). **Reset value is all-analog**
(0xFF/0x3F), so digital pins read '0' until their ANSEL bit is cleared
(DS40001291H Table 2-1 note 3). The GPIO driver clears the bit for
INPUT/OUTPUT modes and sets it for ANALOG.

### WPUB (0x95), Register 3-7

Per-pin PORTB weak pull-ups, gated by OPTION_REG<RBPU> (RBPU=0 enables
the per-pin WPUB bits). POR value 0xFF.

### IOCB (0x96), Register 3-8

Per-pin interrupt-on-change enable for all 8 PORTB pins (the 87XA only
had RB<7:4>). RBIF (INTCON<0>) is set on any enabled pin change; the
mismatch comparator re-arms when PORTB is read, so the ISR must read
PORTB before clearing RBIF (DS40001291H §3.4.3).

## Timer0 (DS40001291H §5.0)

TMR0 (0x01), OPTION_REG (0x81). 8-bit counter with the shared
prescaler: T0CS selects Fosc/4 or RA4/T0CKI; PSA assigns the prescaler
to TMR0 or the WDT; PS2:PS0 select 1:2..1:256 (Table 5-1). Writing TMR0
clears the prescaler (§5.3 Note). **Errata DS80000302K item 10**: the
prescaler-assignment switch (WDT→TMR0→WDT) with T0CKI enabled and 1:1
can spuriously reset; the driver's Start follows §5.1.3.1's sequence
(CLRWDT, TMR0 write, then PSA/PS).

## Timer1 (DS40001291H §6.0)

TMR1L (0x0E), TMR1H (0x0F), T1CON (0x10). 16-bit counter with 1:1..1:8
prescaler (T1CKPS1:0), internal Fosc/4 or external T1CKI/T1OSC clock
(TMR1CS), async mode (T1SYNC), and the LP 32.768 kHz crystal oscillator
(T1OSCEN). The 88X adds the **gate**: TMR1GE (T1CON<6>) enables gating,
T1GINV (T1CON<7>) selects active-high/low, and CM2CON1<T1GSS> picks the
gate source (T1G pin or synchronized C2OUT, DS40001291H §6.6, §8.8.1).
The CCP special-event trigger (§6.10) can reset TMR1H:L.

**Errata DS80000302K items 8/9**: with an external crystal, Timer1 may
miss the first count after a reload (item 8) and the LP oscillator may
stop below 25 °C (item 9). The driver documents the reload sequence
(switch to the internal clock, wait for an increment, reload, switch
back) for firmware authors; it does not force it on internal-clock
users. DS80329B gives the canonical PIC12/14/16/17 reload example.

## Timer2 (DS40001291H §7.0)

TMR2 (0x11), T2CON (0x12), PR2 (0x92, Bank 1). 8-bit timer with 1:1/
1:4/1:16 prescaler (T2CKPS1:0), 1:1..1:16 postscaler (TOUTPS3:0), and
the PR2 period register. TMR2IF fires once per (PR2+1) period. Writing
TMR2 or T2CON clears the prescaler/postscaler counters (§7.1).

## EUSART (DS40001291H §12.0)

One EUSART on this family (the 88X has no EUSART2). TXSTA (0x98),
RCSTA (0x18), TXREG (0x19), RCREG (0x1A), SPBRG (0x99), SPBRGH (0x9A),
BAUDCTL (0x187, Bank 3).

### BAUDCTL, Register 12-3

ABDOVF(7, read-only), RCIDL(6, read-only), SCKP(4), BRG16(3), WUE(1),
ABDEN(0). BRG16 selects the 16-bit SPBRGH:SPBRG pair; ABDEN enables
auto-baud detect (clears on completion, ABDOVF reports overflow); WUE
arms the wake-up-on-start-bit (clears when RCIF sets).

### Baud rate formulas, Table 12-3

Async 8-bit: FOSC/(64·(X+1)) with BRGH=0, FOSC/(16·(X+1)) with BRGH=1.
Async 16-bit: FOSC/(16·(X+1)). Sync: FOSC/(4·(X+1)). The shared
`USART_ComputeSPBRG` (4-arg, cross-family contract) covers the 8-bit
case; `USART_ComputeSPBRG16` covers BRG16.

### TXSTA, Register 12-1

CSRC(7), TX9(6), TXEN(5), SYNC(4), SENDB(3), BRGH(2), TRMT(1,
read-only), TX9D(0). SENDB sends a Sync Break on the next transmission.

### RCSTA, Register 12-2

SPEN(7), RX9(6), SREN(5), CREN(4), ADDEN(3), FERR(2), OERR(1), RX9D(0).
ADDEN enables 9-bit address-detect mode.

## MSSP (DS40001291H §13.0)

SSPBUF (0x13), SSPCON (0x14), SSPCON2 (0x91), PR2 (0x92), SSPADD (0x93),
SSPSTAT (0x94), SSPMSK (via SSPADD when SSPM=1001).

### SSPCON, Register 13-2

WCOL(7), SSPOV(6), SSPEN(5), CKP(4), SSPM3:0(3:0). Mode encodings:
0000-0011 SPI master (Fosc/4, /16, /64, TMR2/2), 0100/0101 SPI slave
(SS control on/off), 0110/0111 I2C slave 7/10-bit, 1000 I2C master,
1001 Load-Mask (SSPMSK), 1011 I2C firmware master, 1110/1111 I2C slave
7/10-bit with Start/Stop interrupts.

### SSPSTAT, Register 13-1

SMP(7), CKE(6), D/A(5), P(4), S(3), R/W(2), UA(1), BF(0).

### SSPCON2, Register 13-3

GCEN(7), ACKSTAT(6), ACKDT(5), ACKEN(4), RCEN(3), PEN(2), RSEN(1),
SEN(0).

### SSPMSK, Register 13-4

I2C address mask, reachable only when SSPM=1001. A mask bit of 1 means
the received address bit IS compared to SSPADD.

**Errata DS80000302K items 2/4/11** (SPI host): TMR2/2 clock can
produce a short first SCK pulse (item 2, workaround: stop TMR2, clear
it, load SSPBUF, restart); Fosc/64 or TMR2/2 with CKE=0 can write-
collide on fast reload (item 4, workaround: delay one SCK period and
check WCOL); disabling the module can emit a clock pulse (item 11).
Item 5 (I2C client R/W bit on NACK) and item 6 (I2C host clock
stretching) affect the I2C paths; the workarounds are documented in the
driver comments.

## ADC (DS40001291H §9.0)

ADCON0 (0x1F), ADCON1 (0x9F), ADRESH (0x1E), ADRESL (0x9E). 10-bit
successive-approximation converter.

### ADCON0, Register 9-1

ADCS1:0(7:6) clock select (Fosc/2, /8, /32, or internal RC), CHS3:0
(5:2) channel select, GO/DONE(1), ADON(0). Channel encodings: 0000-1101
= AN0..AN13, 1110 = CVREF, 1111 = VP6 (fixed 0.6 V reference).

### ADCON1, Register 9-2

ADFM(7) result format, VCFG1(5) VREF- source (VSS or VREF- pin), VCFG0
(4) VREF+ source (VDD or VREF+ pin). Unlike the 87XA, there is no PCFG
field: analog pins are selected per-pin through ANSEL/ANSELH.

**Errata DS80000302K item 3**: selecting the VP6 channel (CHS=1111)
after sampling a channel above ~3.6 V can disturb the HFINTOSC. The
workaround (select a low channel first) is documented in the ADC
driver.

## Comparators (DS40001291H §8.0)

CM1CON0 (0x107), CM2CON0 (0x108), CM2CON1 (0x109), all Bank 2.

### CMxCON0, Registers 8-1/8-2

CxON(7), CxOUT(6, read-only), CxOE(5), CxPOL(4), CxR(2), CxCH1:0(1:0).
C1 has 2 inverting-input channels (C12IN0-, C12IN1-); C2 has 4
(C12IN0-..C12IN3-).

### CM2CON1, Register 8-3

MC1OUT(7)/MC2OUT(6) mirror the comparator outputs (read-only), C1RSEL
(5)/C2RSEL(4) select CVREF vs the 0.6 V fixed reference, T1GSS(1)
selects the Timer1 gate source, C2SYNC(0) synchronizes C2OUT to
Timer1's falling edge.

### SR latch, SRCON (0x185, Bank 3), Register 8-4

SR1(7)/SR0(6) route latch Q to the C1OUT/C2OUT pins, C1SEN(5)/C2REN(4)
connect the comparator outputs to the latch set/reset, PULSS(3)/
PULSR(2) are self-clearing software pulses, FVREN(0) enables the 0.6 V
fixed reference. The latch is reset-dominant (§8.9.1).

### CVREF, VRCON (0x97, Bank 1), Register 8-5

VREN(7), VROE(6), VRR(5), VRSS(4), VR3:0(3:0). VRR=0 (high range):
CVREF = VDD/4 + (VR/32)·VDD. VRR=1 (low range): CVREF = (VR/24)·VDD.
VRSS selects VDD-VSS or the VREF+/VREF- pins as the source.

## ECCP1 / CCP2 (DS40001291H §11.0)

CCPR1L (0x15), CCPR1H (0x16), CCP1CON (0x17), CCPR2L (0x1B), CCPR2H
(0x1C), CCP2CON (0x1D). Capture/Compare need Timer1; PWM needs Timer2
(Table 11-2).

### CCP1CON, Register 11-1

P1M1:0(7:6) PWM output config (single/half/full bridge), DC1B1:0(5:4)
PWM duty LSBs, CCP1M3:0(3:0) mode. Mode encodings: 0000 off, 0010
compare-toggle, 0100-0111 capture (falling/rising/4th/16th), 1000/1001
compare set/clear, 1010 compare-software-interrupt, 1011 compare-
special-event (resets TMR1 or TMR2), 1100-1111 PWM with the four
polarity combinations.

### PWM1CON (0x9B), Register 11-4

PRSEN(7) auto-restart on shutdown-clear, PDC6:0(6:0) dead-time delay in
FOSC/4 cycles. **Errata DS80000302K item 12**: dead-time greater than
the duty cycle produces unpredictable waveforms; keep PDC below the
duty.

### ECCPAS (0x9C), Register 11-3

ECCPASE(7) shutdown status, ECCPAS2:0(6:4) shutdown source (comparator
outputs, INT pin, or combinations), PSSAC1:0(3:2)/PSSBD1:0(1:0) pin
states during shutdown (drive 0, drive 1, tri-state). The shutdown
condition is level-based and persists while present.

### PSTRCON (0x9D), Register 11-5

STRSYNC(4), STRD(3), STRC(2), STRB(1), STRA(0). Pulse steering in
Single-output PWM mode: the same PWM waveform can be routed to any
combination of P1A..P1D.

### CCP2CON, Register 11-2

DC2B1:0(5:4), CCP2M3:0(3:0). Plain CCP, no P1M bits.

## EEPROM (DS40001291H §10.0)

EEDAT (0x10C), EEADR (0x10D), EEDATH (0x10E), EEADRH (0x10F), EECON1
(0x18C), EECON2 (0x18D). 256 B on all parts except the 882 (128 B).
EECON1: EEPGD(7) selects program-flash vs data-EEPROM access, RD(0)/
WR(1) control bits, WREN(2) write-enable, WRERR(3). The write sequence
is the standard 0x55/0xAA EECON2 unlock (§10.2.3). EEIF (PIR2<4>)
signals completion.

## WDT / Sleep / BOR (DS40001291H §14.0)

### WDTCON (0x105, Bank 2), Register 14-3

WDTPS3:0(4:1) prescaler (1:32..1:65536, 1 ms..268 s nominal at 31 kHz
LFINTOSC; reset value 0100 = 1:512), SWDTEN(0) software enable. When
the WDTE config bit is 1 the WDT is always on; when WDTE=0, SWDTEN
controls it (§14.5).

### CONFIG1 (Register 14-1)

DEBUG(13), LVP(12), FCMEN(11), IESO(10), BOREN1:0(9:8), CPD(7), CP(6),
MCLRE(5), PWRTE(4), WDTE(3), FOSC2:0(2:0). BOREN encodings: 11=always
on, 10=on during operation only, 01=software-controlled via
PCON<SBOREN>, 00=off. FOSC: 111=RC, 110=RCIO, 101=INTOSC, 100=INTOSCIO,
011=EC, 010=HS, 001=XT, 000=LP.

### CONFIG2 (Register 14-2)

WRT1:0(10:9) flash self-write protection (per-device ranges), BOR4V(8)
BOR trip voltage (0=2.1 V, 1=4.0 V).

### Sleep

SLEEP clears PD (STATUS<3>) and sets TO; the WDT keeps running if
enabled. Wake-up requires an enabled interrupt source; with GIE set the
device vectors to 0x0004 (§14.6).

## Silicon errata summary (DS80000302K)

All revisions (882 A0, 883/884 A0, 886/887 A2) carry items 1-12; the
886/887 add item 13 (ICSP memory read/verify). The driver-relevant
items are cited inline above: Timer0 prescaler switch (10), Timer1
external crystal (8/9), ADC VP6 (3), MSSP SPI host (2/4/11), MSSP I2C
(5/6), ECCP dead-time (12).
