# pic14-midrange-core

Shared peripheral and CPU-level drivers for the classic PIC14 mid-range
families: PIC16F87XA, PIC16F88X and PIC16F628A. The drivers are
family-blind inside that core class. What differs per part (register
addresses, bank placement, pin and peripheral availability) is selected
by the family's own SFR map plus the `PIC14MIDRANGE_HAS_*` feature
macros in `pic14_midrange.h`. Nothing here serves the enhanced mid-range
(PIC16F193X) or PIC18 parts.

## What lives here

- `include/core/`: the shared IRQ plumbing contract
  (`pic14_irq_common.h`) and the WDT/Sleep/BOR/POR helpers
  (`pic14_wdt_sleep.h`, real `clrwdt`/`sleep` on target, no-ops on
  host).
- `include/peripherals/` and `src/peripherals/`: the GPIO, Timer0/1/2,
  CCP, USART, comparator, VREF and EEPROM drivers, one `pic14_<ppp>`
  header/source pair each.
- `src/epiccc/`: the epic-cc build slices (ISR vector, dispatch tiers,
  WDT/sleep intrinsics) every classic mid-range family on the epic-cc
  path links.
- `src/sim/` and `src/target/`: the link-time-selected execution-model
  pairs, plus the target-only ISR vector (`epic-common/MANUAL.md`
  §2.2).

## epic-cc dispatch tiers

A family's `epiccc_sources` slice picks one dispatch tier from
`src/epiccc/`. The shared `pic16_irq_dispatch_epiccc.c` is the full
fan-out: its USART, SSP, ADC, PSP, comparator, EEPROM and CCP2/BCL
rows each gate on their own `PIC14MIDRANGE_HAS_*` flag, the same
shape as the XC8 twin `pic14_irq_dispatch.c`. Two requirements the
guards cannot hide: with `PIC14MIDRANGE_HAS_PIR1 1`, the die's PIR1
map must name the Timer1, Timer2 and CCP1 flags, and the slice must
link the TIMER0, RB and TIMER2 default handlers (`pic14_timer0.c`,
`pic14_gpio.c`, `pic14_timer2.c`). `PIC14MIDRANGE_HAS_PIR1 0` is a
supported path of its own (the PIR-less EEIF clear via EECON1).
Families whose PIR1 lacks those tokens link
`pic16_irq_dispatch_blink_epiccc.c` (Timer0 + RB change only) or a
family-specific tier instead.

## What lives in a family instead

A family directory holds everything that cannot be shared: the SFR map,
the host/target platform headers, the IRQn enum and vector table, the
sim backend, harnesses, tests and docs. PIC16F628A is the reference for
how thin that shim should be: every driver it has comes from here.

A peripheral driver stays in a family's own tree when it has a single
adopter or genuinely different hardware. Inside the classic mid-range
that is currently: the 88X GPIO (ANSEL/ANSELH analog selection, per-pin
PORTB interrupt-on-change), the 88X comparator pair, the 87XA PSP, and
the 88X OSC, SR-latch and ULPWU. The 87XA/88X SSP and ADC are the
opposite case, shared hardware with two adopters each, and carry their
own migration tickets (#139, #140) to move them here behind feature
macros.

## Non-goals

Deliberate asymmetries, settled in epic-hal#138, named so they stop
coming up:

- Renaming the PIC16F193X module (`epic-pic16f193x-firmware`) to the
  `<slug>-hal` convention.
- Recasing the PIC18 family key (`PIC18Fxx5x`) in the manifest.
- Renaming the 18F arch-named epiccc header.

The 193X and 18F family-named IRQ headers are not non-goals but are
deferred by the same ticket: rename them to the generic `pic16_irq.h`
shape only when a ticket touches that family anyway.
