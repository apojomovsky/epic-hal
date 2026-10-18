# PIC16F5x HAL

First family on the 12-bit baseline core (DS41213D): five parts across
two address shapes. The 16F54 canonical (18-pin, 512 words flash,
25 B GPR) is the tightest budget in this repo and proves every tier:
host-sim blink, real XC8 build, the MPLAB SIM toggle gate and the
family-check scaffold. The remaining parts bank their GPR through FSR
(16F57, 505/506 via FSR<6:5>; 16F59 via FSR<7:5>) and build the same
blink; capability macros per part select ports, bank geometry, OSCCAL
and the comparator/ADC surface.

## Peripheral tier

GPIO (PORTA/PORTB, PORTC on the 28/40-pin and 14-pin parts,
PORTD/PORTE on the 16F59), polled Timer0, WDT/Sleep, the 16F506
comparator pair plus voltage reference (`src/peripherals/
pic16f5x_comp.c`: CxON/CxPOL, VRCON tap/range/output/enable, output
readback; no IRQ exists so no callback API), the 16F506 ADC
(`src/peripherals/pic16f5x_adc.c`: ADCON0/ADRES, ANS cover plus CHS
select, start + poll + 8-bit read; VDD reference, and no IRQ so no
callback or flag API), and a no-op IRQ stub: no part in this family
has an interrupt vector (DS41213D section 4.0), so `EPIC_IRQ_*` and
the Timer0 overflow callback are contract parity only. The analog
block defaults to off for digital I/O: `EPIC_GPIO_Init` clears it so
its shared pins are digital (see `MANUAL.md`), and
`EPIC_COMP_DeInit`/`EPIC_ADC_DeInit` return it there. No USART/SSP
exists on any part, and 512 words cannot hold the shared
serial/tick modules.

## Build and test

Host sim: `cmake -B build && cmake --build build`, run any
`build/example_*` directly (`example_blink`, `example_control`, and the
per-device `example_blink_PIC16F505` .. `example_blink_PIC16F59`)
or `ctest` for the 16F506 `example_comp` and `example_adc` probes. The
verdict is the printed `EPIC_HARNESS_RESULT` marker, not the exit code.
Real target: `make xc8-build MODULE=hal/pic12/16f5x MCU=16F54` (blink;
`MCU=16F506` builds the comp probe from the target slot, while the
16F506 SIM run builds the ADC probe from the SIM slot).
MPLAB SIM toggle gate: `make mdb-test MODULE=hal/pic12/16f5x MCU=16F54
DEVICE=PIC16F54 MODE=toggle STEPI=50000` (PORTB bit 0; the family has
no UART. The blink's toggle period is ~50000 instructions, so STEPI
50000 alternates cleanly while the 200000 default aliases to a constant
phase, see `scripts/ci/ci-target-sim.sh`). All five parts take the same
parameters: swap `MCU=`/`DEVICE=` for `16F505`, `16F506`, `16F57` or
`16F59` and the gate runs unchanged. The 16F506 line runs the ADC
probe instead of the blink: same MODE=toggle watch, with the POR,
programmed and DeInit ADCON0 images plus the Start return gating
the toggle (see `MANUAL.md`).
Epic-cc gate: `make epiccc-build MODULE=hal/pic12/16f5x MCU=16F54
EPIC_CC_HOST=1` builds the family's epic-cc example, a software-loop
PORTB:0 toggle (`tests/example_blink_epiccc.c`, wired through the
manifest's `example.PIC16F5x.epiccc` variant): epic-cc's PicBaseline
simulator executes instructions only (never advances TMR0) and the die
has no interrupt to inject, so the Timer0-polled canonical blink gives
the sim-runner nothing to watch. The CI epiccc-gate job builds the hex
under the pinned epic-cc and gates the toggle through
`scripts/sim/sim-runner` with `--watch PORTB:0` and no `--irq-every`.

## XC8 codegen gotchas (live)

- TRIS/OPTION are control space: written through `__control` externs
  that XC8 lowers to the native `tris`/`option` instructions. No
  file-register address exists for either (a `print OPTION` in mdb
  answers "Symbol does not exist"), and TRIS has no read path, so the
  driver shadows direction in GPR.
- Budget: the 16F54 example must fit 512 words and 25 B at -O2. No
  variadic harness logging, no 32-bit frame locals on target, and the
  WDT refresh loop cannot ride along (example links WDT = OFF); see
  `MANUAL.md` for the measured shape.
- Banking: 16F57/505/506 page GPR through FSR<6:5>, the 16F59 through
  FSR<7:5> (DS41213D section 3.6); only the 16F54 exemplar is flat,
  so no runtime bank select exists on it. The 505/506's PORTB/PORTC
  are 6-bit (RB0..RB5, RC0..RC5) and the 16F59's PORTE is RE4..RE7.
  Struct bulk-init copies do not survive the banked parts: an 8-byte
  `= COMP_HANDLE_DEFAULT` copy lost byte 0 on the 16F506 (bank1
  auto), so the programmed CM1CON0 read 0x22 with C1ON missing and
  the gate stayed dark; assigning fields one by one (the
  `example_blink` shape) programs 0x2A and passes (verified
  2026-10-18). The out-of-line driver reads through FSR indirect
  are fine (pointers carry the bank); only the initializer copy
  was affected.
- 16F506 analog defaults: both comparators and the ADC's ANS<1:0>
  selects come out of reset enabled, holding RB0..RB2 and RC0..RC1
  analog so digital writes to them land nowhere (DS41268D §9.1.2,
  §7.7). `EPIC_GPIO_Init` clears CM1CON0/CM2CON0/ADCON0 before
  configuring a pin; the toggle gate is what caught it (PORTB read
  0x08 with RB0 stuck and TMR0 counting, CM1CON0/ADCON0 still at their
  POR values).
