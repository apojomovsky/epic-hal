# PIC16F188xx register reference

Per-peripheral register reference for the GPIO, interrupt-on-change,
Timer0, Timer1 and Timer2 sources in this HAL. Shared conventions
(naming, handle pattern, harness, interrupt model) are in
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
  bot-walled against plain scripted fetch, so only the cites that name
  a checked page or register below were read against it. A value or
  cite without one is still DFP-derived and **unverified** against the
  datasheet.
- Timer0 encodings (clock sources, prescaler, postscaler, T0OUT) cite
  DS40001825F §27, Registers 27-1/27-2 instead: the sibling 18857/77
  datasheet documents this exact Timer0 module (EDC _modsrc
  DOS-01631_tmr_mid0_v1_8bit on both), read as text, not from a
  bot-walled PDF.
- Timer1 and Timer2 encodings (clock selects, gate sources, T2HLT
  modes, reset sources) and the per-pin PPS defaults cite DS40001802H,
  the family datasheet, §28 (Registers 28-1..28-4), §29 (Registers
  29-1..29-4) and Table 13-1. That revision downloads with a browser
  user agent, was read as text, and its Timer1/2 register bit tables
  match DS40001825F's (the 18857/77 parts share the modules).

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
| TMR1L    | 0xFF  | xxxxxxxx   | Counter low byte; a read latches TMR1H while RD16 is set |
| TMR1H    | 0xFF  | xxxxxxxx   | Counter high byte (buffered while RD16 is set) |
| T1CON    | 0x37  | --00-000   | ON, RD16, SYNC (set: not synchronized), CKPS |
| T1GCON   | 0xFC  | 00000x--   | GE, GPOL, GTM, GSPM, GGO/DONE (hardware-cleared), GVAL (read-only) |
| T1GATE   | 0x1F  | ---00000   | GSS gate source select                   |
| T1CLK    | 0x0F  | ----0000   | CS clock source select                   |
| TMR2     | 0xFF  | 00000000   | Counter                                  |
| PR2      | 0xFF  | 11111111   | Period (EDC name T2PR)                   |
| T2CON    | 0xFF  | 00000000   | ON, CKPS (3 bits), OUTPS                 |
| T2HLT    | 0xFF  | 00000000   | PSYNC, CKPOL, CKSYNC, MODE (5 bits)      |
| T2CLKCON | 0x0F  | ----0000   | CS clock source select                   |
| T2RST    | 0x1F  | ---00000   | RSEL external reset signal select        |
| PIR4     | 0x3F  | --000000   | TMR1IF bit 0, TMR2IF bit 1 used (bits 2..5 are Timer3..6) |
| PIE4     | 0x3F  | --000000   | TMR1IE bit 0, TMR2IE bit 1 used          |

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
| TMR1L    | 0x20C   |
| TMR1H    | 0x20D   |
| T1CON    | 0x20E   |
| T1GCON   | 0x20F   |
| T1GATE   | 0x210   |
| T1CLK    | 0x211   |
| TMR2     | 0x28C   |
| PR2      | 0x28D   |
| T2CON    | 0x28E   |
| T2HLT    | 0x28F   |
| T2CLKCON | 0x290   |
| T2RST    | 0x291   |
| PIR4     | 0x710   |
| PIE4     | 0x71A   |
| ANSELD   | 0xF59   |
| WPUD     | 0xF5A   |
| ANSELE   | 0xF64   |
| WPUE     | 0xF65   |

## Interrupt-on-change bits

| Bit | Name     | Use                                                              |
|-----|----------|------------------------------------------------------------------|
| PIR0<4> | IOCIF | Set when any enabled IOCx flag is set. Cleared by the handler. |
| PIE0<4> | IOCIE | Gates the IOC source. The dispatcher calls IOC only when IOCIF and IOCIE are both set. |
| PIR0<5> | TMR0IF | Set on 8-bit match or 16-bit rollover, postscaled. Cleared by the handler. |
| PIE0<5> | TMR0IE | Gates the Timer0 source. Set by Init only with a callback. |
| PIR4<0> | TMR1IF | Set when TMR1H:TMR1L rolls over FFFFh to 0000h. Cleared by the handler. |
| PIE4<0> | TMR1IE | Gates the Timer1 source. Set by Init only with a callback. |
| PIR4<1> | TMR2IF | Set on the postscaled TMR2 = PR2 match. Cleared by the handler. |
| PIE4<1> | TMR2IE | Gates the Timer2 source. Set by Init only with a callback. |
| INTCON<6> | PEIE  | Set by `EPIC_IRQ_Enable` on any source. Exposes the PIR0 and PIR4 sources. |
| INTCON<7> | GIE   | Global interrupt enable.                                         |

