# Multi-family PIC HAL: refactor plan

Status: **approved; Phases 0-4 done, litmus test met (real-silicon deferred)**.
This document is the plan agreed for generalizing the PIC16F87XA HAL so it
can support additional 8-bit PIC families (starting with
PIC18F2455/2550/4455/4550, DS39632E) without rewriting the whole tree per
chip. Phase 4 (broaden PIC18 peripheral coverage) is complete — every
peripheral on the part is ported.

## Motivation

The HAL and task manager currently assume exactly one architecture:
PIC16F87XA's single interrupt vector, STATUS-bank register addressing, and
register set. A second family was requested (PIC18F2455/2550/4455/4550, datasheet
[DS39632E](https://ww1.microchip.com/downloads/en/DeviceDoc/39632e.pdf)),
and a third and fourth are expected to follow. The
goal is a structure where adding a new family is bounded, mostly-mechanical
work: implement a fixed contract once per chip, not re-derive the whole
harness/build/status-code machinery every time.

## Decision: Option A (shared core, per-family backend)

Considered and rejected: a fully parallel HAL tree per family (no shared
code beyond copy-paste of the pattern). Rejected because the stated goal is
minimizing the diff for chip #3 and #4, and a parallel-tree approach doesn't
shrink that diff, each new family re-derives the same harness contract,
build boilerplate, and status-code enum from scratch.

Chosen instead: extract the parts of the current HAL that are genuinely
architecture-blind into a shared layer (`pic8-common/`), and keep everything
that is genuinely register-specific (SFR maps, bank/BSR addressing, IRQ
vector layout, peripheral register semantics) in per-family trees that all
implement the same contract. See the design writeup in conversation for the
full comparison table; the short version:

**Shared across every family** (extract once, reuse forever):
- Status codes and bit macros (`HAL_StatusTypeDef`, `PIC8_BIT*`)
- The host/target harness contract (`init/tick/running/log`) and its
  target-side no-op implementation (identical on every family, it's four
  functions that do nothing because real silicon needs no harness)
- `HAL_PPP_Init/DeInit/MspInit` naming convention and the weak-ISR-override
  pattern
- Build-system boilerplate (CMake helper functions, Makefile pattern rules)

**Per-family, implementing a fixed contract**:
- SFR address map and bank/BSR addressing scheme
- `platform.h` (how an SFR read/write is spelled, in each of host/target ×
  family)
- IRQ vector layout and priority model (PIC16: one vector, no priority;
  PIC18: two vectors, per-source priority bit)
- Peripheral driver register-level implementation (`HAL_GPIO_*`,
  `HAL_TIMER0_*`, etc. keep the same *names* and *signatures*; the bodies
  differ because the registers differ)
- Config Word generation (PIC16 has one config word; PIC18 has several,
  unrelated fields)

## Target tree layout

```
pic8-common/                         # NEW — thin, stable, rarely touched after Phase 0
  include/core/hal_status.h          # HAL_StatusTypeDef, PIC8_BIT*
  include/core/pic8_harness.h        # the 4-function host/target contract
  src/core/pic8_harness_target.c     # the one shared no-op file
  cmake/pic8_family.cmake            # shared CMake helpers (add_example, etc.)
  mk/pic8_family.mk                  # shared Makefile include (pattern rules, VPATH)

pic16f87xa-hal/                      # EXISTING tree, restructured in place
  include/{host,target}/...          # unchanged pattern; defines PIC8_REG8 etc.
  include/core/pic16_irq.h           # PIC16 IRQn enum + backend (renamed from
                                      # pic16f87xa_interrupt.h's family-specific half)
  include/peripherals/...            # HAL_GPIO_*, HAL_TIMER0_* — names unchanged
  src/...

pic18fxx5x-hal/                      # NEW — same skeleton as pic16f87xa-hal
  include/{host,target}/...          # PIC18 BSR/Access-Bank addressing, LATx-aware GPIO
  include/core/pic18_irq.h           # PIC18 IRQn enum, 2-vector + priority backend
  include/peripherals/...            # same HAL_GPIO_*/HAL_TIMER0_* names, PIC18-shaped bodies
  src/...
  mcu/pic18fxx5x-mplabx/Makefile     # includes mk/pic8_family.mk

pic8-taskmgr/                  # UNCHANGED in scope — must become provably
                                      # family-agnostic (see Phase 3 validation)
```

## Litmus test for the whole effort

After Phase 3, the task manager's build can point at `pic18fxx5x-hal`
instead of `pic16f87xa-hal` with **zero changes to `task_manager.c` or
`task_manager.h`**, and `example_multi_blink` passes on the PIC18 host sim
and on real PIC18 silicon. If that requires touching the task manager, the
contract in `pic8-common/` isn't right yet and Phase 0-2 need revisiting
before peripheral coverage grows further.

## Phases

Each phase has an explicit validation step. A phase is not done until its
validation passes; don't start the next phase on a red validation.

---

### Phase 0 — Extract the shared layer, rename in place (PIC16 only)

No new hardware support in this phase. Pure, behavior-preserving refactor
of the existing PIC16F87XA HAL. **Done** (commit `3f33d48`).

**Tasks**
1. Create `pic8-common/` with `hal_status.h` (`HAL_StatusTypeDef`,
   `HAL_OK/ERROR/BUSY/TIMEOUT/INVALID`, `PIC8_BIT/BIT_SET/BIT_CLR/BIT_TGL/
   BIT_READ`) and `pic8_harness.h` (the existing 4-function contract, moved
   and renamed from `pic16f87xa_harness_*` to `pic8_harness_*`).
2. Move `pic16f87xa_harness_target.c` to `pic8-common/src/core/
   pic8_harness_target.c` unchanged in substance (it's already
   family-blind: four no-ops).
3. Rename in `pic16f87xa-hal`:
   `PIC16F87XA_StatusTypeDef`/`_OK`/... → use the shared `HAL_*` from
   `pic8-common`; `PIC16F87XA_BIT*` → `PIC8_BIT*`; `PIC16F87XA_IRQ_Disable/
   Restore/Enable/DisableSrc/ClearFlag/GetFlag` → `HAL_IRQ_*` (still taking
   a PIC16-defined `IRQn` enum); `PIC16F87XA_WEAK` → `PIC8_WEAK`;
   `PIC16F87XA_REG8`/`_SFR_PTR` → `PIC8_REG8`/`PIC8_SFR_PTR`.
4. Extract shared CMake helper functions (e.g. the `pic16f87xa_add_example`
   pattern) into `pic8-common/cmake/pic8_family.cmake`, parameterized so
   `pic16f87xa-hal/CMakeLists.txt` becomes a thin caller.
5. Extract shared Makefile pattern rules (the `.c` → `.p1` rule, VPATH
   setup, the `.hex` link step) into `pic8-common/mk/pic8_family.mk`, and
   make `pic16f87xa-hal/mcu/pic16f87xa-mplabx/Makefile` include it.
6. Update `pic8-taskmgr` to link against the renamed symbols
   (`HAL_IRQ_Disable/Restore` instead of `PIC16F87XA_IRQ_Disable/Restore`,
   etc.).
7. Update docs (`pic16f87xa-hal/README.md`, `MANUAL.md`,
   `pic8-taskmgr/README.md`, `docs/API.md`, `docs/ARCHITECTURE.md`,
   root `README.md`) for the renamed public API surface.

**Explicitly out of scope for Phase 0**: no PIC18 code, no new directory
for a second family, no behavior change of any kind. If a test's *output*
changes, that's a bug in the refactor, not an intended improvement.

**Validation**
- [x] `pic16f87xa-hal` host-sim build (`cmake --build`) succeeds with zero
      warnings introduced.
- [x] Every existing HAL example (`example_blink`, `example_idle_blink`,
      `example_timer1/2`, `example_ccp_pwm`, `example_usart`, `example_ssp`,
      `example_adc`, `example_comp_vref`, `example_eeprom`, `example_psp`,
      `example_wdt_sleep`) passes with **identical** stdout/exit-code to
      before the refactor. Diff the captured output pre- and post-refactor,
      don't just check exit codes.
- [x] `pic8-taskmgr` host-sim build succeeds; `example_multi_blink`
      produces identical output to the pre-refactor baseline (`fast=12
      med=6 slow=3 blips=1 (ticks=61, tasks=4)`).
- [x] XC8 real-target build succeeds for all four devices (873A/874A/876A/
      877A) with no new warnings; `.hex` output size is unchanged (a size
      change would indicate the rename affected codegen, which it must
      not).
- [x] `grep -r PIC16F87XA_BIT\|PIC16F87XA_StatusTypeDef\|PIC16F87XA_IRQ_`
      the tree, expect zero hits outside of comments/docs explaining the
      old name for migration context (if any are kept).
- [x] Root README and both component READMEs build/link-check clean (no
      broken relative links after any file moves).

**Exit criterion**: all boxes checked, changes committed. This is the
foundation every later phase depends on; do not proceed to Phase 1 with a
partially-done rename.

---

### Phase 1 — Scaffold `pic18fxx5x-hal/` (empty backend)

Stand up the skeleton so the family-selection seam can be exercised before
any real PIC18 register code exists. **Done.**

**Tasks**
1. Create `pic18fxx5x-hal/` mirroring `pic16f87xa-hal/`'s directory shape:
   `include/{host,target}/`, `include/core/`, `include/peripherals/`,
   `src/{core,peripherals,sim}/`, `tests/`, `mcu/pic18fxx5x-mplabx/`.
2. Device-select header (`pic18fxx5x.h` analog to `pic16f87xa.h`): defines
   exactly one of `PIC18F2455/2550/4455/4550`, sets family capability
   macros (flash size, RAM, ADC channel count, has-SPP, etc., mirroring
   `PIC16F87XA_FAMILY_HAS_*`).
3. `include/host/pic18_platform.h` and `include/target/pic18_platform.h`
   stubs: host version backed by a memory array (size TBD once the BSR
   model is mapped in Phase 2), target version a placeholder volatile
   dereference. Both can be minimal/incomplete at this stage, they exist so
   the build wires up, not so registers work yet.
4. `CMakeLists.txt` and `mcu/pic18fxx5x-mplabx/Makefile`, both including
   `pic8-common`'s shared CMake/Makefile fragments from Phase 0.
5. One trivial smoke source (`tests/example_smoke.c`) that does nothing but
   call `pic8_harness_init/tick/running/log/report`, to prove the harness
   contract links against an empty family backend.

**Explicitly out of scope**: no real SFR addresses, no working GPIO or
Timer0 yet. This phase proves the *build* seam, not the *hardware*.

**Validation**
- [x] `cmake -B build -S pic18fxx5x-hal && cmake --build build` succeeds
      and produces `example_smoke`. (Built into `pic18fxx5x-hal/build` to
      avoid colliding with the taskmgr build at the repo-root `build/`;
      `-Wall -Wextra -Werror` clean.)
- [x] `./build/example_smoke` runs and reports pass via the shared harness
      (proves `pic8_harness_*` really is family-blind, since PIC18's
      backend links against the exact same header/contract PIC16 uses).
      Passes on all four devices, each printing its own part name.
- [x] XC8 build of the skeleton (empty peripheral set) produces a `.hex`
      for at least one device (e.g. PIC18F4550) with no link errors, proving
      the Makefile fragment + config-word generation plumbing works end to
      end even before real drivers exist. (Built a `.hex` for all four
      devices: 18F2455/2550/4455/4550.) Required installing the PIC18Fxxxx
      DFP, which is not bundled with XC8; see the Phase 1 note below.
- [x] Confirm the XC8 v3.10 PIC18 interrupt syntax to use in Phase 2
      (`#pragma interrupt` vs `__interrupt(high_priority)` vs something
      else) against the actual XC8 compiler documentation, not assumption.
      Record the answer in this document before Phase 2 starts. **Answer
      recorded below** (§"XC8 v3.10 PIC18 interrupt syntax — resolved").

**Exit criterion**: an empty but buildable-on-both-targets PIC18 tree
exists, and the interrupt-syntax open question is resolved and recorded.

---

### Phase 2 — Port the MVP vertical slice from DS39632E

The smallest slice that lets the task manager run: GPIO, Timer0, interrupts
(two vectors + priority), WDT/sleep. Chosen because it's exactly the task
manager's entire HAL dependency surface (confirmed by grepping
`pic8-taskmgr/src/task_manager.c` and the example), so finishing it
is both useful on its own and sets up Phase 3 directly. **Done.**

**Tasks**, each cited against DS39632E the way the PIC16 drivers cite
DS39582B:
1. SFR map subset: `STATUS`, `BSR`, `PORTB`/`LATB`/`TRISB`,
   `INTCON`/`INTCON2`/`INTCON3`, `PIR1`/`PIE1`/`IPR1`, `TMR0L`/`TMR0H`/
   `T0CON`.
2. `platform.h` (host + target): BSR-aware access replacing PIC16's
   `pic_select_bank()` scheme. Confirm whether the Access Bank shortcut is
   worth modeling in the host sim or whether a flat addressable array
   (sized for the banks actually implemented, per Figure 5-5) is simpler
   and sufficient for now.
3. GPIO driver: **write through LATx, not PORTx.** This is a real
   correctness improvement PIC18 provides natively (DS39632E §10.0), not a
   workaround, unlike PIC16 where the driver conventionally treats PORTx
   writes as read-modify-write of the latch, PIC18 exposes the latch as its
   own mapped register.
4. Timer0 driver: PIC18's T0CON adds an 8/16-bit mode bit PIC16 doesn't
   have; decide the default and document the citation.
5. Interrupt core: two vectors (0008h high-priority, 0018h low-priority),
   `IPEN` (RCON<7>) enabling the priority scheme, per-source priority bits
   in `INTCON2`/`INTCON3`/`IPR1`/`IPR2`. Design the `HAL_IRQ_*` contract
   extension needed to express priority without breaking PIC16 callers,
   options: an optional priority parameter PIC16's implementation ignores,
   or a separate `HAL_IRQ_SetPriority()` that's a no-op on PIC16. Pick one,
   document why, in this file, before implementing.
6. ISR vector entry file(s): using the syntax confirmed in Phase 1, one
   entry point per vector, both delegating to the shared dispatch pattern
   already used by PIC16 (`pic8_dispatch_all_irqs` per vector, or per
   priority level, decide during implementation and record the choice).
7. WDT/Sleep driver: port `HAL_WDT_Refresh`/`HAL_Sleep_Enter`/BOR/POR
   status against PIC18's equivalent registers (RCON's `TO`/`PD`/`BOR`/`POR`
   are laid out differently than PIC16's `PCON`, confirm exact bits against
   DS39632E before porting).
