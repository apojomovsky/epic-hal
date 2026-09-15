# PIC18F1320 HAL, Manual

Family-agnostic conventions, the handle pattern, status codes, the harness,
and the host-sim/target build-time split: see `epic-common/MANUAL.md`. This
manual covers only what is genuinely specific to **PIC18F1320**, verified
against the Microchip datasheet **DS39605F** (PIC18F1220/1320 Data Sheet).

Status: **skeleton**, foundation phase only (epic-hal#178). Filled out fully
in epic-hal#181 (family B phase 4/4), the same way pic18fxx5x-hal/MANUAL.md
and pic16f87xa-hal/MANUAL.md were.

---

## 1. What this is

The smallest of epic-hal's three new PIC18 families (family B, umbrella
#150): an 18-pin part, 4096 words flash, 240 B RAM (0x10-0xFF, a single
bank), forked from `pic18fxx5x-hal` because it shares the same Access
Bank addressing shape and the same two-vector (high/low priority)
interrupt architecture. Confirmed directly against the DFP header and
`p18f1320.toml` in epic-cc, not assumed from the family name: this part
has no USB, no MSSP, a single ECCP1 (no CCP2), and no comparator.

## 2. Interrupts

Two-vector priority scheme (0008h high, 0018h low), `EPIC_IRQ_*` API
identical in shape to `pic18fxx5x-hal`. Sources confirmed present on this
part: INT0/INT1/INT2, RB<7:4> change, Timer0-3, ECCP1, USART TX/RX, ADC,
EEPROM write-complete. No SSP, CCP2, comparator or SPP sources: those
peripherals do not exist on this part (confirmed absent from the DFP
header, not carried over from `pic18fxx5x-hal`'s IRQn enum).

The IRQ backend's own dedicated `mdb` smoke test is
`tests/example_timer0_irq.c`, verified under real `mdb` via `MODE=toggle`
(see section 22).

## 3. Core: WDT, Sleep, BOR/POR

Same shape as `pic18fxx5x-hal`: `EPIC_WDT_Refresh`/`EPIC_Sleep_Enter` are
link-time-selected (`*_sim.c` host, `*_target.c` XC8); BOR/POR status
reads/clears RCON directly. One real difference: RCON bit 6 (SBOREN on
the sibling family's 4550/2455) is unimplemented on this part, confirmed
absent from the DFP header's own bitfield macros.

## 4. GPIO

Only two ports exist: PORTA and PORTB, both full 8-bit (RA0-7, RB0-7); no
PORTC/D/E (an 18-pin package has no room for them, confirmed absent from
the DFP header). Otherwise identical to `pic18fxx5x-hal`'s GPIO driver:
`EPIC_GPIO_*` writes go through LATx, reads come from PORTx, PORTB pull-
ups live in INTCON2<RBPU>.

## 5. Timer0

Identical register shape and driver to `pic18fxx5x-hal`'s Timer0 (T0CON,
TMR0L, TMR0H at the same Access Bank addresses).

## 6. Timers 1-3, ECCP1, USART, ADC, Data EEPROM

Land in epic-hal#179 (Timer1-3), #180 (ECCP1, USART) and #181 (ADC, Data
EEPROM), each through the full `mdb` verification gate before this
section gets filled in.

## 7. Known gaps and gotchas

- **No comparator, no MSSP, no CCP2, no SPP, no USB**: confirmed absent
  from the PIC18Fxxxx DFP header for `pic18f1320`/`pic18lf1320`, not
  assumed from pin count.
- **`#pragma config` field set is smaller than `pic18fxx5x-hal`'s**: no
  `CPUDIV`/`PLLDIV`/`USBDIV`/`VREGEN`/`CCP2MX`/`PBADEN`/`LPT1OSC`/`EBTR2`/
  `CP2`/`WRT2` fields (no USB, no CCP2, fewer flash-protection blocks
  given the smaller 4096-word flash); `STVR` replaces `STVREN`. Confirmed
  against the DFP's `18f1320.cfgmap`.
- **The real-target `mdb` gate watches `LATB`, not `PORTB`**: this part's
  simulated `PORTB` (mdb `print PORTB`) does not mirror the `LATB` output
  latch for an output-configured pin under MPLAB SIM, confirmed
  empirically (LATB visibly toggled across `stepi` samples while PORTB
  read a constant 0 across the same run). `scripts/ci-target-sim.sh`
  exports `TOGGLE_REG=LATB` for this family's gate for that reason.

## 8. The examples

- `tests/example_smoke.c`: bare harness-seam test, no GPIO/Timer0/IRQ.
- `tests/example_blink.c`: Timer0 + GPIO + interrupt canonical smoke,
  the real-target build (`.example.PIC18F1320`).
- `tests/example_timer0_irq.c`: the IRQ backend's dedicated smoke test
  (`.example.PIC18F1320.sim`), enables one Timer0 overflow interrupt and
  asserts the flag/enable bits (`INTCON<TMR0IE/TMR0IF>`, `T0CON<TMR0ON>`)
  read back correctly on the host build; verified on real hardware via
  `MODE=toggle` (see section 7).