## Timer0 bits

Clock source (T0CON1<T0CS2:T0CS0>, DS40001825F Register 27-2):
000 T0CKIPPS, 001 T0CKIPPS inverted, 010 FOSC/4, 011 HFINTOSC,
100 LFINTOSC, 101 SOSC, 110 LC1_out, 111 reserved (no driver value).
T0CKIPPS needs a PPS pin mapping the HAL does not set up yet.
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

## Timer1 bits

Clock source (T1CLK<CS3:CS0>, DS40001802H Register 28-3): 0000
T1CKIPPS, 0001 FOSC/4, 0010 FOSC, 0011 HFINTOSC, 0100 LFINTOSC, 0101
MFINTOSC, 0110 SOSC, 0111 CLKR, 1000 TMR0 overflow, 1010 TMR3
overflow, 1011 TMR5 overflow, 1100..1111 LC1..LC4_out; 1001 is
reserved on Timer1 (no driver value). Prescaler (T1CON<CKPS1:CKPS0>)
divides by 1, 2, 4 or 8. FOSC clocks the counter four times per
instruction cycle (§28.2.1). The PPS inputs reset to T1CKIPPS RC0,
T1GPPS RB5 and T2INPPS RC3 (Table 13-1); the HAL has no PPS setup, so
only those pins are usable.

Gate (T1GCON, T1GATE, §28.7): GE enables it, GPOL=1 counts while the
gate is high, GTM turns rising edges of the source into a toggled
level (measures a full cycle), GSPM lets one acquisition through per
GGO/DONE set, which hardware clears on the trailing edge, and GVAL
reports the gate state even with GE clear. Gate sources (T1GATE<GSS4:
GSS0>, Register 28-4): 00000 T1GPPS, 00001 TMR0 overflow, 00011
TMR2_postscaled, 00100 TMR3 overflow, 00101 TMR4_postscaled, 00110
TMR5 overflow, 00111 TMR6_postscaled, 01000/01001 SMT1/SMT2 match,
01010..01110 CCP1..5_out, 01111/10000 PWM6/7_out, 10001 DDS_out,
10010/10011 C1/C2OUT_sync, 10100 ZCD1, 10101..11000 LC1..4_out;
00010 (Timer1 itself) and 11001..11111 are reserved. The gate-event
interrupt (TMR1GIF, PIR5<0>) has no driver yet.

`EPIC_TIMER1_Init` sets RD16 so `EPIC_TIMER1_ReadCounter` (TMR1L, then
TMR1H) cannot tear on rollover, and never sets GGO/DONE: arm a single
pulse with `EPIC_TIMER1_GateArm`. Init programs the clock, gate and
prescaler registers halted; Start reloads the counter and writes
T1CON with ON. Image after Init with the default handle: T1CLK 0x01,
T1GATE 0x00, T1GCON 0x00, T1CON 0x02; after Start T1CON 0x03. T1GCON
images in the mdb gate mask GVAL, which tracks the pin live.

## Timer2 bits

Clock source (T2CLKCON<CS3:CS0>, DS40001802H Register 29-1): 0000
T2CKIPPS, 0001 FOSC/4, 0010 FOSC, 0011 HFINTOSC, 0100 LFINTOSC, 0101
MFINTOSC/16, 0110 SOSC, 0111 CLKR, 1000 NCO, 1001 ZCD1, 1010..1101
LC1..LC4_out; 1110 and 1111 are reserved. T2CON (Register 29-2): ON
bit 7, CKPS<2:0> divides by 2^N up to 1:128, OUTPS<3:0> divides by
N+1 up to 1:16. TMR2 counts 0..PR2 and clears on the clock after the
match, so TMR2IF fires every prescaler x (PR2+1) x postscaler input
cycles. Writing T2CON or TMR2 clears the prescaler and postscaler
counters; clearing ON resets all counters and the mode state machine
but leaves TMR2 untouched.

