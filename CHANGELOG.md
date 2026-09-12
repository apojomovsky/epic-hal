# Changelog

All notable changes to this project are documented here, generated from
Conventional Commits. Dates are UTC.
## [Unreleased]

### Features

- Add PIC16F628A family support on shared pic14 core (#137) (#137)

- Generate CHANGELOG.md and cut releases with git-cliff


### Refactor

- Converge the post-628A family layout leftovers (#141) (#141)

- Migrate 87XA/88X SSP driver into pic14-midrange-core (#142) (#142)

- Migrate 87XA/88X ADC driver into pic14-midrange-core (#143) (#143)

- Dedupe git-cliff setup into a shared composite action

## [0.5.1] - 2026-09-10

### Bug Fixes

- Authenticate the epic-tasks checkout for prose lint (#126) (#126)


### Documentation

- Require a separate code review before takeoff (#127) (#127)

- Board-track every filed issue (#128) (#128)

- Lead with the API, not the installer detour (#135) (#135)


### Features

- Epic-sdcard and epic-settings PIC18 slice under epic-cc (#125) (#125)

- Bring PIC16F193X onto the epic-cc path (#130) (#130)

- One-command XC8 size baseline for epic-cc#200 (#131) (#131)


### Miscellaneous

- Remove prose-ledger leftovers from the pre-PR gates (#133) (#133)

- Production-ready src comments without planning-stage prose (#134) (#134)

## [0.5.0] - 2026-09-03

### Bug Fixes

- Let --build-dir point outside the repository (#78) (#78)

- Guard against escaped newlines in PR bodies (#95) (#95)

- Share pir-reg, unify irq, make ccp const for epic-cc #67 (with filed gaps) (#100) (#100)

- Correct the PIC18Fxx5x epic-cc config-word translations (#113) (#113)

- Share one printf literal staging buffer instead of one per call site (#124) (#124)


### Documentation

- Point agents at the epic-tasks board for picking up work (#68) (#68)

- HAL file-per-device SFR generation + canonical-per-core CI (#72) (#72)

- State default task base is latest origin/master (#94) (#94)

- Shared non-variadic put API decision (#91) (#106) (#106)


### Features

- Add PIC16F88X family with full peripheral support (#56) (#56)

- Add epic-cc variant include/epiccc + src/epiccc (#64) (#64)

- Add epic-cc variant (HAL-1) (#66) (#66)

- Unified takeoff and worktree workflow (#69) (#69)

- Add epic-cc toolchain backend for HAL-2 (#73) (#73)

- Device registry SFR generation and canonical CI (#76) (#76)

- Build pic16f88x-hal for 887 under epic-cc (#77) (#77)

- Deterministic mdb toggle gate for epic-cc built firmware (#79) (#79)

- Pure logic modules under epic-cc on 16F877A/16F887 (#96) (#96)

- Lint gate replaces the PROSE=1 attestation (#99) (#99)

- Peripheral modules under epic-cc on 16F877A/16F887 (#103) (#103)

- Remove the __EPIC_CC__ stubs from epic-pid and epic-encoder (#108) (#108)

- Scheduling core under epic-cc on 16F877A/16F887 (#111) (#111)

- Serial stack epic-cc conformance (#112) (#112)

- Combo firmware epic-cc conformance, slices, tiers and headroom (#115) (#115)

- Literal printf shim and staged put_str on the epic-cc path (#117) (#117)

- Verify the epic-cc hexes in crates/sim, the public HAL-4 gate (#118) (#118)

- Flip distribution default to epic-cc (HAL-5) (#119) (#119)

- Migrate epic-pid example off printf, restore it under epic-cc (#120) (#120)


### Miscellaneous

- Rename local image pic8-hal-toolchain to epic-hal-toolchain (#62) (#62)

- Rename GHCR image pic8-hal-ci to epic-hal-ci (#63) (#63)

- Port epic-cc's worktree, takeoff ritual, and prose conventions (#65) (#65)

- Run epic-cc in the dev image and add an mdb-hex gate (#81) (#81)

- Refuse force pushes unless the human approves (#82) (#82)

- Add make bootstrap and make doctor for first-time setup (#83) (#83)

- Exercise the epic-cc build path on a canonical job for #80 (#101) (#101)

- Re-pin the epiccc gate to epic-cc master after the #153 merge (#110) (#110)

- Drop dead PIC18 digit map entries, document xtal_hz (#114) (#114)


### Other

- Epic-common under epic-cc + the per-module recipe + 877A baseline (#93) (#93)

- Remove the EPIC_AT callback guards, share one ISR path (closes #105) (#107)

* refactor(hal): remove EPIC_AT callback guards, share one ISR path

* refactor(hal): inline timer0 Init/Start so epic-cc resolves the ISR callback

* ci(hal): pin the epiccc gate to the epic-cc companion branch for #105

* docs(hal): drop the 105 guard-removal plan, distilled into the PR

* ci(hal): re-pin the epiccc gate to the rebased companion SHA

* ci(hal): re-pin the epiccc gate to the post-review companion SHA (#107)


### Refactor

- Pass the manifest mcu to epic-cc --target (#75) (#75)


### Testing

- Measure the C path under epic-cc on 877A/887 (#104) (#104)

- Restore excluded-MCU test and make fixture assertions non-vacuous (#109) (#109)

## [0.4.0] - 2026-08-16

### Features

- Generate release notes from Conventional Commits (#53) (#53)

- One-command release via scripts/release.sh (#54) (#54)


### Refactor

- Rename Epicurus to Epic HAL (#52) (#52)

## [0.3.7] - 2026-08-14

### Features

- Clean default build - tick-only modules and targeted warning suppression (#51) (#51)

## [0.3.6] - 2026-08-14

### Bug Fixes

- Report XC8/device-pack prerequisites upfront with exact install commands (#49) (#49)


### Features

- Scaffold Makefile builds into build/ with all/clean targets (#50) (#50)

## [0.3.5] - 2026-08-14

### Bug Fixes

- Makefile resolves XC8 and DFP dynamically with clear errors (#47) (#47)

- Release bundle gate skips the standalone CLI asset (#48) (#48)


### Features

- Pure-library bundles; CLI ships as a separate release asset (#46) (#46)

## [0.3.4] - 2026-08-13

### Features

- Case-tolerant parts, consumer-only bundles, target-only examples (#45) (#45)

## [0.3.1] - 2026-08-13

### Features

- Pass a part, scaffold in place, vendored bundle (#44) (#44)

## [0.3.0] - 2026-08-13

### Bug Fixes

- Correct PIC16 add/sub carry-fold when b_hi wraps (issue 34) (#38) (#38)

- Size PIC16 asm pins for worst-case -O codegen (#40) (#40)


### Documentation

- Benchmark of hand asm vs XC8 native math (#41) (#41)


### Features

- Add epic-mcp23x17 (MCP23017/MCP23S17 I/O expander)

- Add the GPIO-mimic layer (HAL-shaped per-pin API)

- Docker-first real-target setup with self-instructive vendor handling (#32) (#32)

- Skip tests on non-code changes (#33) (#33)

- Expression rules, full comment pass, bitacore deletion (#35) (#35)

- Doxygen-style function docstrings, whole project (#36) (#36)

- `epicurus init` scaffolder + .X patcher (#42) (#42)

- One-line curl getting started (#43) (#43)


### Other

- Merge pull request #30 from apojomovsky/feat/mcp23x17-module

feat(module): add epic-mcp23x17 (MCP23017/MCP23S17 I/O expander) (#30)

- Merge pull request #31 from apojomovsky/feat/mcp23x17-gpio-mimic

feat(mcp23x17): add the GPIO-mimic layer (HAL-shaped per-pin API) (#31)


### Refactor

- Environment-split src layout, bundle sim/mdb gate (#37) (#37)

- Enforce the epic_* naming convention across modules (#39) (#39)

## [0.2.0] - 2026-08-10

### Bug Fixes

- Scope the top-level-directory rule to subdirectory paths

- Correct the TRISE PSP status bits and drop bogus SFR constants

- CCP driver stores driver-owned callbacks

- Pin the IRQ-shared storage to the ISRs' baked IRP banks

- Pin the PIC16 asm leaves and fix the replay-exposed bugs

- Keep the sim gate single-pass under any run length

- Pin the taskmgr TCB array and extend EPIC_PLACE to all families

- Fit the shared scratch into the common RAM without colliding

- Close the unclosed comments, restore the 193X gate budget, decouple the README


### Documentation

- Design the consumption-first README rewrite

- Rewrite the README around consuming the releases

- Center the hero badges, rename the blink example heading

- Extend the quality roadmap with tasks 6-10

- Mark quality tasks 6-7 done

- Mark quality tasks 2-4 done in the roadmap

- Record the layout budgets and the statics pinning

- Mark quality tasks 5, 9, 10 done in the roadmap

- Mark task 5c done

- Mark task 8 RX harness landed


### Features

- Add exec, target-ci, and mdb-test EXTRA_MDB

- Real mdb gates for every simulatable module (20 gates), 20+ bugs fixed (#12) (#12)

- 12 peripheral/module interleave gates; PIE2 + EEIF dispatch fixes (#13) (#13)

- Add epic-combo-rx-loopback RX harness (task 8)


### Miscellaneous

- Remove stray vmain.p1 (codegen probe artifact) (#14) (#14)

- Remove stray XC8 probe artifacts; guard against recurrence (#15) (#15)

- Move the 12 combo test modules under tests/; root-directory rule (#17) (#17)

- Gate on the SFR-map and config-key audits

- Layout hardening, statics pinning, RX harness, flake hunt (tasks 5, 8-10)

- Record the CI target-job speed follow-up

- Run the statics audit in the target job

- Add the manual flake-hunt job

- Run the hex identity audit in the target job

- Name the composed device-data audit step accurately

- Include the rx-loopback sim build in the flake-hunt emit list

- Skip emit-list modules the branch does not have yet

- Emit the rx-loopback sim build in the target job's emit list

- Skip emit-list modules the branch does not have yet

- Shard the target job by family and parallelize its loops

- Use a reusable workflow for the family checks (no YAML merge key)

- Keep the sim gates sequential under the shard

- Slim the toolchain image 5.7 GB -> 3.98 GB


### Other

- Merge pull request #10 from apojomovsky/docs/readme-rewrite

docs: rewrite the README around consuming the releases (#10)

- Merge pull request #11 from apojomovsky/feat/makefile-dev-targets

feat(make): add exec, target-ci, and mdb-test EXTRA_MDB (#11)

- Merge pull request #18 from apojomovsky/feat/sfr-config-audit

feat(audit): SFR-map DFP audit + config-key audit (quality tasks 6-7) (#18)

- Merge pull request #19 from apojomovsky/feat/swuart-ccp-hardening

fix(hal): CCP driver owns its callbacks (swuart/CCP handle hardening, quality task 4) (#19)

- Merge pull request #20 from apojomovsky/feat/host-property-fuzz

test: host property/fuzz tests for six modules (quality task 2) (#20)

- Merge pull request #21 from apojomovsky/docs/roadmap-statuses

docs: mark quality tasks 2-4 done in the roadmap (#21)

- Merge feat/hex-identity-audit (task 5c) into the layout/flakes branch

- Merge pull request #24 from apojomovsky/feat/rx-loopback-harness

feat(combo): RX loopback harness (task 8) (#24)

- Merge origin/master into the layout/flakes branch (rx-loopback landed)

- Merge pull request #22 from apojomovsky/feat/layout-rx-flakes

feat: layout hardening, statics pinning, PIC16 math replay, flake hunt (tasks 5, 9, 10) (#22)

- Merge pull request #25 from apojomovsky/feat/ci-speed

ci: shard the target job by family and parallelize its loops (#25)

- Merge pull request #26 from apojomovsky/feat/ci-image-slim

ci: slim the toolchain image 5.7 GB -> 3.98 GB (#27)

- Merge pull request #28 from apojomovsky/feat/warning-cleanup

test(gates): trim the 60s wait budgets and silence the triaged XC8 warnings (#28)

- Merge pull request #29 from apojomovsky/feat/release-followup

fix(gates): close unclosed comments, restore the 193X budget, decouple the README (#29)


### Refactor

- Stringdir ISR-handler conversion (class F) + GIE-race removal (class G) (#16) (#16)


### Testing

- Add the SFR-map DFP audit

- Add the config-key audit

- Drop the C11 gate's __at(0x140) handle pin

- Add algebraic property tests for fixed-point ops

- Add randomized ring-buffer stress test

- Add scheduler fuzz test for spawn/kill invariants

- Add randomized CRC round-trip and corruption fuzz test

- Add parser fuzz test for the line state machine

- Add randomized ring and error-count fuzz test

- Add the statics audit for unpinned IRQ-shared statics

- REPEAT=N flake-hunt support in the sim gate loop

- Add hex-rebuild identity check

- Trim the 60s wait budgets and silence the triaged XC8 warnings

## [0.1.0] - 2026-08-08

### Bug Fixes

- Drop dead uint8_t >= 256 guards in sim EEPROM

- Move to XC8 v4.00, self-host the installer, fetch both DFPs

- Don't assume every module's .hex filename, glob for it

- Exclude 40 known-broken (module, MCU) pairs, file the bugs

- Stop redistributing Microchip's installer, use a private GHCR asset

- Drop the ineffective visibility-API steps and the now-dead bootstrap

- Remove ci-assets-mplabx's now-dead bootstrap fallback

- Matrix xc8-build per module, not per (module, MCU)

- Matrix xc8-build per family, not per module

- Pin xc8-build's build step to shell: bash

- Enable global interrupts in pic8_tick_init

- Sim-target harness, disable TXIE after the TXEN workaround

- Partial fix for Bank 1 SFR codegen corruption

- Fix PIE1/PIE2 RMW, dangling USART handle, WDT/sim

- Exclude pic8-math/pic8-modbus PIC16 pairs broken by the PIE1 fix

- PR2/SPBRG corrupted across HAL_TIMER2_WritePeriod/HAL_USART_Init's own bank switch

- Same bank-switch corruption in ADC/VREF/COMP/PSP/SSP/EEPROM

- Sim-target harness baud-rate math and missing WDT=OFF knob

- Rewrite pic18_irq.c to avoid runtime-addressed SFR access

- Pic18fxx5x_ccp.c had the same runtime-address SFR bug

- Sim-tests.yml's wait_ms=80 was starving pic18fxx5x of real time

- Run containers as the host user, not root

- Install cmake/build-essential for local-dev host tests

- Give the mapped container user a real, writable HOME

- Register pic16f193x family in xc8-build matrix discovery

- Prune MPLAB X IDE's bundled hardware/AVR packs, ~10.8GB to ~5.7GB

- Suppress cppcheck preprocessorErrorDirective in pre-commit

- Suppress epic_build.py build stdout in the emit step

- Pass --dfp-dir when emitting build scripts in CI

- Splice conditional sources at their real position

- Compile dependencies before pid.c, matching every other module

- Alphabetize MATH_SOURCES on PIC18, matching the manifest

- Handle a module whose dir is a family's own hal_dir

- Key emitted build-dir by mod.dir, not the module name

- Find the sim .hex at build-sim/<module>, not build-sim/<module>/<mcu>

- Pin pic16_mscratch to common RAM 0x70

- Prefix -I to every module include dir in epicurus.mk

- Collapse RX confirm+arm into one synchronous pass, PIC16F87XA channel A

- Derive RX_CAPTURE_OVERHEAD_CYCLES from a real mdb probe

- Gate TMR1 dispatch on TMR1IE; confine RX fast path to PIC16F87XA

- Restrict channel B to PIC16F193X variants that have PORTD

- Exclude PIC16F1934 from channel B, insufficient flash

- Exclude epic-swuart/epic-taskmgr from 16F873A/874A

- Un-wedge epic-tick's PIC16 sim-target gate


### Documentation

- Remove dev-only and self-deprecating comments

- Add MANUAL.md: the human-readable manual

- Brand the repo for public release: root README, MIT license, split docs

- Tighten prose voice across docs, comments, and docstrings

- Multi-family PIC HAL refactor plan (PIC18 as first new family)

- Rebrand root README to a multi-family framing

- Mark PIC18 Phase 4 complete (all peripherals ported)

- Add pic8-math implementation plan (AN526/AN544 port)

- XC8 inline-asm binding probe -- establish the convention

- Add pic8-fsm to the root component table

- Extract XC8 v3.10 compat fixes as a reapplicable patch

- Add README, ARCHITECTURE, and API docs for pic8-usb and pic8-sdcard

- Stop vendoring datasheet PDFs, link Microchip's hosted copies instead

- Split HAL manual into shared conventions + per-family reference

- Add AGENTS.md with CLAUDE.md symlink

- Commit after every finished piece of work

- Phase 4 architecture, API, README, root component row

- Phase 4 architecture, API, README, MANUAL addenda

- Overhaul top-level README

- Overhaul top-level README

- Record Phase 0's first green run

- Record Phase 1's confirmed green run

- Record that automated visibility-private failed

- Record that Phase 1's confirmed green run

- Record Phase 2's mdb/MPLAB SIM findings

- Phase 2 done, confirmed on a clean run

- Track the PIE1-fix RAM regression

- Cross-check Phase 4 XC8 findings against the real manual

- Dispatcher-depth experiment weakens the stack-depth theory

- Rule out the XC8E-11 indirect-call theory too

- Targeted variable pinning moves the corruption, doesn't fix it

- Storage-diff forensics found candidates, neither panned out

- Record the Phase 4 root cause and mark it resolved

- Record the ADC/VREF/COMP/PSP/SSP/EEPROM fix

- Add device/family bring-up guide, supersede the old family-#3 checklist

- Refresh stale Status lines on 4 shipped/partially-shipped modules

- Mark hal-manual-plan.md done, trim pic8-modbus-plan.md to a pointer

- Fix stale/broken cross-references, add missing doc links

- Require doc updates as part of finishing dev/fix work

- Mark foundation host-verified in plan doc

- Cross-reference the Docker-first local dev flow

- Document the MPLAB X/XC8 installer wall explicitly

- Record full end-to-end verification, including mdb

- Refresh front page for CI, Docker, and the 193X family

- Fix stale mdb-pending claims after toolchain landed

- Bring AGENTS.md up to date with the third family + tooling

- Document the native + Docker dev/debug/iterate cycle

- Timer2/4/6 register reference + bank-8 addendum

- Rewrite README for all 13 peripherals landed

- Rewrite project README opening, update for PIC16F193X full coverage

- Fold PIC16F193X lessons into the adding-a-device guide

- Brand the project as Epicurus

- Drop the meta explanation from Epicurus's README title

- Give Epicurus a slogan, logo, and an opening that earns the name

- Add the other 5 logo concepts to the README for comparison

- Replace hand-drawn logo candidates with the Gemini-generated one

- Center the README title/slogan, drop the C99 and host&silicon badges

- Drop the feature-list paragraph from the README intro

- Pull the dual-target-build paragraph out of the README intro too

- Add distribution and consumption design spec

- Add the handoff prompt for the distribution plans

- Document the modules.toml schema

- Update every doc that described the Makefile build

- Document consuming Epicurus from a release bundle

- Record the headless-build probe result

- Confirm the headless-build probe against a real project

- Record epic-fsm's PIC16F1937 footprint

- Document the RX hot-path fix, close its spec, update v3's disclosed limitation

- Correct the RX hot-path fix's disclosed-limitations scope


### Features

- Initial scaffold with GPIO, interrupts, sim backend

- Timer0, Timer1, Timer2 drivers

- CCP1 / CCP2 driver (capture / compare / PWM)

- USART driver (async + sync, SPBRG baud-rate calc)

- MSSP driver (SPI + I2C, register-level)

- ADC driver (10-bit, 5/8 channels)

- Comparator and Vref drivers

- Data EEPROM driver

- Parallel Slave Port driver (40/44-pin only)

- WDT refresh, Sleep, BOR/POR status helpers

- MPLAB X / XC8 project template

- MPLAB X / XC8 Makefile

- Working XC8 v3.10 target build (.hex for all 4 devices)

- Idle blink example (Timer1 + T1OSC + Sleep) + ISR vector

- Cooperative task manager + multi-blink example

- Stream per-task dispatch logs in the example

- Scaffold pic18f2455-hal (empty backend, build seam proven)

- Port the MVP vertical slice from DS39632E to pic18f2455-hal

- Point the task manager at pic18f2455-hal (litmus test met)

- Port PIC18 Timer1 + Timer2

- Port PIC18 Timer3

- Port PIC18 ECCP1 + CCP2

- Port PIC18 MSSP (SSP) driver

- Port PIC18 EUSART driver

- Port PIC18 Comparator driver

- Port PIC18 Data EEPROM driver

- Port PIC18 A/D Converter driver

- Port PIC18 Streaming Parallel Port driver

- Add task_reset, the re-arm/re-trigger verb

- Vendor-agnostic table-driven FSM engine

- Phase 0 scaffold -- tree, CMake, XC8 Makefiles, smoke tests

- Phase 1 core arithmetic -- mul/div/addsub, 3 backends + tests

- Phase 2 BCD -- conversions + DAW adjust, 3 backends + tests

- Phase 3 derived numeric routines -- sqrt/diff3/simpson38

- Phase 4 RNGs -- LFSR rand_next + CLT rand_gauss, tests

- Phase 5 validation + docs -- golden vectors, selftest, README

- 1 ms timebase (HAL_GetTick/HAL_Delay equivalent) on Timer2

- Interrupt-driven ring-buffered UART + printf retarget

- I2C/SPI MEM register-access idiom on the MSSP/SSP HAL

- Vendor-agnostic digital-input debouncer on pic8-tick

- Vendor-agnostic ADC oversampling + moving-average filter

- Add pic8 utility modules

- USB CDC-ACM wrapper for PIC18F4550, real XC8 build validated

- SD/MMC-over-SPI storage wrapper, mock-verified + real XC8 build

- HD44780 character LCD driver with GPIO and SPI transports

- Add a Modbus RTU slave built on pic8-serial + pic8-tick

- Phase 0 scaffold + signed-shift probe

- Phase 1 core engine + full test suite

- Phase 2 setpoint-step + manual/auto transfer example

- Phase 3 cross-compile sanity check + footprint

- Phase 0a GPIO change-interrupt hook in both HALs

- Phase 0b scaffolding

- Phase 1 core engine + full test suite

- Phase 2 examples

- Phase 3 cross-compile sanity check + footprint

- Add host CMake/ctest matrix, reuse pre-commit checks in CI

- Add XC8 cross-compile matrix (Phase 1)

- Add MPLAB X IDE to the toolchain image (Phase 2)

- Phase 3, sim-target harness for the pic8-tick pilot

- Phase 4, wire the sim-target pilot into CI (sim-tests.yml)

- Local mdb reproduction, dedupe sim-tests.yml's build+run logic

- Add new enhanced mid-range HAL family foundation

- Install PIC12-16F1xxx DFP, verify real-target build

- Pin PIC12-16F1xxx_DFP, note dual CI/local-dev consumers

- Add root Makefile for Docker-first local dev

- MODE=gpio PORTA-marker mdb reporting

- Timer1 peripheral through the host-sim smoke gate and the §4 mdb gate

- Timer2/Timer4/Timer6 peripheral through the §4 gate

- CCP1/CCP2 peripheral (capture/compare) through the §4 gate

- EUSART peripheral (async 8-bit) through the §4 gate

- MSSP SPI-master peripheral through the §4 gate

- ADC peripheral through the §4 gate

- Dual comparator (C1/C2) peripheral through the §4 gate

- EEPROM peripheral through the §4 gate

- DAC, FVR, SR latch, CPS peripherals through the §4 gate

- CCP3/CCP4/CCP5 peripheral through the §4 gate

- LCD segment driver through the §4 gate

- Add manifest schema loader and validator

- Resolve dependencies, sources, and includes

- Generate modules.toml from the existing Makefiles

- Add the xc8-cc build-script emitter

- Drive make xc8-build through epic_build.py

- Add a module for PIC16F193X's bare-HAL firmware smoke

- Model the HARNESS=sim build variant

- Populate the real HARNESS=sim variants

- Teach epic_build.py the HARNESS=sim variant

- Resolve per-family bundle contents from the manifest

- Emit the consumer-facing epicurus.mk fragment

- Emit epicurus-sources.json and SUPPORT.md

- Emit QUICKSTART.md and MPLABX.md

- Add the bundle assembly CLI

- Add one reference MPLAB X project per family

- Ship the family's reference MPLAB X project

- Build, gate, and publish bundles on tag

- Add epic-fsm to the build manifest


### Miscellaneous

- Replace em dashes with common punctuation across docs and code

- Replace em-dashes with commas/colons/periods in recent modules

- USB CDC-ACM plan + vendor M-Stack for PIC18F4550

- SD/MMC-over-SPI storage plan, vendoring M-Stack's storage/

- Design notes for a PIC16F877A VGA video generator for a 6502

- Add missing trailing newlines in pic8-encoder + HAL GPIO files

- Add a pre-commit hook (trailing whitespace, no-em-dash, cppcheck)

- Add scripts/bootstrap.sh for fresh-clone dev setup

- Add CI plan (host tests, XC8 cross-compile, MPLAB SIM target tests)

- Retrigger xc8-build now that ci-toolchain-assets is published

- Dump key SFRs at halt in sim-tests.yml

- Shorten sim-tests.yml wait to sample pre-reset state

- Register dump at wait=80 (past setup, before any WDT reset)

- Trim bloated/leftover-iteration comments to production-terse form

- Trim bloated/leftover-iteration comments to production-terse form

- Trim bloated/leftover-iteration comments to production-terse form

- Trim bloated/leftover-iteration comments to production-terse form

- Trim bloated/leftover-iteration comments to production-terse form

- Trim bloated/leftover-iteration comments to production-terse form

- Trim bloated/leftover-iteration comments to production-terse form

- Trim bloated comments to production-terse form

- Trim bloated comments to production-terse form

- Trim bloated comments to production-terse form

- Trim bloated comments to production-terse form

- Trim bloated comments to production-terse form

- Trim bloated comments to production-terse form

- Trim bloated comments to production-terse form

- Trim bloated comments to production-terse form

- Trim bloated comments to production-terse form

- Trim bloated comments to production-terse form

- Trim bloated comments to production-terse form

- Gitignore .claude/ (local Claude Code state)

- Add plan doc for new enhanced mid-range family

- Add plan doc for Docker-first local dev + CI image push

- Design + task breakdown for Phase 0 + Timer1

- One detailed implementation plan per remaining peripheral

- Skip docs-only PRs, narrow host-tests to affected modules

- Wire PIC16F193X's mdb gate into sim-tests.yml

- Add the three distribution implementation plans

- Surface the manifest build's stderr in the gate log

- Design the higher-level module rollout roadmap

- Design the higher-level module rollout roadmap

- Add the epic-fsm port implementation plan

- Consolidate 4 workflows/~14 jobs into 1 workflow/2 jobs

- Design a fix for the real PIC16F876A/877A link failure

- Design an RX hot-path fix for v3's timing race

- Write the RX hot-path fix implementation plan


### Other

- Merge pull request #1 from apojomovsky/distribution-manifest

Manifest and build driver (plan 1 of 3) (#1)

- Merge pull request #2 from apojomovsky/bundle-generator

Bundle generator (plan 2 of 3) (#2)

- Merge pull request #3 from apojomovsky/worktree-pic16f193x-fsm

feat(pic16f193x): port epic-fsm (module 1 of the rollout) (#3)

- Merge pull request #4 from apojomovsky/worktree-ci-consolidate-checks

ci: consolidate 4 workflows/~14 jobs into 1 workflow/2 jobs (#4)

- Merge remote-tracking branch 'origin/master' into mplabx-projects-and-release

# Conflicts:
#	.github/workflows/bundle-gate.yml
#	.gitignore
#	docs/superpowers/specs/2026-08-06-pic16f193x-module-rollout-design.md

- Merge pull request #6 from apojomovsky/swuart-bitbang

feat(swuart): add CCP hardware-timed bit-banged software UART (#6)

- Merge pull request #8 from apojomovsky/epic-math-bank1-fixup-overflow

fix(epic-math): pin pic16_mscratch to common RAM; bundle -I fix (#8)

- Merge pull request #7 from apojomovsky/swuart-rx-hotpath

fix(swuart): collapse RX confirm+arm into one synchronous pass (PIC16F87XA channel A) (#7)

- Merge pull request #9 from apojomovsky/fix/pic16-irq-dispatch-tick-sim

fix(hal): un-wedge epic-tick's PIC16 sim-target gate (GIE-race read, TXIE gating, dispatch co-location) (#9)

- Merge branch 'master' into mplabx-projects-and-release

- Merge pull request #5 from apojomovsky/mplabx-projects-and-release

feat(mplabx): reference MPLAB X projects + v0.1.0 release pipeline (#5)


### Refactor

- Unify example_blink into a single main

- #ifdef-free examples via link-time harness

- Remove the last build-mode #ifdef from the HAL

- Drop dead __cplusplus guards from headers

- Extract pic8-common shared layer, rename in place

- Rename task manager to pic8-taskmgr (fully family-agnostic)

- Rename pic18f2455-hal family tree to pic18fxx5x-hal

- Pull the toolchain image instead of building it

- Rename HAL_ prefix to EPIC_ across the codebase

- Rename pic8_/pic8- common+module layer to epic_/epic-

- Rename PIC8_ uppercase macros/vars to EPIC_

- Build xc8-build's matrix from the manifest

- Point sim-tests at manifest-built HARNESS=sim firmware

- Delete the 29 mcu/*-mplabx Makefiles


### Testing

- Gate the migration on byte-identical .hex output

- Extend the equivalence gate to the HARNESS=sim variants

- Gate bundles on building outside the repo

- Cover the reference MPLAB X projects in bundle-gate

- Re-target the reference-project gate at ci-target-bundle.sh
