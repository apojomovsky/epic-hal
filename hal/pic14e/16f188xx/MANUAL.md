# PIC16F188xx register reference

Per-peripheral register reference for the GPIO, interrupt-on-change,
Timer0, EUSART, MSSP1 and PPS sources in this HAL. Shared conventions (naming, handle
pattern, harness, interrupt model) are in
[common/MANUAL.md](../../../common/MANUAL.md). This file covers only
what is specific to this family: 16F18854/55/56/57 (28-pin) and
16F18875/76/77 (40-pin, adding PORTD and PORTE).

## Sources and verification

- Addresses, implemented-bit masks, POR values and bit positions come
  from the Microchip.PIC16F1xxxx_DFP 1.32.471 EDC (`PIC16F18855.PIC`)
  and proc header (`pic16f18855.h`), pinned by the manifest.
  `scripts/build/gen-sfr.py` rewrites each `PIC_REG_*` address the EDC
  defines. Do not hand-edit addresses. The bit masks are
  hand-maintained and cross-checked against the DFP proc header.
- Datasheet: DS40001802 (PIC16F18854/55/56/57/75/76/77). Its PDF is
  bot-walled against scripted fetch, so no page cite below is checked.
  A value or cite without a checked page is still DFP-derived and
  **unverified** against the datasheet.
- Timer0 encodings (clock sources, prescaler, postscaler, T0OUT) cite
  DS40001825F §27, Registers 27-1/27-2 instead: the sibling 18857/77
  datasheet documents this exact Timer0 module (EDC _modsrc
  DOS-01631_tmr_mid0_v1_8bit on both), read as text, not from a
  bot-walled PDF.

## Implemented bits (from EDC impl masks)