Hardware limit modes (T2HLT<MODE4:MODE0>, Register 29-3, Table 29-1):
the free-running group 0x00 (software gate), 0x01/0x02 (hardware gate
high/low), 0x03/0x04/0x05 (reset on any, rising or falling edge of
the reset signal), 0x06/0x07 (held in reset while low/high); the
one-shot group 0x08 (software start), 0x09..0x0F (edge start, with
edge or level reset); the monostable group 0x11..0x13; and the level
triggered one-shots 0x16/0x17. 0x10, 0x14, 0x15 and 0x18..0x1F are
reserved. T2HLT also carries PSYNC (bit 7), CKPOL (bit 6) and CKSYNC
(bit 5). The reset signal (T2RST<RSEL4:RSEL0>, Register 29-4) is the
T2INPPS pin for 00000, TMR4/TMR6 postscaled for 00010/00011,
CCP1..5_out 00100..01000, PWM6/7_out 01001/01010, C1/C2OUT_sync
01011/01100, ZCD1 01101, LC1..4_out 01110..10001; 00001 (Timer2
itself) and 10010..11111 are reserved.

`EPIC_TIMER2_Init` programs T2CON ratios with ON clear, then T2CLKCON,
T2RST and T2HLT; Start writes PR2, zeroes TMR2 and writes T2CON with
ON. Image after Init with the default handle and PR2 7: T2CLKCON 0x01,
T2RST 0x00, T2HLT 0x00, T2CON 0x00; after Start T2CON 0x80.

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

## ISR bank rule

XC8 -O2 emits a bank select for every volatile SFR and file-scope
static access, but never for cstack autos: an auto read or written
while BSR holds another bank hits the wrong address. Read in the
family's own `.s` (the `ioc_capture` switch form first): the restore
`movlb` is missing, `IOCBF` never clears, and the mdb gate stays red.
The same `.s` shows a static pinned to bank 2 getting a correct
`movlb 2` before every access, so the rule is proven both ways.
Therefore ISR-reachable code (the vector, the dispatcher, the IOC and
Timer0..Timer2 handlers) keeps no cstack autos: flag snapshots are file-scope
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
image with the stop-callback armed (see Timer0 bits). The T0CKIPPS and
LC1 rows have no stimulus model anywhere and never tick.

Timer1 and Timer2 follow the Timer0 gate shape (callback-free setup
proof, then a vectored proof whose callback stops the timer). Their
gate proofs use RB5 (T1G) and RC3 (T2IN) configured as outputs: MPLAB
SIM feeds the output latch back through the POR PPS mapping, so the
gate level is deterministic without a stimulus file. MPLAB SIM prints
a W0106 partial-support warning for TMR5 at load; it is unrelated to
the Timer1/Timer2 sections. The gate-event interrupt (TMR1GIF) has no
driver, and the Timer2 edge-start, monostable and level-start modes
have no behavioural coverage in either the host sim (it freezes the
counter) or the mdb gate (only mode 0x01 is exercised live).

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

Timer1 and Timer2 advance per tick the same way. Timer1 counts for the
internal sources (FOSC/4, HFINTOSC, LFINTOSC, MFINTOSC, SOSC, and FOSC
at four counts per tick), for each Timer0 overflow pulse, and for each
rising edge of RC0 (the POR T1CKIPPS pin); its gate reads RB5 (POR
T1GPPS), the Timer0 overflow pulse or the Timer2 postscaled pulse, and
models GPOL, GTM (toggle flip-flop cleared while the timer is off),
GSPM with GGO/DONE (cleared on the trailing edge) and GVAL. Timer2
ticks the same internal sources and models modes 0x00..0x07 and the
software one-shot 0x08 against RC3 (POR T2INPPS); the edge-start,
monostable and level-start modes and every unmodelled clock row
(T2CKIPPS included) freeze the counter, and every unmodelled gate and
reset row reads low, which holds a gated counter except in active-low
gate mode, where a low signal lets it run.
The pin rows read the PORTx array, so a test drives them with
`pic16f188xx_sim_drive_input` ('C' 0, 'B' 5, 'C' 3) or by making the
pin an output; an output pin reads back its latch, and pin reads are
not gated on ANSEL. The sim does not model the RD16 buffer or the
prescaler clear on a TMR1L/TMR2 write: counter registers are plain
memory, and the prescaler clears only while the timer is off.