8. Host sim backend (`pic18_sim.c`): Timer0 stepping + GPIO drive/read
   hooks, mirroring `pic16f87xa_sim.c`'s approach and public API shape
   (`pic8_sim_reset/step/drive_input/read_output/set_irq_callback`, moved
   to the shared naming from Phase 0 where the API is actually
   family-blind).
9. Example: a PIC18 `example_blink` analog exercising Timer0 + GPIO +
   interrupts, buildable on host sim and XC8.

**Validation**
- [x] Host-sim `example_blink` (PIC18) toggles the target pin and reports
      pass, mirroring the PIC16 example's structure and pass criteria.
      (`RB0 toggled 9 times`, exit 0, on all four devices; 600k cycles /
      256×256 ≈ 9 overflows as expected. `-Wall -Wextra -Werror` clean.)
- [x] XC8 build produces a `.hex` for at least PIC18F4550 (40-pin, matches
      the family's most-featured part) with the interrupt vectors correctly
      placed at 0008h/0018h (verify via the `.hex`/`.lst`/map output, not
      just "it compiled"). (Built a `.hex` for all four devices; the
      symbol map places `_PIC18_IRQ_HandlerHigh` at 0x0008 and
      `_PIC18_IRQ_HandlerLow` at 0x0018, both calling
      `pic8_dispatch_all_irqs`.)
- [ ] On real PIC18 hardware: **deferred — no PIC18 board on hand.** The
      Timer0 + interrupt + GPIO chain is verified on the host sim and the
      XC8 build places the vectors correctly; real-silicon blink
      confirmation is flagged as deferred per the open-questions rule, not
      silently skipped.
- [x] `HAL_WDT_Refresh` / `HAL_Sleep_Enter` compile and, on host sim, behave
      as documented no-ops exactly like the PIC16 versions. (Host impls are
      empty no-ops; `example_blink` calls `HAL_WDT_Refresh` every loop
      iteration without crashing; both compile on XC8 as `clrwdt`/`sleep`.)
- [x] Re-run all of Phase 0's PIC16 validation checks, this phase must not
      have touched anything under `pic16f87xa-hal/` or `pic8-common/` in a
      way that regresses PIC16. (Touched `pic8-common/` to add
      `core/pic8_irq.h`, and `pic16f87xa-hal` to add the no-op
      `HAL_IRQ_SetPriority` — both additive. PIC16 taskmgr host baseline
      unchanged `fast=12 med=6 slow=3 blips=1 ticks=61 tasks=4`; PIC16 XC8
      `.hex` program space unchanged at 1783 words, the uncalled no-op
      dropped by the linker. One additional `(520) function never called`
      warning for the no-op, same class as the existing unused-HAL-function
      warnings, not a behavioral regression.)

**Exit criterion**: a working PIC18 blink example on host sim, with the
XC8 build producing correct vector placement, and zero PIC16 regression.

---

### Phase 3 — Point the task manager at PIC18 (the litmus test) — **done**

**Tasks**
1. Add a `pic8-taskmgr` build variant (or a build-time switch) that
   links against `pic18fxx5x-hal` instead of `pic16f87xa-hal`.
2. Do **not** modify `task_manager.c`/`task_manager.h` to make this work.
   If a modification seems necessary, stop and treat it as a Phase 0-2 gap:
   the shared contract is missing something, go fix the contract, not the
   consumer.
3. Adjust only `example_multi_blink.c`'s device-specific bits if any exist
   (there shouldn't be many, it was already written against PORTB generically
   for HAL portability across PIC16 family members).

**How it was done (contract fixes, not consumer logic edits):**
- *Family-neutral includes (the Phase 0-2 gap).* `task_manager.c`/`.h`
  included PIC16-family headers by name (`core/pic16_irq.h`,
  `peripherals/pic16f87xa_timer0.h`, `core/pic16f87xa_wdt_sleep.h`). The
  contract fix: each family now provides family-neutral public headers
  under those names — `core/hal_irq.h`, `peripherals/hal_timer0.h`,
  `core/hal_wdt_sleep.h`, and a neutral top `pic8_hal.h` — that pull in
  that family's specific header. The task manager's 3 include lines switch
  to the neutral names; `example_multi_blink.c` switches to `pic8_hal.h`.
  The build's include path (which family's HAL is added) decides which
  family resolves. `task_manager.c`/`.h` diff vs Phase 2 is exactly those
  3 include lines — zero scheduler-logic change.