| Register | Impl  | POR        | Meaning                                  |
|----------|-------|------------|------------------------------------------|
| PORTA    | 0xFF  | xxxxxxxx   | RA0..RA7                                 |
| LATA     | 0xFF  | xxxxxxxx   | Latch follows the PORTA pins             |
| TRISA    | 0xFF  | 0xFF       | Direction for RA0..RA7, all input        |
| ANSELA   | 0xFF  | 0xFF       | Analog select, all analog                |
| WPUA     | 0xFF  | 0x00       | Weak pull-up per PORTA pin               |
| IOCAP, IOCAN, IOCAF | 0xFF | 0x00 | Rising, falling, flag per PORTA pin |
| PORTB    | 0xFF  | xxxxxxxx   | RB0..RB7                                 |
| LATB     | 0xFF  | xxxxxxxx   | Latch follows the PORTB pins             |
| TRISB    | 0xFF  | 0xFF       | Direction for RB0..RB7, all input        |
| ANSELB   | 0xFF  | 0xFF       | Analog select, all analog                |
| WPUB     | 0xFF  | 0x00       | Weak pull-up per PORTB pin               |
| IOCBP, IOCBN, IOCBF | 0xFF | 0x00 | Rising, falling, flag per PORTB pin |
| PORTC    | 0xC0  | xxxxxxxx   | RC0..RC7 read mask; all eight RC fields are defined and TRISC/LATC report 0xFF, so the driver mask stays 0xFF |
| LATC     | 0xFF  | xxxxxxxx   | Latch follows the PORTC pins             |
| TRISC    | 0xFF  | 0xFF       | Direction for RC0..RC7, all input        |
| ANSELC   | 0xFF  | 0xFF       | Analog select, all analog                |
| WPUC     | 0xFF  | 0x00       | Weak pull-up per PORTC pin               |
| PORTD    | 0xFF  | xxxxxxxx   | RD0..RD7, 40-pin parts only              |
| LATD     | 0xFF  | xxxxxxxx   | Latch follows the PORTD pins             |
| TRISD    | 0xFF  | 0xFF       | Direction for RD0..RD7, all input        |
| ANSELD   | 0xFF  | 0xFF       | Analog select, all analog                |
| WPUD     | 0xFF  | 0x00       | Weak pull-up per PORTD pin               |
| PORTE    | 0x0F (0x08 on 28-pin, RE3 only) | ----xxxx | RE0..RE3 read; RE3 is input-only (MCLR), 40-pin parts add RE0..RE2 |
| LATE     | 0x07  | -----xxx  | Latch for RE0..RE2, 40-pin parts only    |
| TRISE    | 0x07  | -----111  | Direction for RE0..RE2, 40-pin parts only|
| ANSELE   | 0x07  | -----111  | Analog select RE0..RE2, 40-pin parts only|
| WPUE     | 0x0F (0x08 on 28-pin) | ----x000 | Weak pull-up RE0..RE2 (RE3 bit exists) |
| IOCEP, IOCEN, IOCEF | 0x08 | ----0--- | Rising, falling, flag for RE3 only |
| INTCON   | 0xC1  | 00-----1   | INTEDG, PEIE, GIE only                   |
| PIR0     | 0x31  | --00---0   | INTF, IOCIF, TMR0IF                      |
| PIE0     | 0x31  | --00---0   | INTE, IOCIE, TMR0IE                      |
| TMR0L    | 0xFF  | 00000000   | Counter low byte                         |
| TMR0H    | 0xFF  | 11111111   | Counter high byte, match value in 8-bit mode |
| T0CON0   | 0xBF  | 0-000000   | T0EN, T0OUT (read-only), T016BIT, T0OUTPS |
| T0CON1   | 0xFF  | 00000000   | T0CS, T0ASYNC, T0CKPS                    |
| PIR3     | 0x3F  | --000000   | SSP1IF, BCL1IF, SSP2IF, BCL2IF, TXIF (read-only), RCIF (read-only) |
| PIE3     | 0x3F  | --000000   | SSP1IE, BCL1IE, SSP2IE, BCL2IE, TXIE, RCIE |
| RC1REG   | 0xFF  | 00000000   | Receive FIFO head (read-only), a read pops it |
| TX1REG   | 0xFF  | 00000000   | Transmit buffer, a write clears TXIF     |
| SP1BRGL, SP1BRGH | 0xFF | 00000000 | Baud generator low and high byte  |
| RC1STA   | 0xFF  | 00000000   | SPEN, RX9, SREN, CREN, ADDEN, FERR/OERR/RX9D (read-only) |
| TX1STA   | 0xFF  | 00000010   | CSRC, TX9, TXEN, SYNC, SENDB, BRGH, TRMT (read-only), TX9D |
| BAUD1CON | 0xDB  | 01-00-00   | ABDOVF/RCIDL (read-only), SCKP, BRG16, WUE, ABDEN |
| SSP1BUF  | 0xFF  | xxxxxxxx   | Receive buffer (read), transmit register (write) |
| SSP1ADD  | 0xFF  | 00000000   | Baud divisor for the two Fosc/(4*(n+1)) modes |
| SSP1STAT | 0xFF  | 00000000   | SMP, CKE writable; DA, P, S, RW, UA, BF status |
| SSP1CON1 | 0xFF  | 00000000   | WCOL, SSPOV (clear-only), SSPEN, CKP, SSPM |
| SSP1CON2 | 0xFF  | 00000000   | GCEN, ACKSTAT (read-only), ACKDT, ACKEN, RCEN, PEN, RSEN, SEN |
| PPSLOCK  | 0x01  | -------0   | PPSLOCKED                                |
| xxxPPS   | 0x1F (T0CKIPPS 0x0F) | per input | Input pin select: port in bits 4:3, pin in 2:0 |
| RxyPPS   | 0x3F  | --000000   | Output source select, 0 = port latch     |

## Register addresses

Addresses are the `PIC_REG_*` values in `include/pic16f188xx_sfr.h`
(generated). The table below only lists the ones this HAL uses, in the
canonical 0xFxx spelling: on 16F18857/16F18877 every banked address at
or above 0xF00 reads 0x1Fxx instead (same low 12 bits; the header
selects the spelling per part, and the sfr-map audit checks both).

