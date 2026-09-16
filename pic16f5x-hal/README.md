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

GPIO (PORTA/PORTB, PORTC on the 28/40-pin and 20-pin parts,
PORTD/PORTE on the 16F59), polled Timer0, WDT/Sleep, and a no-op IRQ
stub: no part in this family has an interrupt vector (DS41213D section
4.0), so `EPIC_IRQ_*` and the Timer0 overflow callback are contract
parity only. No USART/SSP/ADC-comparator drivers yet: 16F506's
comparator/ADC surface is the follow-up batch, and 512 words cannot
hold the shared serial/tick modules.

## Build and test

Host sim: `cmake -B build && cmake --build build`, run any
`build/example_*` directly (`example_blink`, `example_control`).
Real target: `make xc8-build MODULE=pic16f5x-hal MCU=16F54` (blink).
MPLAB SIM toggle gate: `make mdb-test MODULE=pic16f5x-hal MCU=16F54
DEVICE=PIC16F54 MODE=toggle STEPI=50000` (PORTB bit 0; the family has
no UART. The blink's toggle period is ~50000 instructions, so STEPI
50000 alternates cleanly while the 200000 default aliases to a constant
phase, see `scripts/ci-target-sim.sh`).
Epic-cc gate: `make epiccc-build MODULE=pic16f5x-hal MCU=16F54
EPIC_CC_HOST=1` awaits epic-cc#437 (the p16f54 RAM model caps GPR at
9 bytes; the sim-runner PicBaseline arm is landed and fixture-verified
in the meantime).

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