- *Build variant.* `pic8-taskmgr/CMakeLists.txt` gained a
  `-DHAL_FAMILY=PIC18` switch (default PIC16) that points `HAL_DIR` at
  `pic18fxx5x-hal` and swaps the device list. A separate
  `mcu/pic18fxx5x-taskmgr-mplabx/Makefile` does the same for XC8.
- *Timer0 handle ownership (a latent bug the litmus test exposed).* The
  Timer0 driver stored a pointer to the caller's (stack-local) handle;
  the ISR read it back after the caller returned, a dangling pointer.
  PIC16 tolerated this by luck; PIC18's stack layout reused the slot and
  the ISR called a garbage function pointer → infinite recursion → stack
  overflow. Fix (HAL driver, not the consumer): the driver now *copies*
  the handle into owned static storage in both families. PIC16's baseline
  output is unchanged by the copy.

**Validation**
- [x] `example_multi_blink` builds against `pic18fxx5x-hal` on host sim
      with zero changes to `task_manager.c`/`task_manager.h` logic. (Diff
      vs Phase 2 is exactly the 3 include-line neutralizations above; the
      scheduler body is byte-identical.)
- [x] Host-sim run produces the same *shape* of output (four distinct
      blink rates, one spawned one-shot, slot reuse confirmed via
      `tasks=N` not growing) as the PIC16 run. (PIC18: `fast=16 med=8
      slow=4 blips=1 ticks=80 tasks=5`, exit 0, all four devices; PIC16:
      `fast=12 med=6 slow=3 blips=1 ticks=61 tasks=4`. Tick counts and the
      `tasks` snapshot differ because the PIC18 run reaches a second
      supervisor period (t=80 vs 61), leaving a freshly-spawned blip
      pending at the end — a timing artifact the plan allows, not a
      slot-reuse failure: `blips=1` ran and freed, slots did not exhaust.)