| Name     | Address |
|----------|---------|
| INTCON   | 0x0B    |
| PORTA    | 0x0C    |
| PORTB    | 0x0D    |
| PORTC    | 0x0E    |
| TRISA    | 0x11    |
| TRISB    | 0x12    |
| TRISC    | 0x13    |
| LATA     | 0x16    |
| LATB     | 0x17    |
| LATC     | 0x18    |
| PIR0     | 0x70C   |
| PIE0     | 0x716   |
| ANSELA   | 0xF38   |
| WPUA     | 0xF39   |
| IOCAP    | 0xF3D   |
| IOCAN    | 0xF3E   |
| IOCAF    | 0xF3F   |
| ANSELB   | 0xF43   |
| WPUB     | 0xF44   |
| IOCBP    | 0xF48   |
| IOCBN    | 0xF49   |
| IOCBF    | 0xF4A   |
| ANSELC   | 0xF4E   |
| WPUC     | 0xF4F   |
| IOCCP    | 0xF53   |
| IOCCN    | 0xF54   |
| PORTD    | 0x0F    |
| TRISD    | 0x14    |
| LATD     | 0x19    |
| PORTE    | 0x10    |
| TRISE    | 0x15    |
| LATE     | 0x1A    |
| TMR0L    | 0x1C    |
| TMR0H    | 0x1D    |
| T0CON0   | 0x1E    |
| T0CON1   | 0x1F    |
| ANSELD   | 0xF59   |
| WPUD     | 0xF5A   |
| ANSELE   | 0xF64   |
| WPUE     | 0xF65   |
| PIR3     | 0x70F   |
| PIE3     | 0x719   |
| RC1REG   | 0x119   |
| TX1REG   | 0x11A   |
| SP1BRGL  | 0x11B   |
| SP1BRGH  | 0x11C   |
| RC1STA   | 0x11D   |
| TX1STA   | 0x11E   |
| BAUD1CON | 0x11F   |
| SSP1BUF  | 0x18C   |
| SSP1ADD  | 0x18D   |
| SSP1STAT | 0x18F   |
| SSP1CON1 | 0x190   |
| SSP1CON2 | 0x191   |
| SSP1CON3 | 0x192   |
| PPSLOCK  | 0xE8F   |
| T0CKIPPS | 0xE91   |
| SSP1CLKPPS | 0xEC5 |
| SSP1DATPPS | 0xEC6 |
| SSP1SSPPS | 0xEC7  |
| RXPPS    | 0xECB   |
| RA0PPS   | 0xF10 (RB0PPS 0xF18, RC0PPS 0xF20, RD0PPS 0xF28; 8 consecutive per port) |

## Interrupt-on-change bits

| Bit | Name     | Use                                                              |
|-----|----------|------------------------------------------------------------------|
| PIR0<4> | IOCIF | Set when any enabled IOCx flag is set. Cleared by the handler. |
| PIE0<4> | IOCIE | Gates the IOC source. The dispatcher calls IOC only when IOCIF and IOCIE are both set. |
| PIR0<5> | TMR0IF | Set on 8-bit match or 16-bit rollover, postscaled. Cleared by the handler. |
| PIE0<5> | TMR0IE | Gates the Timer0 source. Set by Init only with a callback. |
| INTCON<6> | PEIE  | Set by `EPIC_IRQ_Enable` on either source. Exposes PIR0 sources. |
| INTCON<7> | GIE   | Global interrupt enable.                                         |
| PIR3<0> / PIE3<0> | SSP1IF / SSP1IE | MSSP1 event. The handler clears the flag, then runs the callback. |
| PIR3<4> / PIE3<4> | TXIF / TXIE | Read-only flag, set while the transmitter is on and TX1REG is empty. TXIE is a one-shot the handler clears. |
| PIR3<5> / PIE3<5> | RCIF / RCIE | Read-only flag, set while the receive FIFO holds a byte. |

## Timer0 bits

Clock source (T0CON1<T0CS2:T0CS0>, DS40001825F Register 27-2):
000 T0CKIPPS, 001 T0CKIPPS inverted, 010 FOSC/4, 011 HFINTOSC,
100 LFINTOSC, 101 SOSC, 110 LC1_out, 111 reserved (no driver value).
T0CKIPPS counts edges on the pin `EPIC_PPS_MapInput(PPS_INPUT_T0CKI, ...)`
selects (POR pin RA4; PORTA or PORTB only).
Prescaler (T0CON1<T0CKPS3:T0CKPS0>): encoding N divides by 2^N,
0000=1:1 through 1111=1:32768. Postscaler (T0CON0<T0OUTPS3:T0OUTPS0>):
value N divides by N+1, 0000=1:1 through 1111=1:16. 8-bit mode
(T016BIT=0) matches TMR0L against TMR0H and resets on match; 16-bit
mode rolls TMR0H:TMR0L over. T0OUT toggles per postscaled
match/rollover (§27.7) and is read-only: a T0CON0 write never changes
it, so readbacks taken while the timer runs mask it out.

