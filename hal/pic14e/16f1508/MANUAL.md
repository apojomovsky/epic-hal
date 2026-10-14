# PIC16F1508 / PIC16F1509 register reference

Per-peripheral register reference for the GPIO, interrupt-on-change, timer,
and CLC sources in this HAL. Shared conventions (naming, handle pattern,
harness, interrupt model) are in
[common/MANUAL.md](../../../common/MANUAL.md). This file covers only
what is specific to this part.

## Sources and verification

- Addresses and implemented-bit masks come from the Microchip DFP
  1.9.258 EDC (`PIC16F1509.PIC`), pinned by the manifest.
  `scripts/gen-sfr.py` rewrites each `PIC_REG_*` address the EDC defines,
  CLC included. Do not hand-edit addresses. The CLC bit masks are
  hand-maintained and cross-checked against the DFP proc header.
- Datasheet: DS40001609E (PIC16(L)F1508/9), published at
  <https://ww1.microchip.com/downloads/en/DeviceDoc/40001609E.pdf>. Page cites
  below are its printed page numbers. Checked so far: the PIR3/PIE3 CLC bits and
  addresses (#388), the CLCxCON bit layout (#395), the CLCxPOL, CLCxSEL0/1 and
  CLCxGLS0..3 bit positions with the Table 24-1 input-select codes (#384), the
  Figure 24-3 gate roles behind the host-sim mode map (#384, #400), the T1CON
  and T2CON bit layouts with the PIR1/PIE1 TMR1/TMR2 bits (#415), and the
  TMR1L/H, T1CON, T1GCON, TMR2, PR2 and T2CON addresses from the Table 3-7
  memory map (#415). A value or cite without a datasheet page here is still
  DFP/EDC-derived and **unverified**.

## Implemented bits (from EDC impl masks)

| Register | Impl  | Meaning                                                    |
|----------|-------|------------------------------------------------------------|
| PORTA    | 0x3F  | RA0..RA5                                                   |
| LATA     | 0x37  | LATA3 unimplemented (RA3 is input-only, MCLR)              |
| TRISA    | 0x3F  | RA0..RA5                                                   |
| ANSELA   | 0x17  | Analog select on RA0, RA1, RA2, RA4 only                   |
| WPUA     | 0x3F  | Weak pull-up per PORTA pin                                 |
| PORTB    | 0xF0  | RB4..RB7 only (RB0..RB3 do not exist)                      |
| LATB     | 0xF0  | Latch follows the implemented pins                         |
| TRISB    | 0xF0  | Direction for RB4..RB7                                     |
| ANSELB   | 0x30  | Analog select on RB4 (AN10) and RB5 (AN11)                 |
| WPUB     | 0xF0  | Weak pull-up for RB4..RB7                                  |
| IOCBP, IOCBN, IOCBF | 0xF0 | Interrupt-on-change for RB4..RB7 only             |
| PORTC    | 0xFF  | RC0..RC7 (width from EDC; not checked against a datasheet)  |
| LATC, TRISC | 0xFF | Follows the PORTC pins                                 |
| ANSELC   | 0xCF  | Analog select on the implemented analog PORTC pins         |

`ANSELA`'s field note in the EDC is 0x3F, but the implemented-bit mask
is 0x17. The driver uses the implemented-bit mask. Writes outside it
are dropped by hardware.

## Register addresses

Addresses are the `PIC_REG_*` values in `include/pic16f1508_sfr.h`
(generated). The table below only lists the ones this HAL uses:

| Name     | Address |
|----------|---------|
| INTCON   | 0x0B    |
| OPTION   | 0x95    |
| TMR1L    | 0x16    |
| TMR1H    | 0x17    |
| T1CON    | 0x18    |
| T1GCON   | 0x19    |
| TMR2     | 0x1A    |
| PR2      | 0x1B    |
| T2CON    | 0x1C    |
| PORTA    | 0x0C    |
| PORTB    | 0x0D    |
| PORTC    | 0x0E    |
| LATA     | 0x10C   |
| LATB     | 0x10D   |
| LATC     | 0x10E   |
| TRISA    | 0x8C    |
| TRISB    | 0x8D    |
| TRISC    | 0x8E    |
| ANSELA   | 0x18C   |
| ANSELB   | 0x18D   |
| ANSELC   | 0x18E   |
| WPUA     | 0x20C   |
| WPUB     | 0x20D   |
| IOCAP    | 0x391   |
| IOCAN    | 0x392   |
| IOCAF    | 0x393   |
| IOCBP    | 0x394   |
| IOCBN    | 0x395   |
| IOCBF    | 0x396   |

IOCAP, IOCAN, and IOCAF are generated but have no driver body here,
because IOC is implemented on PORTB only.

## Interrupt-on-change bits

| Bit | Name     | Use                                                              |
|-----|----------|------------------------------------------------------------------|
| INTCON<0> | IOCIF | Set when any enabled IOCB flag is set. Cleared by the handler. |
| INTCON<3> | IOCIE | Gates the IOC source. The dispatcher calls IOC only when IOCIF and IOCIE are both set. |
| INTCON<6> | PEIE  | Set by `EPIC_IRQ_Enable` on a CLC or timer source. Exposes PIR1, PIR2 and PIR3 sources; the dispatcher acknowledges enabled unowned ones (see CLC interrupt routing). |
| INTCON<7> | GIE   | Global interrupt enable.                                         |

`IOCBF` flags are cleared in the handler one captured bit at a time with a
single-bit clear, never by a masked write of the whole register, so a flag
hardware sets during the handler is not overwritten. An edge that lands
after the clear re-raises IOCIF. A second edge on an already-captured bit
merges into that capture; the PORTB byte passed to the callback is read
after the clear, so it reflects the pin state at that read.

## Weak pull-ups

`OPTION_REG<7>` is `nWPUEN` (active low: 0 enables pull-ups on any
set WPU bit, 1 disables them globally). The driver follows this rule:

- Setting any WPU bit clears `nWPUEN` (pull-ups enabled).
- Clearing a WPU bit leaves `nWPUEN` cleared while any other WPU bit
  is still set.
- `nWPUEN` is set (pull-ups disabled globally) only when WPUA and WPUB
  are both zero.

Reset state is WPUA=0x3F and WPUB=0xF0 on target. Code that depends on
pull-up state must clear WPUA and WPUB first.

## Interrupt vector

Single vector, no priority. Hardware saves context automatically
(`common/MANUAL.md` §6). The ISR vector is in
`src/target/pic16f1508_isr_vector.c`.

## Timer1

16-bit timer/counter (DS40001609E §19.0, page 155). TMR1L at 0x16 and TMR1H
at 0x17 hold the count, T1CON at 0x18 holds the clock source, prescaler and
on bit, T1GCON at 0x19 is the gate control. The driver covers the
free-running timer path and the external counter path; the gate stays at
POR (TMR1GE clear, counting regardless of the gate function).

| Register | Bits used                                              |
|----------|--------------------------------------------------------|
| T1CON    | TMR1CS 7:6, T1CKPS 5:4, T1OSCEN 3, T1SYNC 2, TMR1ON 0 (bit 1 unimplemented) |

T1CON is Register 19-1 (page 163). Clock sources: 00 is FOSC/4 (timer),
01 is FOSC, 10 is the T1CKI pin or crystal with T1OSCEN, 11 is LFINTOSC.
The driver accepts 00 and 10 with T1OSCEN clear (T1CKI pin, counter mode),
setting T1SYNC so the input is not resynchronized; 01 and 11 return
`EPIC_INVALID`. Prescaler T1CKPS<1:0> is 1:1, 1:2, 1:4, 1:8. T1CON POR is
0x00 (Table 3-9); the counter holds no POR value. Overflow sets PIR1
TMR1IF (bit 0), enabled by PIE1 TMR1IE (bit 0): Register 7-5 (PIR1,
page 78) and Register 7-2 (PIE1, page 75). T1_overflow feeds the CLC data
inputs at D1 code 6 and D2 code 2 (Table 24-1, page 258): it is the beat
dimmer R source and the counter the quadrature assist feeds.

## Timer2

8-bit timer with PR2 period match (DS40001609E §20.0, page 166). TMR2 at
0x1A holds the count, PR2 at 0x1B holds the period, T2CON at 0x1C holds the
postscaler, on bit and prescaler. TMR2 counts 0..PR2 and resets to 0 on the
cycle it matches PR2, so it never holds PR2+1; PR2 powers up at 0xFF (§20.1).
A write to TMR2 or T2CON clears the prescaler and postscaler counters.

| Register | Bits used                                              |
|----------|--------------------------------------------------------|
| T2CON    | T2OUTPS 6:3, TMR2ON 2, T2CKPS 1:0 (bit 7 unimplemented) |

T2CON is Register 20-1 (page 168). Prescaler T2CKPS<1:0> is 1:1, 1:4, 1:16,
1:64 (all four encodings distinct on this part); postscaler T2OUTPS<3:0> is
1:(N+1) for N 0..15. T2CON POR is 0x00 (Table 3-9). Each match feeds the
postscaler, which sets PIR1 TMR2IF (bit 1), enabled by PIE1 TMR2IE (bit 1):
Register 7-5 (PIR1, page 78) and Register 7-2 (PIE1, page 75). The match
output T2_match also feeds the CLC data inputs at D1 code 7 and D2 code 3
(Table 24-1, page 258), the debouncer slow clock, and the PWM module (§23).

This part has Timer0, Timer1 and Timer2 only (§18, §19, §20); there is no
TMR3, TMR4 or TMR6. The second beat-dimmer pulse source is therefore a
preloaded TMR1 (255 counts to overflow) against TMR2 with PR2 255 (256
counts to match), not a second 8-bit match timer.

## Configurable logic cell (CLC)

Four cells, CLC1..CLC4, at a stride of 8 from CLC1CON at 0xF10 (CLCDATA
at 0xF0F). Addresses are from the DFP EDC and match the Bank 30 map and the
register summary in DS40001609E Table 3-7 (page 25) and Table 3-9 (page 31).
Bit positions: CLCxCON is Register 24-1 (page 263), CLCxPOL is Register 24-2
(page 264), CLCxSEL0 is Register 24-3 (page 265), CLCxSEL1 is Register 24-4
(page 266), CLCxGLS0..3 are Registers 24-5..24-8 (pages 267 to 270), and
CLCDATA is Register 24-9 (page 271).

| Register | Bits used                                             |
|----------|-------------------------------------------------------|
| CLCxCON  | LCxMODE 2:0, INTN 3, INTP 4, OUT 5, OE 6, EN 7        |
| CLCxPOL  | G1POL..G4POL 3:0, POL 7 (bits 6:4 unimplemented)      |
| CLCxSEL0 | D1S 2:0, D2S 6:4 (bits 7 and 3 unimplemented)         |
| CLCxSEL1 | D3S 2:0, D4S 6:4 (bits 7 and 3 unimplemented)         |
| CLCxGLS0..3 | D1N..D4T, two bits per gate input (N then T)       |

Input-select codes are DS40001609E Table 24-1 (page 258), built into
`EPIC_CLC_SourceCode` (`include/peripherals/pic16f1508_clc_src.h`) and covered
by the host probe `example_clc_src`. Each multiplexer sees eight of the 16
LCx_in signals: D1 takes LCx_in[0..7], D2 takes [4..11], D3 takes [8..15], and
D4 takes [12..15] at codes 0..3 then [0..3] at codes 4..7. Rows 0..11 name the
same signal on every cell; rows 12..15 differ per cell. The driver still
validates only the 0..7 range; a multiplexer that cannot see an input yields
`EPIC_CLC_SRC_INVALID`, which `EPIC_CLC_Init` rejects with no register
written.

Edge interrupts: `CLC_HandleTypeDef.InterruptRising` sets LCxINTP (CLCxCON bit 4) and
`InterruptFalling` sets LCxINTN (bit 3), written in the same CON store that sets EN. CLCnIF is
raised on the matching edge of the cell output, after the POL inversion, only when the edge
bit is set (DS40001609E Register 24-1 page 263, section 24.2 page 260). Each field accepts 0 or
1; any other value makes `EPIC_CLC_Init` return `EPIC_INVALID` with no register written. The
datasheet notes that an edge arriving while software clears CLCnIF still sets the flag again.

Interrupt routing: PIR3 bits 0..3 are CLC1IF..CLC4IF and PIE3 bits 0..3 are
CLC1IE..CLC4IE, with bits 7:4 unimplemented in both. Verified against
DS40001609E Register 7-7 (PIR3, page 81), Register 7-4 (PIE3, page 78),
Table 7-1 (page 82) and Table 3-9 (page 27), which also give PIR3 at 0x13 and
PIE3 at 0x93; the EDC `_addr` values agree (PIR3 0x13, PIE3 0x93, impl mask 0x0F).
`EPIC_IRQ_Enable` on a CLC source sets PIE3 and INTCON.PEIE. The dispatcher runs
`CLCn_IRQHandler` only when CLCnIF and CLCnIE are both set, and that handler
clears CLCnIF, then runs the callback stored in `CLC_HandleTypeDef.Callback` for that cell
(NULL runs nothing). The callback runs in ISR context, after the flag is cleared, so an event
that pends again inside it re-raises the vector. `EPIC_CLC_Init` stores the callback before it
sets EN, so an enabled cell never runs the previous callback; `EPIC_CLC_DeInit` clears it.
Because the handler reads the slot from ISR context, disable the CLCn interrupt
(`EPIC_IRQ_DisableSrc`) before reconfiguring a live cell. A rejected `Init` returns `EPIC_INVALID`
and writes nothing, so the previous callback stays in place.
PEIE also exposes the PIR1 and PIR2 sources and PIR3<7:4>. The timer drivers own
PIR1 bits 0 and 1 (TMR1IF/TMR1IE, TMR2IF/TMR2IE): with PEIE set the dispatcher
runs `TIMER1_IRQHandler`/`TIMER2_IRQHandler` when the flag and enable are both
set, and each handler clears its flag before running the stored callback. While
PEIE is set, the dispatcher acknowledges each other source that is both flagged
and enabled: it clears that source's PIE bit, then its flag, one bit at a time,
and runs no handler. So a CLC enable next to a pending PIR1 or PIR2 source no
longer re-enters the vector. Enabling an unowned peripheral interrupt alongside
a CLC source is unsupported: the dispatcher disables it. A flagged source whose
enable is clear stays pending for polling, as does a TMRnIF with PEIE clear.
With PEIE clear the unowned sources cannot reach the vector, and the dispatcher
leaves them alone. PIR3<7:4> are unimplemented on the PIC16F1508
per EDC and Register 7-7 (PIR3 bits 7:4 read as 0). The PIR1, PIR2, PIE1 and PIE2 addresses
(0x11, 0x12, 0x91, 0x92) are confirmed in DS40001609E Table 3-9 (page 27). The PEIE requirement
for every peripheral interrupt is the note under Register 7-4 (PIE3, page 78).

MPLAB-SIM limits the CLC coverage of the mdb gate (all recorded on #383).
The gate configures CLC1 through `EPIC_CLC_Init` and drives gate-polarity
constants through modes 0..3, reading the CON.OUT live output at every step,
plus output-polarity inversion and the EN-cleared forced zero. Mode 3 runs
the TB3273 S-R latch shape (S=G1|G2, R=G3|G4): set, hold, reset, hold-low.
SIM keeps the sequential state across samples, so the holds prove the latch;
S+R priority stays host-only (the sim assumes S wins). Two SIM gaps shape
this: a single store of most CLC1 D3S codes (anything but 0 and 5) to
CLC1SEL1 throws `E0107-SIM ... CLC1SEL1 : null`, with no preceding access
needed (`EPIC_REG8(PIC_REG_CLC1SEL1) = 0x01U` as the only CLC access throws)
and no CON order avoiding it (cleared first and enabled first both throw,
while the same code writes cleanly to CLC2SEL1), so the gate uses D3S 5
(HFINTOSC); and the SIM never sets the CLCDATA mirror copy (CON.OUT reads 1
while CLCDATA stays 0 after a settle cycle), so the gate observes CON.OUT
only. The CLCDATA mirror and the sequential modes 4..7 are covered by the
host probes (`example_clc`, `example_clc_sim`, `example_clc_latch`,
`example_clc_beat`) only.
The SIM prints W0700/W0701 notes during the CLC steps (no propagation
delay modelled; output-as-own-input loop warning). They are benign: no
cell feeds its own output.

The gate also covers the timers: TMR2 with PR2 7 free-runs to a match (flag
within a bounded spin, counter never above PR2, then T2CON 0 and PR2 0xFF),
and TMR1 preloaded to 0xFFF0 overflows to its flag (then T1CON 0 and counter
0). Both spins call `epic_harness_tick`, which pumps the host model and is a
no-op on target, so the same section runs in both builds.

## Host simulation model

`pic16f1508_sim_step` recomputes every cell from its CON, POL, and GLS
registers: gates OR their selected true and negated terms (an empty gate
reads 0 before its GxPOL bit, per Table 24-2 rows 0x00; a gate with both the
true and the negated term of one input selected reads 0, per section 24.1.2),
the mode logic combines the gates, POL inverts the cell, and the result lands
in the CLCDATA bit and the CON.OUT bit. A 0 to 1 edge with INTP set, or a 1 to
0 edge with INTN set, raises the cell's PIR3 flag even with its PIE bit clear;
the sim callback runs only under CLCxIF, CLCxIE, PEIE, and GIE. A disabled cell
reads 0 with no edge and no flag, and keeps its sequential bit.

Gate roles per mode are DS40001609E Figure 24-3 (page 262): S-R latch S=G1|G2
and R=G3|G4; flip-flops clock on the G1 rise with D=G2 and R=G3, S=G4 on the
1-input D, D=G2|G4 on the 2-input D, K=G4 on the J-K (J=G2); transparent latch
D=G2, LE=G3, S=G4, R=G1. Two figure gaps stay open. The figure draws AND gates
in front of the OR-XOR output while Register 24-1 (page 263) and section
24.1.3 name mode 1 OR-XOR; the model follows the name, and the mdb gate only
asserts vectors where both readings agree. The datasheet gives no priority
between S and R, so S wins here by assumption. Covered by the host probe
`example_clc_sim`.

One seam stays open. SEL codes are stored but not decoded: the Dx levels
come from `pic16f1508_sim_drive_clc_input`, a host-only hook. LCxOE does not
drive a pin in the sim: the output-pin mapping is unverified, so only the
mirrors observe it.

Timers advance one instruction cycle per tick: TMR2 applies the T2CKPS
prescaler, matches TMR2 against PR2 with reset to 0, and divides matches
through the T2OUTPS postscaler to TMR2IF; TMR1 applies the T1CKPS prescaler
and counts TMR1H:TMR1L to overflow and TMR1IF. Only the internal FOSC/4
source advances; any other TMR1CS halts the model. The T1CKI counter path
runs through `pic16f1508_sim_timer1_external_pulse`, one call per rising
edge, prescaled the same way. A raised flag fires the sim callback, which
routes through the shared dispatcher exactly like the vector. Covered by the
host probes `example_tmr1` and `example_tmr2`, including the 256:255 beat
ratio of TMR2 at PR2 255 against TMR1 preloaded to 0xFF01.
