# PIC16F5x HAL

First family on the 12-bit baseline core (DS41213D): five parts across
two address shapes. The 16F54 canonical (18-pin, 512 words flash,
25 B GPR) is the tightest budget in this repo and proves every tier:
host-sim blink, real XC8 build, the MPLAB SIM toggle gate and the
family-check scaffold. The remaining parts (16F57/59 flat,
16F505/506 banked 20-pin) build the same blink; capability macros per
part select ports, OSCCAL and the comparator/ADC surface.

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
MPLAB SIM toggle gate: `TOGGLE_STEPI=50000 make mdb-test
MODULE=pic16f5x-hal MCU=16F54 DEVICE=PIC16F54 MODE=toggle` (PORTB bit
0; the family has no UART. The blink's toggle period is ~50000
instructions, so 50000 alternates cleanly while the runner's 200000
default aliases to a constant phase, see `scripts/ci-target-sim.sh`).
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
- Banked parts (505/506) page GPR through FSR<6:5>; the flat exemplar
  has no bank bits at all, so no runtime bank select exists on this
  core.
