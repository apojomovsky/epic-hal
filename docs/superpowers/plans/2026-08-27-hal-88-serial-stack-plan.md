# HAL-3e implementation plan (ephemeral)

## Findings that shaped the design

1. My first `put_*` draft used end-pointer subtraction and a pointer phi;
   clang lowers both into value-position folded GEPs (`getelementptr
   inbounds nuw (...)` inside `phi ptr [...]` arms and `ptrtoint (ptr
   getelementptr ...)`) that irparse rejects outside its instruction-level
   gep dispatch. Index-based emission avoids the shapes entirely; bisect
   over probe_full.ll showed 81/81 pre-existing blocks parse fine and the
   failures start exactly at my functions. The gap itself gets filed
   upstream with the .ll repro; the cluster does not depend on it landing.
2. Stack-local arrays panic irparse (known class, mcp23x17 precedent):
   demo buffers are file/function-static.
3. Growing the per-family flat `epiccc_sources` slice with usart.c (a)
   pushes the whole-program overlay past the last 877A bank and (b) breaks
   previously-green epic-tick via a fresh whole-program interaction
   (`iselcore: no gep ... USART_TX_IRQHandler::4`). The flat model cannot
   absorb the serial stack; per-module HAL subsets are required instead.

## Design

- `epicmanifest.py`: optional module key `epiccc_hal_sources`
  (repo-root-relative). Present -> verbatim replacement for the family
  slice on epic-cc builds; absent -> today's behavior. Loader validates
  paths belong to the module's family dirs or epic-common.
- New per-family dispatch tiers under `src/epiccc/`, factored to share one
  body include so the flag-clear scaffolding stays identical:
  `pic16_irq_dispatch_serial_epiccc.c` (USART RX/TX),
  `pic16_irq_dispatch_serial_tick_epiccc.c` (+ TIMER2 for modbus),
  `pic16_irq_dispatch_swuart_epiccc.c` (+ TIMER1, CCP1, CCP2).
- Module sets (halt overrides):
  - epic-serial, epic-console: usart + core irq + vector +
    dispatch_serial (+ fmt state lives in the module).
  - epic-modbus: same + timer2 driver + dispatch_serial_tick. Its XC8
    exclusion on 877A/887 stays unless the lean set provably fits BOTH
    toolchains' rules; measured facts go to the PR either way.
  - epic-swuart: ccp (+timer1, gpio pins if referenced) + core irq + 
    vector + dispatch_swuart.
- Gates: bounded demo mains per module emitting the harness PASS marker
  through the real TX path, run under `sim-mdb-run.sh ... uart` against
  the epic-cc-built hex via the established copy-to-build-sim + skip-build
  flow; PORTB-toggle gate where the demo naturally toggles (swuart).
  XC8 hexes rebuilt for the four modules as the unchanged-path evidence.

## Explicit non-goals

- No compiler changes in this PR; the two irparse value-position classes
  are filed as an epic-cc issue with repro.
- No PIC18 box (ticket scopes PIC14 devices; PIC18 contingent on the
  backend ticket as stated on the parent).