`EPIC_TIMER0_Init` sets T0ASYNC from the source: asynchronous rows
(T0CKIPPS, LFINTOSC, SOSC, LC1_out) run async, FOSC/4 and HFINTOSC
run synced. The handler clears TMR0IF first, then runs the owned
slot, with no cstack autos (ISR bank rule below).

Gate shape, learned the hard way: a stop-on-overflow callback always
beats main's post-Start readback (overflow at 8 ticks lands inside the
read window), so the gate proves setup callback-free and reads no
image after the armed Start. Either spin exits on its condition, so
the gate never depends on SIM wall-clock speed.

Unlike the older enhanced mid-range parts, INTCON on this part carries
no IOCIF/IOCIE bits (impl 0xC1): the flag and enable live in PIR0/PIE0.
`IOCxF` flags are cleared in the handler one captured bit at a time
with a single-bit clear, never by a masked write of the whole register,
so a flag hardware sets during the handler is not overwritten. An edge
that lands after the clear re-raises IOCIF. A second edge on an
already-captured bit merges into that capture.

The handler clears all three ports' flags but reports PORTB only
(`IOCBF` mask plus the `PORTB` byte), matching the sibling families'
contract. IOCAF/IOCCF edges still vector and are still cleared, so no
armed edge sticks the vector; they just have no callback argument.

## EUSART bits

Asynchronous mode only; the module matches the sibling PIC16F193X
EUSART (DS41364B §23). Bit positions are DFP-checked by the sfr-map
audit, the DS page cites are unchecked.

- Baud: Fosc / (D * (n + 1)), D = 64 (BRGH=0, BRG16=0), 16 (BRGH=1,
  BRG16=0 or BRGH=0, BRG16=1), 4 (BRGH=1, BRG16=1). `USART_ComputeSPBRG`
  rounds to the nearest divisor and returns 0xFFFF when n does not fit
  (255 on the 8-bit generator, 65535 on the 16-bit one).
- `Init` writes the port off first, loads SP1BRGL/H, BAUD1CON.BRG16 and
  TX1STA (BRGH, TX9, TXEN), and writes RC1STA (SPEN, CREN, RX9, ADDEN)
  last. ADDEN is set only with 9-bit frames.
- TXIF is read-only: a TX1REG write clears it, and it reads set again once
  the byte moves to the shift register. Because it stays set while the
  buffer is empty, `EPIC_USART_EnableTxInterrupt` is a one-shot: the handler
  clears TXIE before it runs the callback.
- RCIF is read-only and clears when RC1REG is read with the FIFO empty. The
  FIFO is two deep; a third byte raises OERR, cleared by dropping CREN
  (`EPIC_USART_ClearOverrun`).
- 9-bit: set TX9D before writing TX1REG. RX9D belongs to the byte at the
  FIFO head and must be read before RC1REG: `EPIC_USART_GetRX9D` does that
  by hand, the RX handler latches it for callbacks
  (`EPIC_USART_GetLastRX9D`).
- Pins: TX needs `EPIC_PPS_MapOutput(..., PPS_OUTPUT_EUSART_TX)` and a TRIS
  output, RX needs `EPIC_PPS_MapInput(PPS_INPUT_EUSART_RX, ...)` and a
  digital input (see PPS for the ports that work).
- Not driven: synchronous mode, auto-baud, break, wake-up, SCKP.

## MSSP bits

MSSP1 as SPI master or I2C master; the module matches the sibling
PIC16F193X MSSP (DS41364B). Bit positions are DFP-checked, the DS page
cites are unchecked.

| SSPM | Mode | Period (ticks of Fosc/4) |
|------|------|--------------------------|
| 0000 | SPI master Fosc/4 | 1 |
| 0001 | SPI master Fosc/16 | 4 |
| 0010 | SPI master Fosc/64 | 16 |
| 1000 | I2C master, Fosc/(4*(SSP1ADD+1)) | SSP1ADD + 1 |
| 1010 | SPI master, Fosc/(4*(SSP1ADD+1)) | SSP1ADD + 1 |

The Timer2 clock mode (0011) waits for the Timer2 port; the slave modes
are not driven.

- `Init` writes SSP1CON1 = 0 first, then SSP1STAT (SMP, CKE), SSP1ADD and
  SSP1CON2, and SSP1CON1 (SSPM, CKP, SSPEN) last. In I2C mode CKE and CKP
  stay clear and SMP is the slew-rate control: set for 100 kHz and 1 MHz,
  clear for 400 kHz. `SSP_ComputeADD` rounds the divisor up so the bus is
  never faster than asked.
