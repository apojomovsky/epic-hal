# Plan: Remove the remaining `#ifndef EPIC_AT` callback guards and land the crates/sim e2e (epic-hal#105)

Closes epic-hal#105, which closes epic-hal#67. Two repos, two PRs.

## Background

epic-cc#137 (cross-context stored callbacks, PR #142) and epic-cc#138
(const table 256-byte window, PR #141) are merged on epic-cc master. The
compiler can now express what epic-hal#67 was waiting on: a callback
stored in Init/main and fired from the ISR via a global. The HAL side is
removing the guards; the e2e proof lands in epic-cc, the tree that owns
`crates/sim` and already vendors the hal-pic18 slice
(`crates/driver/tests/hal_pic18_slice_e2e.rs`, per #100's follow-up).

## epic-hal PR (this repo)

1. Delete every `#ifndef EPIC_AT` / `#else` / `#endif` callback fork in
   `pic16f87xa-hal/src/peripherals/`: adc, ccp (x2), comp, eeprom, gpio,
   psp, ssp, timer0, timer1, timer2, usart (x2). Keep the real callback
   body, delete the `(void)` branch. Also the `#ifdef __EPIC_CC__`
   cross-context fork in `pic16f88x-hal/src/peripherals/pic16f88x_ssp.c`.
   Same for `pic16f88x-hal`: gpio, timer0, and the
   `pic16f88x_usart_epiccc.c` variant (guards only; the file stays, its
   llvm.umin workaround is a separate filed gap).
2. Restore the single shared implementation in both `example_blink.c`
   files: delete the `__EPIC_CC__` forks (static handle, NULL callback,
   TMR0IF poll loop, report epilogue). One source again on both paths,
   callback-driven ISR blink.
3. Bump `EPIC_CC_PIN` in `.github/workflows/ci.yml` from 25a98424 to the
   epic-cc sha that includes #142 and still builds the 887 slice
   (verified locally before choosing; DEVELOPMENT.md documents the
   procedure). Update DEVELOPMENT.md if the pin rationale changes.

## epic-cc companion PR (other repo)

4. Vendor a `hal-pic16` slice under `crates/driver/tests/fixtures/`,
   following the `hal-pic18` pattern: the real 87xa peripheral sources
   (timer0, gpio, pic16_irq, vector, dispatch) adapted to compile under
   epic-cc, with a program that registers a Timer0 overflow callback in
   main and fires it from the ISR.
5. `crates/driver/tests/hal_pic16_slice_e2e.rs`: run the vendored slice
   on the Pic14 sim for p16f877a (and the PIC18 analog for p18f4550 per
   epic-cc#137 acceptance), asserting the callback fires end to end
   (cross-context: stored in Init/main, invoked by the ISR) and an
   `irq_table` field reads non-zero (the epic-cc#114 zero-blob
   regression guard).

## Verification

- `make epiccc-build MODULE=pic16f88x-hal MCU=16F887` and
  `MODULE=pic16f87xa-hal MCU=16F877A` (with an epic-cc driver built from
  master, #142 included).
- `make mdb-epiccc MODULE=pic16f88x-hal MCU=16F887 DEVICE=PIC16F887`
  (toggle gate + register read, the "not just compiled" acceptance).
- Host-sim ctest for both families.
- epic-cc: `make test CRATE=driver` and the sim crate suite; e2e green
  for both devices.
- Takeoff ritual in both repos, PRs, reviewer pass.