- [x] XC8 build succeeds and produces a `.hex` for the PIC18 target. (All
      four devices: 18F2455/2550/4455/4550; vectors at 0008h/0018h.)
- [ ] On real PIC18 hardware: **deferred — no PIC18 board on hand.** The
      Timer0 + interrupt + GPIO + scheduler chain is verified on the host
      sim and the XC8 build places the vectors correctly; real-silicon
      four-LED confirmation is flagged as deferred, not silently skipped.

**Exit criterion**: the litmus test in this document's introduction passes.
This is the point where "the refactor worked" becomes demonstrated, not
just designed. **Met** (modulo the deferred real-silicon check): the task
manager targets PIC18 with no scheduler-logic change, only the 3-line
include neutralization the contract fix required.

---

### Phase 4+ — Broaden PIC18 peripheral coverage

Not blocking for the multi-family architecture claim, this is ordinary HAL
growth at the established one-peripheral-at-a-time pace.

**Done so far:**
- **Timer1** (done): 16-bit timer/counter, same API as PIC16.
  PIC18 T1CON adds `RD16` (16-bit read/write mode, set by the driver) and
  the read-only `T1RUN` status bit; the rest of T1CON matches PIC16.
  Host-sim stepping + `example_timer1` (3 overflows at 1:1, 65536 cycles
  each) + XC8 all four devices.