- SPI: a SSP1BUF write starts a byte, BF and SSP1IF set when it finishes,
  and the read clears BF. A write during a transfer sets WCOL and sends
  nothing; a byte that ends while BF is still set sets SSPOV and is lost.
- I2C: each call sets one SSP1CON2 bit (SEN, RSEN, PEN, RCEN, or ACKDT then
  ACKEN) and returns. The hardware clears the bit when the operation ends
  and sets SSP1IF; `EPIC_SSP_I2C_IsIdle` also waits out a byte write
  (SSP1STAT.RW). ACKSTAT is 0 when the slave acknowledged the last written
  byte. Bus collision is PIR3.BCL1IF, polled, with no handler.
- Pins: SPI needs SCK and SDO outputs (`PPS_OUTPUT_SSP1_SCK`, `_SDO`) and
  SDI as an input (`PPS_INPUT_SSP1DAT`), I2C drives SCL and SDA through the
  same SCK and SDO output codes and reads them back through
  `PPS_INPUT_SSP1CLK` and `PPS_INPUT_SSP1DAT`; the bus needs external
  pull-ups.

## PPS

PPS routes peripherals to pins; the HAL never touches TRIS or ANSEL, so
pair every mapping with `EPIC_GPIO_Init`. Addresses are 0xE8F.. and
0xF10.. on five parts and 0x1E8F.. and 0x1F10.. on 16F18857/16F18877. All
values below come from the DFP EDC (`ppsval`, POR and impl masks); the
datasheet tables are unchecked.

| Input | Register | POR pin | Field |
|-------|----------|---------|-------|
| T0CKI | T0CKIPPS | RA4 | 4 bits: port A or B |
| MSSP1 clock | SSP1CLKPPS | RC3 | port<<3 \| pin |
| MSSP1 data | SSP1DATPPS | RC4 | port<<3 \| pin |
| MSSP1 SS | SSP1SSPPS | RA5 | port<<3 \| pin |
| EUSART RX | RXPPS | RC7 | port<<3 \| pin |

Port field: A 0, B 1, C 2, D 3 (40-pin parts). Output codes written to
RxyPPS: 0x00 port latch, 0x10 EUSART TX, 0x14 MSSP1 SCK/SCL, 0x15 MSSP1
SDO/SDA (the EDC lists many more; they belong to unported peripherals).

Which ports a function reaches is not in the EDC. MPLAB SIM 6.35 raises
W1600 ("invalid attempt to map pin") for TX, SCK and SDO on PORTA and
PORTD pins, for RX on PORTA and PORTD, and for SS1 on PORTB and PORTC;
every other combination tried (TX, SCK, SDO on PORTB and PORTC, RX on B
and C, SS1 on A and D, SSP1CLKPPS and SSP1DATPPS on any port) maps clean.
The driver writes any valid-looking pin and leaves the choice to the
caller: use PORTB or PORTC for the EUSART and MSSP outputs.

Lock: `EPIC_PPS_Unlock` and `EPIC_PPS_Lock` run the 0x55, 0xAA key writes
and then a single `bcf` or `bsf` on PPSLOCKED, consecutively with
interrupts masked, as a platform macro (`epic_pps_unlock_sequence` and
`epic_pps_lock_sequence`). The last step must be a bit instruction: XC8
compiles `reg &= ~1` to `movlw 0xFE; andwf`, and MPLAB SIM does not take
that (nor a whole-register `movwf`) as the unlock. PPSLOCKED is clear
after reset, so mapping needs no unlock. With PPS1WAY set (the XC8
default) the hardware lets the bit be cleared and set once; MPLAB SIM
accepts one sequence per reset, a Lock first, and leaves PPSLOCKED
unchanged for an Unlock before it or after it. With PPS1WAY OFF, which the
mdb gate's config sets, the cycle repeats. Map after reset and `Lock` once
when PPS1WAY is on.

## ISR bank rule

XC8 -O2 emits a bank select for every volatile SFR and file-scope
static access, but never for cstack autos: an auto read or written
while BSR holds another bank hits the wrong address. Read in the
family's own `.s` (the `ioc_capture` switch form first): the restore
`movlb` is missing, `IOCBF` never clears, and the mdb gate stays red.
The same `.s` shows a static pinned to bank 2 getting a correct
`movlb 2` before every access, so the rule is proven both ways.
Therefore ISR-reachable code (the vector, the dispatcher, the IOC,
Timer0, EUSART and MSSP handlers) keeps no cstack autos: flag snapshots are file-scope
statics, tests run on WREG, and every SFR names its register literally.
Normal (non-ISR) driver code is unaffected: calls reset the tracking
there, and the gate's register readbacks prove it.

