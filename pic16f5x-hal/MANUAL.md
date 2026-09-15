# PIC16F5x HAL manual

Register facts for the PIC16F5x family that are not shared conventions
(those live in `epic-common/MANUAL.md`) and not driver behavior (each
driver cites its own datasheet sections). Everything here is cited to
DS41213D, cross-checked against the DFP
(`Microchip.PIC16Fxxx_DFP`, `edc/PIC16Fnnn.PIC` and
`xc8/pic/include/proc/pic16f54.h`).

## Family shape

Five parts on the 12-bit baseline core (`edc:arch="16c5x"`,
`instructionSetId="pic12c5xx"`), two address shapes. The 18/28/40-pin
parts (16F54/57/59) are single-bank flat-RAM dice; the 20-pin parts
(16F505/506) bank GPR 4 ways through FSR<6:5>.

| part | pins | flash | GPR | banks | ports | extras |
|---|---|---|---|---|---|---|
| 16F54 | 18 | 512 W | 25 B | flat | A (RA0..3), B | - |
| 16F57 | 28 | 2048 W | 72 B | flat | A, B, C | - |
| 16F59 | 40 | 2048 W | 73 B | flat | A, B, C, D, E | - |
| 16F505 | 20 | 1024 W | 72 B | 4x FSR<6:5> | B, C | OSCCAL (file 0x05) |
| 16F506 | 20 | 1024 W | 72 B | 4x FSR<6:5> | B, C | OSCCAL, comparator, ADC |

GPR is 0x07..0x1F on the 16F54 (25 B, DS41213D section 1.0), the
tightest budget in this repo; every real-target example must fit both
512 words of flash and 25 B of RAM. The 505/506 have no PORTA: OSCCAL
takes file 0x05 and the 20-pin die has no RA0 output pins (DS41319).

## No interrupts, no WDT sleep

No part in this family has an interrupt vector, INTCON, PIE/PIR or
RETFIE (DS41213D section 4.0: the baseline core keeps the 2-level
hardware stack for CALL/RETLW only, and TMR0 has no overflow flag).
The HAL's IRQ API is a no-op stub (`src/core/pic16_irq_stub.c`,
`pic16_irq_dispatch.c`); `EPIC_IRQ_*` and `epic_dispatch_all_irqs`
are contract-parity no-ops and Timer0 is polled. There is also no
SLEEP/CLRWDT distinction available to C: the WDT is a config-bit
feature (see below), and `EPIC_WDT_Refresh` clears the WDT through the
`clrwdt` instruction where a refresh loop can be afforded.

## Control-space TRIS and OPTION

TRISA/TRISB (and TRISC..TRISE on the wider parts) and OPTION are not
file registers: they are written with the dedicated `tris <f>` and
`option` instructions (DS41213D section 12.0, Table 12-1) and have no
read path. The HAL shadows the direction byte in GPR and re-emits the
whole TRIS on every transition (`src/peripherals/pic16f5x_gpio.c`).

- XC8 target: `extern volatile __control` externs pinned to the TRIS
  select addresses; XC8 lowers an assignment to the native instruction
  (probed: `TRISA = v` becomes `movf v,w; tris 5`).
- Host sim: writes route into `pic16f5x_sim_trisa..e` shadow bytes so
  tests can observe the programmed direction.
- epic-cc: literal scratch byte at 0x0C + inline `movf 12, w`/`tris N`
  (the baseline asm pass resolves only literal operands; epic-cc#437
  tracks the p16f54 RAM-model fix that lets the epic-cc blink build).

OPTION bits (DS41213D Register 9-1): PS<2:0> prescaler ratio, PSA
(prescaler assign), T0SE, T0CS. There is no RBPU (no weak pull-ups on
the baseline die) and no INTEDG (RB0 has no interrupt), so the HAL's
OPTION surface is exactly those five bits.

## Register placement (DS41213D sections 2.0, 3.0)

Core file registers, identical on every part: INDF 0x00, TMR0 0x01,
PCL 0x02, STATUS 0x03, FSR 0x04, PORTB 0x06. PORTA 0x05 on 16F54/57/59;
OSCCAL 0x05 on 16F505/506; PORTC 0x07 on 16F57/59/505/506; PORTD/PORTE
0x08/0x09 on the 16F59. STATUS carries C/DC/Z/nPD/nTO plus PA<2:0>
(program-page select PC<10:8>, not a bank select); the 505/506 drop
PA1/PA2 (2-bit FSR bank addressing, DS41319).

The sfr-map audit cross-checks every `PIC_REG_*` address and bit row
against the five DFP headers; OPTION's bits are DFP_MISSING_OK (control
space carries no `_POSN` macros) and are datasheet facts instead.

## Config word (DS41213D section 14.1)

`#pragma config` surface: OSC (LP/XT/HS/RC), WDT (ON/OFF, not WDTE), CP.
There is no PWRTE and no BOREN on the baseline die. The config-key audit
links each example's config TU per part, so the field spelling is
compiler-verified.

## Budget discipline

The 512-word, 25-byte 16F54 is the family's canonical and the gate
that proves every tier: host-sim blink, real XC8 build, the MPLAB SIM
toggle gate (`MODE=toggle`, PORTB bit 0) and the family-check scaffold.
Measured costs that shape the examples:

- A variadic `epic_harness_log` call with arguments overflows the
  stack budget (error 1360, no space for auto/param main) and XC8's
  printf machinery alone overflows flash; the family's blink reports
  through the fixed `epic_harness_report` marker path only.
- A 32-bit loop counter cannot share `main`'s frame with the Timer0
  handle: the host-bounded loop keeps `uint32_t i` inside `#ifdef
  PIC16F5X_HOST` and the target builds an infinite polled loop.
- The WDT refresh loop does not fit 512 words alongside the blink, so
  the example links with WDT = OFF (both target and sim variants).

## MPLAB SIM notes

mdb accepts `device PIC16F54; hwtool SIM`. `print TRISA` at POR returns
0x1F; `print OPTION` answers "Symbol does not exist" (OPTION is an
instruction, not a register, on 12-bit cores), so a gate asserts the
TRIS shadow/port reads, never an OPTION print. The stepi-advanced
toggle protocol samples PORTB bit 0 every 50000 instructions: the
blink's Timer0 toggles at a ~50000-instruction period (prescaler 256 x
count 256 at Fosc/4), so 50000 alternates cleanly while the 200000
default aliases to a constant phase (verified 2026-09-15; see
`scripts/ci-target-sim.sh`).
