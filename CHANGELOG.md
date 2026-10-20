# Changelog

All notable changes to this project are documented here, generated from
Conventional Commits. Dates are UTC.
## [0.7.0] - 2026-10-20
### Breaking
- Group the repo root into common/, hal/<arch>/, lib/ and demos/ (#316)
- Derive module ids from the directory path (#421)
### Features
- PIC16F83_84 family on pic14-midrange-core, 16F84A exemplar (#166)
- 63x/67x/68x family with 16F677 canonical (#168)
- Onboard the remaining 63x/67x/68x parts (#173)
- Changelog ref cleanup and epic-hal CLI version stamp (#187)
- Family B phase 1 foundation, platform+SFR+harness+IRQ (#188)
- PIC18F2520 foundation (platform, SFR, GPIO, IRQ) (#190)
- Timer1/2/3 drivers through section 4 gate (#191)
- ECCP/CCP, MSSP, EUSART through section 4 gate (#192)
- ADC, Comparator, EEPROM, MANUAL, closeout (#193)
- Add PIC16F7x family, exemplar 16F77 (epic-hal#147) (#189)
- Onboard non-A 16F873 to the 87XA family (#195)
- Timer1/2/3 drivers through section 4 gate (#198)
- ECCP1 and EUSART drivers through section 4 gate (#199)
- ADC and Data EEPROM, MANUAL/README closeout, litmus (#200)
- PIC18F6520 foundation (platform, SFR, GPIO, IRQ) (#201)
- Timer1/2/3 drivers through section 4 gate (#202)
- CCP1-5, MSSP, EUSART1/2 drivers through section 4 gate (#203)
- ADC, comparator, data EEPROM, closeout (family C complete) (#205)
- Add PIC16F5x family, exemplar 16F54 (epic-hal#151) (#204)
- Wire epic-cc gate legs for the 2520/1320/6520 exemplars (#206)
- Wire family-check jobs for the 2520/1320/6520 families (#207)
- Onboard the remaining 87x parts (#209)
- Onboard the remaining PIC16F7x/7x7 parts (#210)
- PIC16F818_819 family on pic14-midrange-core (epic-hal#159) (#214)
- Onboard 18F2220 and 18F2320 (#211)
- Onboard 16F505/16F506/16F57/16F59 (#157) (#212)
- Onboard the 64/80-pin PIC18 6xxx/8xxx parts (#248)
- PIC18F4550 flagship demo, XC8 baseline (#251)
- Move the PIC16F818_819 epiccc slice onto the shared dispatch (#255)
- Epic-cc comparison tooling, fix a real UB bug it caught (#256)
- Add PID thermostat flagship demo plus XC8 baseline (#281)
- Add demo scoreboard doc plus nightly dual-toolchain matrix (#284)
- Add Modbus plus I2C expander flagship demo plus XC8 baseline (#283)
- Wire ECAN-quad USART/CCP and 80-pin GPIO on pic18f6520-hal (#295)
- Build a module's sim variant with epic-cc (#332)
- Declare minimum epic-cc, policy titles and notes template (#339)
- Expose epic-cc optimization profile in epic_build.py and epiccc-build (#340)
- Wire epic-tick for the PIC16F628A family (#354)
- Wire epic-tick for the PIC16F193X family (#357)
- Onboard PIC16F1508/1509 GPIO, IOC interrupts, and mdb smoke (#374)
- Add EPIC_CLC driver and CLC register contract (#386)
- Route CLC interrupts through PIR3/PIE3 (#385) (#390)
- Add per-cell CLC interrupt callback (#389) (#394)
- Model CLC cells in host sim with edge interrupts (#396)
- Verify CLC PIR3/PIE3 bits and select INTN/INTP edges in EPIC_CLC_Init (#401)
- CLC input-source table and output-behaviour gate (#402)
- S-R latch CLC demo, host probe plus mdb gate (Refs #363) (#412)
- Beat-frequency auto-dimmer host probe (#414)
- CLC switch-debouncer and quadrature demos, host plus mdb gate (#416)
- Timer drivers for the CLC demos (TMR2 clock, TMR1 counter) (#417)
- Generate HAL module index, family facts and README render (#422)
- Check the vendored epic-docs include against its recorded checksum (#438)
- Plain put_str when epic-cc defines __EPIC_CC_FLASH_PARAMS__ (#457)
- Decode CLC SEL sources in host sim with hook override (#463)
- Count quadrature steps on the TMR1 position counter (#472)
- Onboard sibling parts 18854/56/57/75/76/77 (#473)
- Port Timer0 with host sim and mdb gate (#483)
- Port EUSART, MSSP and PPS setup (#487)
- Port Timer1 gate control and Timer2 hardware limit modes (#486)
- Port EEPROM, WDT/sleep helpers and NCO (#491)
- Port CCP and PWM (#501)
- Gate smallest-flash variant on the PR matrix path (#514)
- Implement gpsim Tier-2 PIC16 check (#515)
- Model Timer2 start modes in host sim with per-mode probes (#519)
- Port Timer4 with host sim model and an mdb match gate (#530)
- Port the LVD driver with host sim model and an mdb readback gate (#533)
- Port the 16F506 comparator pair (#534)
- Port ADC, DAC, FVR and comparators (#510)
- Port Timer4 basic mode and run CCP/PWM on the Timer3/Timer4 couple (#535)
- Port the 16F506 ADC (#537)
- Route T1CKI, T1G and T2IN through PPS inputs (#540)
- Allow more than one MPLAB SIM gate per variant (#541)
- Gpio8 and SPI example programs plus manifest extra_examples (#565)
### Fixes
- Carry conditional_sources' after field through bundlegen (#208)
- Scope the pre-commit cppcheck includes per staged directory (#216) (#222)
- Gate the shared epiccc dispatch USART block on HAS_USART (#226)
- Distinguish a broken cppcheck install from real findings (#227) (#228)
- Add pic18fxx5x_timer0.c to combo-taskmgr-serial epiccc slice (#238)
- Dispatch TIMER0 overflow under epic-cc via a new tick tier (#239)
- Add pic18fxx5x_timer0.c and the tick tier to the PIC18Fxx5x epiccc family default (#242)
- CCP driver avoids storing a runtime-indexed GEP address (#253)
- Guard the shared epic-cc dispatch's Timer2 and CCP1 rows (#258)
- Refuse a stale cached driver in epiccc-build (#261)
- Reject unknown keys in manifest module and family tables (#268)
- Epiccc platform headers and gate coverage for the 628A and 83_84 slices (#269)
- Compare stamped driver sha in epiccc-build guard (#271)
- Derive canonical parts from manifest variants (#274)
- Restore the epiccc-build emit step dropped in the guard rework (#286)
- Never arm TXIE for polled mdb TX (#276)
- Include stddef.h where NULL is used (#287)
- Add eeprom.c to the bridge epiccc slice (#289)
- Point container HOME at the writable mount (#290)
- Derive gen-sfr canonical parts from manifest variants (#293)
- Move 193X sim and epic-cc legs to canonical 16F1939 (#294)
- Drop stale __EPIC_CC__ stubs after epic-cc#604 (#301)
- Resolve epic-cc dev image tag via dev-image-tag.sh (#305)
- Add sim-scoped epic-cc slice for epic-encoder with USART (#311)
- Repair stale pre-rename paths in bench, vendor docs and CI check (#320)
- Stop comparing the menu demo's non-asserted fire-tick line (#319)
- MODULE accepts a manifest id or a directory (#322)
- Reference projects name only files that exist (#326)
- Run all standalone tooling tests, fail fast on non-host-sim ids (#330)
- Keep the family IRQ table in swapped epic-cc driver slices (#343)
- Normalize control and bridge fire-tick lines in toolchain comparison (#346)
- Extract phase-4 hammer loop so main fits one page (#348)
- Add epic-cc sim slice with harness USART for PIC16 parts (#356)
- Authenticate and cache the epiccc-gate clang bundle fetch (#358)
- Call log_fire_ticks in sim main so the trace carries the line
- Run the worktree's own hook scripts, not the main checkout's
- Backfill SIM_VARIANTS with newer-family sim pairs
- Backfill flake-hunt sim list and guard it with the sync test
- Fail --check when a family's EDC is absent (#378)
- Ack unowned PIE1/PIE2/PIE3 sources in IRQ dispatcher (#392)
- Fail write mode when a family's EDC is absent (#399)
- Scaffold the blink with the watchdog off (#428)
- Define EPIC_HAL_DFP in the xc8 scaffold Makefile (#429)
- Stop build*/ from ignoring scripts/build/ (#443)
- Ship and select the epic-cc source slice for the scaffolded epic-cc build (#444)
- Set the xc8 scaffold TOOLCHAIN default before epic-hal.mk is included (#447)
- Remove XC8 size figures from public text (#450) (#451)
- Suppress link-stage 759/2098 on provably-NULL callbacks (#466)
- Pin the hub sha256 in the shared-include check (#484)
- Feed CLC T1_overflow and T2_match as pulses ahead of the postscaler (#489)
- Read PCON and RCON POR and BOR flags as active low (#539)
- Settle CLC cross-cell cascades in one host-sim step (#542)
- Restore EEPROM_WRITES help lost in multi-gate edit (#543)
- Report POR on fresh reset in PCON/RCON images (#545)
- Load OPTION_REG POR 0xFF in pic14 host-sim resets (#556)
- Gate Timer0 prescaler on PSA and clear it on reset (#566)
### Performance
- Store only CCP callbacks, drop 3x16B handle copies (#500)
- Store only CCP callbacks on 18f1320, 18f2520 and 18f6520 (#512)
- Size the taskmgr table to each demo's spawn count (#522)
### Documentation
- Generate the family lists from the manifest (#234)
- Point the README pack pin at the bundle epic-hal.mk (#232)
- Record epic-cc build and gate results for the PIC18 combos (#243)
- Record epic-cc#463's fix and the follow-up gate gap (#244)
- Document the shared epic-cc driver's glibc constraint (#259)
- Record what MPLAB X regenerates in a reference project (#265)
- Regenerate families and bundles docgen blocks (#275)
- Record which PIC18F4550 timers advance under MPLAB SIM (#291)
- Record epic-cc#604 root cause for lcd-tick F01/F03/F07 (#296)
- Refresh control-demo epic-cc row after #610 fix (#297)
- Close out dual-toolchain comparison on current master (#298)
- Never post to third-party repositories (#306)
- Point the review-gate summary at the PR Review record (#347)
- Human-friendly CONTRIBUTING and a lean, current README (#369)
- Use per-ticket mktemp for PR body files (#387)
- Record CLC1SEL1 sim-throw repro in MANUAL (#406)
- Push through epic-tasks push and trust its exit code (#407)
- Rewrite the README for the first major release (#425)
- Move design history under docs/internals with a migration map (#435)
- Generate the XC8 pack and bundle page from one DFP helper (#441)
- Describe the working tick gate, epic-cc#173 is closed (#440)
- HAL module index and per-module pages for the hub (#448)
- Use style guide terms for core names (#455)
- Use PIC14 (mid-range) for bank-switch row (#460)
- Rewrite opening with approved copy (#461)
- Normalize PIC16 arch shorthand to PIC14 (#464)
- Point to external-contributions rule (#467)
- Move math-cycle-benchmark experiment to private repo; widen XC8 lint (#475)
- Lead quickstart with open source MIT, drop no licence pitch (#485)
- Record the gate-image flash-budget lesson (#513)
- Record the MPLAB-SIM live vector flag-write limit (#531)
- Point the SFR layer note at the renumbered gotchas section (#536)
- Record CCP-driven ers gap for Timer2 start modes (#544)
- Record the lib-module by combo-gate coverage matrix (#558)
### Miscellaneous
- Pin the clang bundle to the stable v0.3.0 release (#172)
- Enforce Allman braces, retrofit merged family (#196)
- Retrofit whole repo to Allman braces (#197)
- Enable the 16F54 toggle leg (#220)
- Enable the PIC16F818_819 16F819 leg (#221)
- Align manifest and demo DFP pins with the toolchain image (#231)
- Guard the DFP pins against drift between image, manifest and demos (#262)
- Install one canonical part per family in the bundle gate (#263)
- Cover every manifest family, adding the three PIC18 families and 83_84 (#266)
- One-shot pic8-hal-ci to epic-hal-ci tag copy (#350)
- Drop pic8-hal-ci fallback, pull epic-hal-ci only (#351)
- Pass --report on target builds too (#408)
- Add 877A and 887 tick gates (#465)
### Refactor
- Make default ops tables static const on epic-cc
- Anchor cross-module paths at one repo root (#418)
- Group scripts/ by purpose (#419)
- Route every pack-name read through epicmanifest.dfp_name (#452)
- Route remaining pack reads through dfp_name, harden guard (#459)
### Testing
- Fail on guard skew between standalone and discovery runs (#342)
- Set RB0 digital in 18F4550 epic-bus toggle probe (#382)
- Epic-cc 18F4550 gate exercises mem dispatch; run it in CI (#398)
- Re-add init calls to epic-cc probe after pin bump (#403)
- Add host unit tests for delay, elapsed and wraparound (#511)
- Sync-test the family-check emit list (#516)
- Cover gpio8 and SPI transports on host (#517)
- Add host ctest for RTU framing, CRC and dispatch (#554)
- Gate golden_vectors.h against its generator in ctest (#555)
- Wire 16F72 gpio bank-probe into CI sim gates (#559)
- Gate EUSART live via uartio capture (#560)
- Clear the EUSART section-4 live gate via mdb readback (#562)
- Add host ctest for I2C/SPI MEM transactions via mock ops (#563)
## [0.6.0] - 2026-09-13
### Features
- Add PIC16F628A family support on shared pic14 core (#137)
- Onboard 16F627/627A/628/648A and the LF variants (#158)
### Fixes
- Route cut-release through a PR to satisfy the master ruleset (#161)
- Use a PAT for cut-release so its release PR checks actually run (#165)
### Miscellaneous
- Git-cliff changelog and manual cut-release workflow (#160)
### Refactor
- Converge the post-628A family layout leftovers (#141)
- Migrate 87XA/88X SSP driver into pic14-midrange-core (#142)
- Migrate 87XA/88X ADC driver into pic14-midrange-core (#143)
## [0.5.1] - 2026-09-10
### Features
- Epic-sdcard and epic-settings PIC18 slice under epic-cc (#125)
- Bring PIC16F193X onto the epic-cc path (#130)
- One-command XC8 size baseline for epic-cc#200 (#131)
### Fixes
- Authenticate the epic-tasks checkout for prose lint (#126)
### Documentation
- Require a separate code review before takeoff (#127)
- Board-track every filed issue (#128)
- Lead with the API, not the installer detour (#135)
### Miscellaneous
- Remove prose-ledger leftovers from the pre-PR gates (#133)
- Production-ready src comments without planning-stage prose (#134)
## [0.5.0] - 2026-09-03
### Features
- Add PIC16F88X family with full peripheral support (#56)
- Add epic-cc variant include/epiccc + src/epiccc (#64)
- Add epic-cc variant (HAL-1) (#66)
- Unified takeoff and worktree workflow (#69)
- Add epic-cc toolchain backend for HAL-2 (#73)
- Device registry SFR generation and canonical CI (#76)
- Build pic16f88x-hal for 887 under epic-cc (#77)
- Deterministic mdb toggle gate for epic-cc built firmware (#79)
- Pure logic modules under epic-cc on 16F877A/16F887 (#96)
- Lint gate replaces the PROSE=1 attestation (#99)
- Peripheral modules under epic-cc on 16F877A/16F887 (#103)
- Remove the __EPIC_CC__ stubs from epic-pid and epic-encoder (#108)
- Scheduling core under epic-cc on 16F877A/16F887 (#111)
- Serial stack epic-cc conformance (#112)
- Combo firmware epic-cc conformance, slices, tiers and headroom (#115)
- Literal printf shim and staged put_str on the epic-cc path (#117)
- Verify the epic-cc hexes in crates/sim, the public HAL-4 gate (#118)
- Flip distribution default to epic-cc (HAL-5) (#119)
- Migrate epic-pid example off printf, restore it under epic-cc (#120)
### Fixes
- Let --build-dir point outside the repository (#78)
- Guard against escaped newlines in PR bodies (#95)
- Share pir-reg, unify irq, make ccp const for epic-cc #67 (with filed gaps) (#100)
- Correct the PIC18Fxx5x epic-cc config-word translations (#113)
- Share one printf literal staging buffer instead of one per call site (#124)
### Documentation
- Point agents at the epic-tasks board for picking up work (#68)
- HAL file-per-device SFR generation + canonical-per-core CI (#72)
- State default task base is latest origin/master (#94)
- Shared non-variadic put API decision (#91) (#106)
### Miscellaneous
- Rename local image pic8-hal-toolchain to epic-hal-toolchain (#62)
- Rename GHCR image pic8-hal-ci to epic-hal-ci (#63)
- Port epic-cc's worktree, takeoff ritual, and prose conventions (#65)
- Run epic-cc in the dev image and add an mdb-hex gate (#81)
- Refuse force pushes unless the human approves (#82)
- Add make bootstrap and make doctor for first-time setup (#83)
- Exercise the epic-cc build path on a canonical job for #80 (#101)
- Re-pin the epiccc gate to epic-cc master after the #153 merge (#110)
- Drop dead PIC18 digit map entries, document xtal_hz (#114)
### Other
- Epic-common under epic-cc + the per-module recipe + 877A baseline (#93)
- Remove the EPIC_AT callback guards, share one ISR path (closes #105) (#107)

* refactor(hal): remove EPIC_AT callback guards, share one ISR path

* refactor(hal): inline timer0 Init/Start so epic-cc resolves the ISR callback

* ci(hal): pin the epiccc gate to the epic-cc companion branch for #105

* docs(hal): drop the 105 guard-removal plan, distilled into the PR

* ci(hal): re-pin the epiccc gate to the rebased companion SHA

* ci(hal): re-pin the epiccc gate to the post-review companion SHA
### Refactor
- Pass the manifest mcu to epic-cc --target (#75)
### Testing
- Measure the C path under epic-cc on 877A/887 (#104)
- Restore excluded-MCU test and make fixture assertions non-vacuous (#109)
## [0.4.0] - 2026-08-16
### Features
- Generate release notes from Conventional Commits (#53)
- One-command release via scripts/release.sh (#54)
### Refactor
- Rename Epicurus to Epic HAL (#52)
## [0.3.7] - 2026-08-14
### Features
- Clean default build - tick-only modules and targeted warning suppression (#51)
## [0.3.6] - 2026-08-14
### Features
- Scaffold Makefile builds into build/ with all/clean targets (#50)
### Fixes
- Report XC8/device-pack prerequisites upfront with exact install commands (#49)
## [0.3.5] - 2026-08-14
### Features
- Pure-library bundles; CLI ships as a separate release asset (#46)
### Fixes
- Makefile resolves XC8 and DFP dynamically with clear errors (#47)
- Release bundle gate skips the standalone CLI asset (#48)
## [0.3.4] - 2026-08-13
### Features
- Case-tolerant parts, consumer-only bundles, target-only examples (#45)
## [0.3.1] - 2026-08-13
### Features
- Pass a part, scaffold in place, vendored bundle (#44)
## [0.3.0] - 2026-08-13
### Features
- Add epic-mcp23x17 (MCP23017/MCP23S17 I/O expander)
- Add the GPIO-mimic layer (HAL-shaped per-pin API)
- Docker-first real-target setup with self-instructive vendor handling (#32)
- Skip tests on non-code changes (#33)
- Expression rules, full comment pass, bitacore deletion (#35)
- Doxygen-style function docstrings, whole project (#36)
- `epicurus init` scaffolder + .X patcher (#42)
- One-line curl getting started (#43)
### Fixes
- Correct PIC16 add/sub carry-fold when b_hi wraps (issue 34) (#38)
- Size PIC16 asm pins for worst-case -O codegen (#40)
### Documentation
- Benchmark of hand asm vs XC8 native math (#41)
### Other
- Merge pull request #30 from apojomovsky/feat/mcp23x17-module

feat(module): add epic-mcp23x17 (MCP23017/MCP23S17 I/O expander)
- Merge pull request #31 from apojomovsky/feat/mcp23x17-gpio-mimic

feat(mcp23x17): add the GPIO-mimic layer (HAL-shaped per-pin API)
### Refactor
- Environment-split src layout, bundle sim/mdb gate (#37)
- Enforce the epic_* naming convention across modules (#39)
## [0.2.0] - 2026-08-10
### Features
- Add exec, target-ci, and mdb-test EXTRA_MDB
- Real mdb gates for every simulatable module (20 gates), 20+ bugs fixed (#12)
- 12 peripheral/module interleave gates; PIE2 + EEIF dispatch fixes (#13)
- Add epic-combo-rx-loopback RX harness (task 8)
### Fixes
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
### Miscellaneous
- Remove stray vmain.p1 (codegen probe artifact) (#14)
- Remove stray XC8 probe artifacts; guard against recurrence (#15)
- Move the 12 combo test modules under tests/; root-directory rule (#17)
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

docs: rewrite the README around consuming the releases
- Merge pull request #11 from apojomovsky/feat/makefile-dev-targets

feat(make): add exec, target-ci, and mdb-test EXTRA_MDB
- Merge pull request #18 from apojomovsky/feat/sfr-config-audit

feat(audit): SFR-map DFP audit + config-key audit (quality tasks 6-7)
- Merge pull request #19 from apojomovsky/feat/swuart-ccp-hardening

fix(hal): CCP driver owns its callbacks (swuart/CCP handle hardening, quality task 4)
- Merge pull request #20 from apojomovsky/feat/host-property-fuzz

test: host property/fuzz tests for six modules (quality task 2)
- Merge pull request #21 from apojomovsky/docs/roadmap-statuses

docs: mark quality tasks 2-4 done in the roadmap
- Merge feat/hex-identity-audit (task 5c) into the layout/flakes branch
- Merge pull request #24 from apojomovsky/feat/rx-loopback-harness

feat(combo): RX loopback harness (task 8)
- Merge origin/master into the layout/flakes branch (rx-loopback landed)
- Merge pull request #22 from apojomovsky/feat/layout-rx-flakes

feat: layout hardening, statics pinning, PIC16 math replay, flake hunt (tasks 5, 9, 10)
- Merge pull request #25 from apojomovsky/feat/ci-speed

ci: shard the target job by family and parallelize its loops
- Merge pull request #26 from apojomovsky/feat/ci-image-slim

ci: slim the toolchain image 5.7 GB -> 3.98 GB
- Merge pull request #28 from apojomovsky/feat/warning-cleanup

test(gates): trim the 60s wait budgets and silence the triaged XC8 warnings
- Merge pull request #29 from apojomovsky/feat/release-followup

fix(gates): close unclosed comments, restore the 193X budget, decouple the README
### Refactor
- Stringdir ISR-handler conversion (class F) + GIE-race removal (class G) (#16)
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
### Fixes
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

Manifest and build driver (plan 1 of 3)
- Merge pull request #2 from apojomovsky/bundle-generator

Bundle generator (plan 2 of 3)
- Merge pull request #3 from apojomovsky/worktree-pic16f193x-fsm

feat(pic16f193x): port epic-fsm (module 1 of the rollout)
- Merge pull request #4 from apojomovsky/worktree-ci-consolidate-checks

ci: consolidate 4 workflows/~14 jobs into 1 workflow/2 jobs
- Merge remote-tracking branch 'origin/master' into mplabx-projects-and-release

# Conflicts:
#	.github/workflows/bundle-gate.yml
#	.gitignore
#	docs/superpowers/specs/2026-08-06-pic16f193x-module-rollout-design.md
- Merge pull request #6 from apojomovsky/swuart-bitbang

feat(swuart): add CCP hardware-timed bit-banged software UART
- Merge pull request #8 from apojomovsky/epic-math-bank1-fixup-overflow

fix(epic-math): pin pic16_mscratch to common RAM; bundle -I fix
- Merge pull request #7 from apojomovsky/swuart-rx-hotpath

fix(swuart): collapse RX confirm+arm into one synchronous pass (PIC16F87XA channel A)
- Merge pull request #9 from apojomovsky/fix/pic16-irq-dispatch-tick-sim

fix(hal): un-wedge epic-tick's PIC16 sim-target gate (GIE-race read, TXIE gating, dispatch co-location)
- Merge branch 'master' into mplabx-projects-and-release
- Merge pull request #5 from apojomovsky/mplabx-projects-and-release

feat(mplabx): reference MPLAB X projects + v0.1.0 release pipeline
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