## SIM gaps

MPLAB SIM ignores a software write to `PIR0.IOCIF`: the flag never
sets, so a software-triggered IOC never vectors in the gate. The
`gpio-sim` probe checks the write reads back first and runs its live
vector section (callback runs, both flags clear) only where it does:
the host sim and real silicon. On the SIM gate it covers the register
images and the IOC enable path. A software write to `IOCxF` does land
in SIM (the gate observes `IOCBF` set), only the aggregate `IOCIF`
does not follow it.

Timer0 runs freely under `run`: an 8-tick overflow lands inside
main's post-Start read window, so the gate never asserts a post-Start
image with the stop-callback armed (see Timer0 bits). The LC1 row has no
stimulus model anywhere and never ticks; T0CKIPPS edges come from pins
the host sim drives, and the SIM gate drives none.

MPLAB SIM models the EUSART (TXIF, TRMT, the one-shot TX interrupt all
behave as on the host sim) but does not shift MSSP1 data: after a
SSP1BUF write BF and SSP1IF never set, so the gate checks the SPI and I2C
register images and runs the SPI byte only where BF sets (host sim,
silicon). A software write to `PIR3.SSP1IF` does read back and vector,
unlike `PIR0.IOCIF`. The RX interrupt has no stimulus on the SIM gate (RCIF is
read-only), so its handler is checked in the generated `.s`: both latches
are statics, every SFR access carries its own bank select, no autos.
PPS mappings the SIM rejects print W1600 and still write the register
(see PPS); the gate maps only combinations that map clean.

## Weak pull-ups

Each port's `WPUx` register enables its own pull-ups directly; the pack
ships no `OPTION_REG` for this part, so there is no global enable to
manage. Reset state is `WPUx=0x00`: pull-ups stay off until set.

## Interrupt vector

Single vector, no priority. Hardware saves context automatically
(`common/MANUAL.md` §6). The ISR vector is in
`src/target/pic16f188xx_isr_vector.c`.

## Host simulation model

`pic16f188xx_sim_step` refreshes PORTx from latch and driven inputs,
then samples IOC on every port: an edge matching the port's IOCxP/IOCxN
mask sets that port's IOCxF flags and PIR0.IOCIF. The sim callback runs
only under IOCIF, IOCIE, PEIE, and GIE. Pin reads are not gated on
ANSEL (see README known limits). Timer0 advances per tick with
prescaler, 8/16-bit counting, postscaler and T0OUT toggle modelled;
only the internal sources (FOSC/4, HFINTOSC, LFINTOSC, SOSC) tick, and
delivery to the dispatcher gates on TMR0IF, TMR0IE, PEIE and GIE like
silicon. One divergence: a full T0CON0 write clears T0OUT in the sim
array, while silicon preserves the read-only bit; tests pin T0CON0
images only where no full write intervenes.

The EUSART, MSSP1 and PPSLOCK models are in `pic16f188xx_sim_serial.c`.
Registers with side effects go through `epic_sfr_read8`/`epic_sfr_write8`,
which the host platform header routes to hooks (`EPIC_REG8` stays the raw
array cell, and a direct `EPIC_REG8` poke skips the hook): a TX1REG write
loads the transmitter, an RC1REG read pops the receive FIFO, an SSP1BUF
write starts a byte, and the read-only bits (TRMT, RX9D, OERR, FERR,
RCIDL, the SSP1STAT status bits, ACKSTAT, TXIF, RCIF) keep their hardware
value across a software write. Time is instruction ticks: the EUSART bit
time is D * (n + 1) / 4 ticks and an SPI or I2C period is 1, 4, 16 or
SSP1ADD + 1 ticks per the SSPM mode. Probes stimulate it with
`pic16f188xx_sim_usart_rx_inject`, `_spi_set_miso`/`_set_loopback` and one
scripted I2C slave (`_i2c_set_slave`) whose bus events land in a trace
(`_i2c_trace`). Not modelled: pin routing (the serial models ignore
PPS), framing errors, bus collision, the slave modes and PPS1WAY.