- **Timer2** (done): 8-bit timer with PR2 + postscaler, same API
  as PIC16; simpler because PIC18's PR2 is in the Access Bank (no bank
  switching). Host-sim stepping + `example_timer2` (5 overflows, period
  250 cycles with PR2=249) + XC8 all four devices.
- **Timer3** (done): 16-bit timer/counter, mirrors Timer1's API;
  shares Timer1's T1OSC (no T3OSCEN), RD16 set, leaves T3CCP2:T3CCP1 at
  reset (CCP timer-select bits, managed by the future CCP/ECCP driver).
  Overflow -> PIR2<TMR3IF> (added PIR2/PIE2/IPR2 to the SFR map + the
  PIC18_IRQ_TMR3 source). Host-sim stepping + `example_timer3` (3
  overflows at 1:8, 524288 cycles each) + XC8 all four devices. The
  PIC18F2455 family's full timer set (Timer0-3) is now covered.
- **ECCP1 + CCP2** (done): the first genuinely PIC18-richer
  peripheral. Mirrors PIC16's `HAL_CCP_*` API (capture/compare/PWM, weak
  CCP1/CCP2 ISRs) and adds the Enhanced CCP features PIC16's plain CCP
  lacks: multi-output PWM (single / half-bridge / full-bridge
  forward/reverse via P1M), programmable dead-band delay + auto-restart
  (ECCP1DEL), and auto-shutdown with source select (FLT0 / comparators)
  + per-pair pin states (ECCP1AS). PIC18 also adds a Compare "toggle on
  match" mode. CCP2 is the plain CCP. Added PIC18_IRQ_CCP2 (PIR2<CCP2IF>).
  SFRs: CCP1CON/CCPR1L/H, CCP2CON/CCPR2L/H, ECCP1DEL, ECCP1AS. The host
  sim models the SFR image (the example verifies CCP1CON/CCPR1L/ECCP1DEL
  + counts Timer2 PWM periods); no CCP pin-toggle sim. `example_ccp_pwm`
  exercises the half-bridge + dead-band path. XC8 all four devices.
