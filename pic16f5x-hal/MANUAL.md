# PIC16F5x HAL manual

Register facts for the PIC16F5x family that are not shared conventions
(those live in `epic-common/MANUAL.md`) and not driver behavior (each
driver cites its own datasheet sections). Cited per part: DS41213D for
the 16F54/57/59, DS41236C for the 16F505 (shared with the PIC12F508/509)
and DS41268D for the 16F506 (shared with the PIC12F510), all
cross-checked against the DFP (`Microchip.PIC16Fxxx_DFP`,
`edc/PIC16Fnnn.PIC` and `xc8/pic/include/proc/pic16fnnn.h`).

## Family shape

Five parts on the 12-bit baseline core (`edc:arch="16c5x"`,
`instructionSetId="pic12c5xx"`), two address shapes. The 16F54 is a
flat-RAM die; the 16F57 (FSR<6:5>), 16F59 (FSR<7:5>) and the 20-pin
505/506 (FSR<6:5>) bank their GPR through FSR (DS41213D section 3.6,
Figures 3-4/3-5).

| part | pins | flash | GPR | banks | ports | extras |
|---|---|---|---|---|---|---|
| 16F54 | 18 | 512 W | 25 B | flat | A (RA0..3), B | - |
| 16F57 | 28 | 2048 W | 72 B | 4x FSR<6:5> | A, B, C | - |
| 16F59 | 40 | 2048 W | 134 B | 8x FSR<7:5> | A, B, C, D, E (RE4..7) | - |
| 16F505 | 14 | 1024 W | 72 B | 4x FSR<6:5> | B (6-bit), C (6-bit) | OSCCAL (file 0x05) |
| 16F506 | 14 | 1024 W | 67 B | 4x FSR<6:5> | B (6-bit), C (6-bit) | OSCCAL, 2 comparators, ADC |

GPR is 0x07..0x1F on the 16F54 (25 B, DS41213D section 1.0), the
tightest budget in this repo; every real-target example must fit both
512 words of flash and 25 B of RAM. The banked parts (16F57/59,
505/506) select their GPR window through FSR<6:5> or FSR<7:5>
(DS41213D section 3.6); only the 16F54 has no bank bits. The 505/506
have no PORTA: OSCCAL takes file 0x05 and the 14-pin die has no RA0
output pins (DS41236C/DS41268D Table 3-3 pinout tables). The 506's
GPR is 3 common bytes (0x0D..0x0F) plus 4x16 banked, not the 505's 8
common bytes, so it holds 67 B to the 505's 72 (DS41268D section 4.2,
Table 3-1).

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
  (the baseline asm pass resolves only literal operands). The CI
  epiccc-gate leg builds the family's epic-cc example, a software-loop
  PORTB:0 toggle rather than this family's Timer0-polled blink:
  epic-cc's PicBaseline sim executes instructions only and never
  advances TMR0, and the die has no interrupt to inject.

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
PA1/PA2 (2-bit FSR bank addressing, DS41236C/DS41268D sections 3.0/4.0).
The 16F506 adds the analog block below, files 0x08..0x0C
(DS41268D §4.2, Figure 4-3 and Table 4-2).

The sfr-map audit cross-checks every `PIC_REG_*` address and bit row
against the five DFP headers; OPTION's bits are DFP_MISSING_OK (control
space carries no `_POSN` macros) and are datasheet facts instead.

## 16F506 analog block, and why GPIO init clears it

Only the 16F506 has the analog files, and both comparators plus the ADC
analog selects come out of reset **enabled** (DS41268D Table 4-2, the
register-file summary's Power-on Reset column; the map itself is
Figure 4-3 in §4.2):

| file | register | POR | what it holds analog |
|---|---|---|---|
| 0x08 | CM1CON0 | 0xFF | C1ON=1: RB0 (AN0/C1IN+) and RB1 (AN1/C1IN-) |
| 0x09 | ADCON0 | 0xFC | ANS<1:0>=11: AN2/AN1/AN0 (RB2/RB1/RB0) |
| 0x0A | ADRES | - | ADC result |
| 0x0B | CM2CON0 | 0xFF | C2ON=1: RC0 (C2IN+) and RC1 (C2IN-) |
| 0x0C | VRCON | 0x3F | comparator reference (VREN=0 at POR) |

A pin held analog is not available for digital output, and the
ANS<1:0> selection stays in effect regardless of ADON (DS41268D
§9.1.2); a power-on reset forces the comparator input pins to analog
reset mode (§7.7). So on this part a digital write to RB0 lands
nowhere until the block is cleared: `EPIC_GPIO_Init` writes
CM1CON0/CM2CON0/ADCON0 to 0 before configuring a pin, and the MPLAB
SIM toggle gate is what exposed it (PORTB read 0x08 with RB0 stuck,
TMR0 counting, CM1CON0/ADCON0 still at their POR values; verified
2026-09-17). A comparator/ADC driver re-enables what it needs; the
digital-I/O default is analog off.

## Config word (DS41213D section 14.1)

`#pragma config` surface: OSC (LP/XT/HS/RC), WDT (ON/OFF, not WDTE), CP.
There is no PWRTE and no BOREN on the baseline die. The config-key audit
links each example's config TU per part, so the field spelling is
compiler-verified.

The 505/506 carry a wider config word (DS41236C section 7.1,
Register 7-2 for the 505; DS41268D section 10.1, Register 10-2 for the
506): 12 bits with MCLRE and FOSC<2:0>, so eight oscillator selections
including INTRC (internal 4 MHz), EXTRC and EC, and the datasheets name
the watchdog bit WDTE rather than WDT. The family's examples use
OSC=XT, WDT=OFF, CP=OFF on every part, which all five accept and the
config-key audit verifies per part.

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

mdb accepts `device PIC16F54; hwtool SIM`, and the four sibling part
names too (`PIC16F505`, `PIC16F506`, `PIC16F57`, `PIC16F59`, each
verified by its own toggle gate). `print TRISA` at POR returns
0x1F; `print OPTION` answers "Symbol does not exist" (OPTION is an
instruction, not a register, on 12-bit cores), so a gate asserts the
TRIS shadow/port reads, never an OPTION print. The stepi-advanced
toggle protocol samples PORTB bit 0 every 50000 instructions: the
blink's Timer0 toggles at a ~50000-instruction period (prescaler 256 x
count 256 at Fosc/4), so 50000 alternates cleanly while the 200000
default aliases to a constant phase (verified 2026-09-15; see
`scripts/ci-target-sim.sh`). The same parameters hold for the four
siblings (verified 2026-09-17), and the 16F506 is the part whose analog
POR defaults the gate caught (see the analog section above).
