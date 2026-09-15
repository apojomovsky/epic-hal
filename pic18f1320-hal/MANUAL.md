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
(see section 12).

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

## 5. Timers 0-3

All four timers share the `pic18fxx5x-hal` register shape and drivers,
verified 1-to-1 against the 1320's own DFP header (Microchip.PIC18Fxxxx_DFP
1.7.171): identical Access Bank addresses and bit layouts. Register
sections cited from the 1320's own datasheet, DS39605F, not the 4550's.

## 6. Timer0

*DS39605F §11.0, Register 11-1 (T0CON 0xFD5).*

8/16-bit timer/counter with its own prescaler, controlled by T0CON;
default 8-bit to stay a PIC16 drop-in. Overflow -> TMR0IF + weak
`TIMER0_IRQHandler`. `example_blink` is the canonical Timer0 + GPIO +
interrupt smoke; `example_timer0_irq` is the IRQ backend's dedicated
smoke (`INTCON<TMR0IE/TMR0IF>`, `T0CON<TMR0ON>` readback).

## 7. Timer1

*DS39605F §12.0, Register 12-1 (T1CON 0xFCD, identical to 4550).*

16-bit timer/counter. T1CON adds RD16 (bit 7, set by this driver for
atomic 16-bit access) and read-only T1RUN (ignored). Prescaler 1:1/2/4/8.
Overflow sets PIR1<TMR1IF>. `example_timer1`: internal/1:1, 2 overflows
per 150k cycles on host; real-`mdb` gate proves `TMR1L` counts with
`T1CON<RD16|TMR1ON>` and `PIR1` cleared (ISR fired) on hardware.

## 8. Timer2

*DS39605F §13.0, Register 13-1 (T2CON 0xFCA, identical to 4550).*

8-bit timer with PR2 period + postscaler (1:1..1:16). Match (TMR2==PR2)
resets TMR2 and sets PIR1<TMR2IF> after the postscaler. Drives CCP PWM.
`example_timer2`: PR2=9, 10 matches per 100 cycles on host; real-`mdb`
gate proves `TMR2` counts, `PR2` reads 9, `T2CON<TMR2ON>` = 0x04 and
`PIR1<TMR2IF>` set on match.

## 9. Timer3

*DS39605F §14.0, Register 14-1 (T3CON 0xFB1, identical to 4550).*

Second 16-bit timer alongside Timer1. Shares T1OSC (no oscillator field).
T3CCP1 (bit 3) selects Timer1 vs Timer3 for CCP (reset default Timer1;
the CCP driver leaves it). Overflow sets PIR2<TMR3IF>. `example_timer3`:
internal/1:1, 2 overflows per 150k cycles; real-`mdb` gate proves
`TMR3L` counts with `T3CON<RD16|TMR3ON>` and `PIR2` cleared (ISR fired).

## 10. ECCP1, USART, ADC, Data EEPROM

Land in epic-hal#180 (ECCP1, USART) and #181 (ADC, Data EEPROM), each
through the full `mdb` verification gate before this section gets filled
in.

## 11. Known gaps and gotchas

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

## 12. The examples

- `tests/example_smoke.c`: bare harness-seam test, no GPIO/Timer/IRQ.
- `tests/example_blink.c`: Timer0 + GPIO + interrupt canonical smoke,
  the real-target build (`.example.PIC18F1320`).
- `tests/example_timer0_irq.c`: the IRQ backend's dedicated smoke test
  (`.example.PIC18F1320.sim`), enables one Timer0 overflow interrupt and
  asserts the flag/enable bits (`INTCON<TMR0IE/TMR0IF>`, `T0CON<TMR0ON>`)
  read back correctly on the host build; verified on real hardware via
  `MODE=toggle` (see section 11).
- `tests/example_timer1.c`: Timer1 overflow smoke (host), `mdb`-verified.
- `tests/example_timer2.c`: Timer2 PR2-match smoke (host), `mdb`-verified.
- `tests/example_timer3.c`: Timer3 overflow smoke (host), `mdb`-verified.