- **SSP / MSSP** (done): the Master Synchronous Serial Port
  (SPI master/slave + I2C master/slave). Mirrors PIC16's `HAL_SSP_*` API
  (same `SSP_HandleTypeDef`, `SSP_*TypeDef`, weak `SSP_IRQHandler`);
  simpler than the PIC16 driver because the PIC18 MSSP registers are all
  in the Access Bank (no bank switching), and the control register is
  `SSPCON1` (PIC16's is `SSPCON`). Register-level only; the I2C state
  machine is left to the user. SFRs: SSPCON1/SSPCON2/SSPSTAT/SSPADD/
  SSPBUF. `example_ssp` verifies the SSPADD formula (16 MHz / 100 kHz →
  39, / 400 kHz → 9), SPI master programming, write/read loopback via the
  new `pic18_sim_drive_ssp_rx` hook, and I2C master Start/Stop (SEN/PEN).
  XC8 all four devices.
- **EUSART** (done): Enhanced USART. Mirrors PIC16's `HAL_USART_*` API +
  the PIC18 additions via `BAUDCON` + `SPBRGH` — 16-bit baud generator
  (`BRG16`), auto-baud detect (`ABDEN`), 9-bit address-detect (`ADDEN`).
  `USART_ComputeSPBRG` encodes all four rows of Table 20-1. SFRs:
  TXSTA/RCSTA/BAUDCON/SPBRG/SPBRGH/TXREG/RCREG. `example_usart` verifies
  the BRG math (8/16-bit), init, TX/RX, auto-baud, address-detect.
- **Comparator** (done): two on-chip comparators, mirrors PIC16's
  `HAL_COMP_*` API — PIC18 `CMCON` has the same bit layout and eight
  modes, just in the Access Bank. Added `PIC18_IRQ_CMP` (PIR2<CMIF>).
  `example_comp` verifies mode/inv/CIS, output readback, change flag.
- **Data EEPROM** (done): 256-byte data EEPROM, mirrors PIC16's
  `HAL_EEPROM_*` API; PIC18 moves the registers into the Access Bank and
  adds `EEPGD`/`CFGS` (kept 0 for data EEPROM). Write does the mandatory
  0x55→0xAA unlock; the sim models the cell array. `example_eeprom`
  verifies read, write unlock, completion (EEIF), buffer round-trip.
- **A/D Converter** (done): 10-bit ADC, 10/13 channels. A fresh design
  (not a mirror) for the PIC18's three control registers — `ADCON0`
  (channel/GO-DONE/ADON), `ADCON1` (PCFG pin config + VCFG Vref),
  `ADCON2` (ADCS clock / ACQT acquisition / ADFM). Tables cross-checked
  against DS39632E §21; PCFG is a raw Table 21-3 code. The ADC IRQ
  (`PIR1<ADIF>`) already existed; this wired `ADC_IRQHandler` into the
  dispatch. `example_adc` verifies init (3 ADCON regs), start/done, read
  in both ADFM justifications, Vref, channel select.
- **SPP** (done): Streaming Parallel Port, the USB-era parallel port
  (the PIC18 analog of PIC16's PSP), 40/44-pin only — fully gated through
  `PIC18FXX5X_FAMILY_HAS_SPP` across sfr.h, the driver, sim, dispatch,
  CMake (a `PIC18FXX5X_FAMILY_HAS_SPP` derivation mirroring the PIC16 PSP
  pattern), and the XC8 Makefile (`HAS_SPP` for 18F4455/18F4550 only).
  Register-level: programs SPPCON/SPPCFG/SPPEPS, byte-level SPPDATA
  access, busy/WR/RD status, SPPIF IRQ; the USB streaming protocol is
  left to the user. `example_spp` verifies init, write/read, status
  flags. XC8 all four devices (spp.c compiles only on the 40-pin parts).

**Phase 4 complete:** every peripheral on the PIC18F2455 family is
ported, validated host (16/16 examples, `-Wall -Wextra -Werror`) and XC8
(4/4 devices, no errors). USB and the extended instruction set remain
large enough to scope as separate future efforts, not part of "general
8-bit PIC support."

**Validation per peripheral** (repeat of the existing HAL's established
pattern, nothing new to invent): datasheet-cited driver, host-sim example,
XC8 build, and where hardware is available, a real-hardware confirmation,
matching the rigor already applied to every PIC16 peripheral driver.

---

## How to add family #3 (and #4), after this refactor lands

**Superseded by `docs/adding-a-device.md`.** That document is the
current operational procedure for adding a device or family (datasheet
sourcing, the Path A/new-variant vs Path B/new-family decision, and,
learned since this section was first written, a mandatory real-`mdb`
verification gate per peripheral that this section didn't have). Kept
here only as a pointer, not duplicated: the specific decisions recorded
below in "Open questions" (PIC18's interrupt pragma syntax, its BSR/
Access-Bank sim approach, etc.) remain a useful historical record of
what was actually decided and why, even though the step-by-step
checklist itself has moved.

## Open questions (resolve during the phase noted)

- **XC8 v3.10 PIC18 interrupt pragma/attribute syntax** — resolve in
  Phase 1, record the answer in this document. **RESOLVED (Phase 1):**
  PIC18F2455/2550/4455/4550 are classic dual-priority-vector PIC18
  devices (vectors at 0008h high, 0018h low; DS39632E §9.0), not the
  vectored-interrupt-controller (VIC) variants. Per the XC8 C Compiler
  User Guide for PIC (DS50002737K) §5.9.1.2 "Writing Dual-priority or
  Legacy Mode ISRs", Phase 2's `pic18_isr_vector.c` uses:

  ```c
  void __interrupt(high_priority) PIC18_IRQ_HandlerHigh(void) { ... }
  void __interrupt(low_priority)  PIC18_IRQ_HandlerLow(void)  { ... }
  ```

  (No priority argument ⇒ defaults to high priority; Microchip
  recommends always specifying it. The legacy `#pragma interrupt` /
  `#pragma interruptlow` form is obsolete in XC8 v2+.)
- **How much of PIC18's BSR/Access-Bank addressing to model in the host
  sim** (full 16-bank fidelity vs. a simpler flat array sized to what's
  actually implemented) — resolve during Phase 2 task 2. (Phase 1
  provisionally uses a flat 4096-byte array indexed by the 12-bit
  address; the macros stay the same regardless of the Phase 2 decision.)
  **RESOLVED (Phase 2):** flat array. The sim keeps the 4096-byte array
  indexed by the physical 12-bit address; the BSR is a stored SFR but
  the sim does no address translation. C drivers that need a banked GPR
  compute the physical address themselves, exactly as the PIC16 sim
  already works with its 512-byte flat array and per-bank driver
  addresses. This is sufficient for a C-level HAL where XC8 banks GPR
  automatically, and it keeps the two families' sim approaches
  symmetric. The MVP slice's SFRs (STATUS, BSR, PORTB/LATB/TRISB,
  INTCON/2/3, PIR1/PIE1/IPR1, TMR0L/H/T0CON, RCON) are all in the
  Access Bank (0xF60-0xFFF), so no banking is needed for them at all.
- **Shape of the `HAL_IRQ_*` priority contract extension** (optional
  parameter vs. separate setter) — resolve during Phase 2 task 5, before
  writing the interrupt core. **RESOLVED (Phase 2):** separate setter.
  Add `HAL_IRQ_SetPriority(irq, prio)` to the shared `pic8-common`
  contract with a shared `HAL_IRQ_PRIORITY_HIGH` / `HAL_IRQ_PRIORITY_LOW`
  enum. PIC16's implementation is a no-op (single vector, no priority);
  PIC18's writes the matching IPR bit. `HAL_IRQ_Enable` keeps its
  current signature, so `task_manager.c` and every existing PIC16
  example need zero changes — which is exactly what the Phase 3 litmus
  test demands. (An optional-parameter approach was rejected because it
  changes a shared signature and forces edits to every PIC16 caller.)
- **Hardware availability for real-silicon validation** — if PIC18
  hardware isn't on hand when Phase 2/3 validation is reached, flag it
  explicitly in the validation checklist as deferred rather than silently
  skipping the check.
- **PIC18Fxxxx DFP toolchain component** (resolved during Phase 1): XC8
  v3.x ships the PIC16Fxxx DFP but not the PIC18Fxxxx DFP, so the Phase 1
  XC8 `.hex` build initially failed with `error: (2104) no device-support
  files found`. Resolved by installing
  `Microchip.PIC18Fxxxx_DFP.1.7.171.atpack` into
  `/opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC18Fxxxx_DFP/`. This is
  an environment setup step, documented in
  `pic18fxx5x-hal/mcu/pic18fxx5x-mplabx/README.md`; it is not part of the
  repo. Family #3 onward will need its own DFP installed the same way.
