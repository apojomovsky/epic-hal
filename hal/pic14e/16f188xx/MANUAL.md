# PIC16F188xx register reference

Per-peripheral register reference for the GPIO and interrupt-on-change
sources in this HAL. Shared conventions (naming, handle pattern,
harness, interrupt model) are in
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
| ANSELD   | 0xF59   |
| WPUD     | 0xF5A   |
| ANSELE   | 0xF64   |
| WPUE     | 0xF65   |

## Interrupt-on-change bits

| Bit | Name     | Use                                                              |
|-----|----------|------------------------------------------------------------------|
| PIR0<4> | IOCIF | Set when any enabled IOCx flag is set. Cleared by the handler. |
| PIE0<4> | IOCIE | Gates the IOC source. The dispatcher calls IOC only when IOCIF and IOCIE are both set. |
| INTCON<6> | PEIE  | Set by `EPIC_IRQ_Enable` on IOC. Exposes PIR0 sources. |
| INTCON<7> | GIE   | Global interrupt enable.                                         |

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
Therefore ISR-reachable code (the vector, the dispatcher, the IOC
handler) keeps no cstack autos: flag snapshots are file-scope statics,
tests run on WREG, and every SFR names its register literally. Normal
(non-ISR) driver code is unaffected: calls reset the tracking there,
and the gate's register readbacks prove it.

## SIM gaps

MPLAB SIM ignores a software write to `PIR0.IOCIF`: the flag never
sets, so a software-triggered IOC never vectors in the gate. The
`gpio-sim` probe checks the write reads back first and runs its live
vector section (callback runs, both flags clear) only where it does:
the host sim and real silicon. On the SIM gate it covers the register
images and the IOC enable path. A software write to `IOCxF` does land
in SIM (the gate observes `IOCBF` set), only the aggregate `IOCIF`
does not follow it.

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
ANSEL (see README known limits).
