# PIC16F1508 / PIC16F1509 register reference

Per-peripheral register reference for the GPIO and interrupt-on-change
sources in this HAL. Shared conventions (naming, handle pattern,
harness, interrupt model) are in
[common/MANUAL.md](../../../common/MANUAL.md). This file covers only
what is specific to this part.

## Sources and verification

- Addresses and implemented-bit masks come from the Microchip DFP
  1.9.258 EDC (`PIC16F1509.PIC`), pinned by the manifest.
  `scripts/gen-sfr.py` rewrites each `PIC_REG_*` address the EDC defines,
  CLC included. Do not hand-edit addresses. The CLC bit masks are
  hand-maintained and cross-checked against the DFP proc header.
- Datasheet section and figure cites are **unverified**. The datasheet
  must be supplied by a human, and this file gets section cites then.
  No value here is taken from a datasheet page.

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
| INTCON<6> | PEIE  | Peripheral interrupt enable (not used by this HAL's IOC path). |
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

## Configurable logic cell (CLC)

Four cells, CLC1..CLC4, at a stride of 8 from CLC1CON at 0xF10 (CLCDATA
at 0xF0F). Addresses are from the DFP EDC. Bit positions below are from
the pinned DFP 1.9.258 proc header and the #361 transcription. Both are
**unverified** against DS40001609E; they are not checked against the
datasheet yet.

| Register | Bits used                                             | Source         |
|----------|-------------------------------------------------------|----------------|
| CLCxCON  | LCxMODE 2:0, INTN 3, INTP 4, OUT 5, OE 6, EN 7        | DFP + #361     |
| CLCxPOL  | G1POL..G4POL 3:0, POL 7                               | DFP only       |
| CLCxSEL0 | D1S 2:0, D2S 6:4                                      | DFP + #361     |
| CLCxSEL1 | D3S 2:0, D4S 6:4                                      | DFP + #361     |
| CLCxGLS0..3 | D1N..D4T, two bits per gate input (N then T)       | DFP only       |

The input-select codes (the DS40001609E input-select table, not verified here) are not in this HAL (#384).
The driver passes raw 0..7 mode and source codes through and validates
only the range.
Interrupt routing: PIR3 bits 0..3 are CLC1IF..CLC4IF and PIE3 bits 0..3 are
CLC1IE..CLC4IE (DFP 1.9.258 proc header `pic16f1509.h`; EDC `PIR3` at 0x13,
`PIE3` at 0x93). Both are **unverified** against DS40001609E. `EPIC_IRQ_Enable`
on a CLC source sets PIE3 and INTCON.PEIE. The dispatcher runs
`CLCn_IRQHandler` only when CLCnIF and CLCnIE are both set, and that handler
clears CLCnIF. No application callback exists yet, so an enabled CLC source
wakes the vector and is acknowledged, but the application cannot act on it.

MPLAB-SIM throws on some CLC register access sequences, so the mdb gate
does not exercise CLC (follow-up #383). The CLC driver is covered by the
host probes `example_clc` (registers) and `example_clc_irq` (interrupt routing) only.
