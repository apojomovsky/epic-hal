# Design note: PIC16F5x remaining parts, 16F505/16F506/16F57/16F59 (epic-hal#157)

Status: pending approval (design gate). Ephemeral plan, deleted before merge.

Path A per `docs/adding-a-device.md` §2/§3: the four parts join the
existing `pic16f5x-hal` family, which #151 already created and which
already declares them. No new family, no new tree, no new peripherals
beyond the one defect below.

## 1. What master already has (measured, not assumed)

`feat(hal): add PIC16F5x family, exemplar 16F54` (74da1dd, PR #204)
pre-staged the whole per-part skeleton. The four "new" parts are already
declared in every declaration surface:

- `epic-common/manifest/modules.toml` `[families.PIC16F5x].variants`
  (`16F505, 16F506, 16F57, 16F59, 16F54`) and
  `[modules.pic16f5x-hal.supported]`.
- `pic16f5x-hal/include/pic16f5x_hal.h` per-part capability blocks
  (flash/RAM/FSR bank bits/PORTA-E/OSCCAL/COMP_ADC/six-bit ports).
- `pic16f5x-hal/include/pic16f5x_sfr.h` per-part `#if` guards.
- `pic16f5x-hal/CMakeLists.txt:32` `EPIC_FAMILY_DEVICES` (all five) with
  `epic_add_example_per_device`.
- `scripts/sfr-map-audit.py` FAMILIES tuples, `CONDITIONAL_REGS`,
  `CONDITIONAL_BITS` for all five parts.
- `.github/workflows/ci.yml` / `nightly.yml` `family-5x` job, which
  sweeps every manifest variant on push/nightly.

Starting state measured in a worktree off `064023d`:

| probe | command | result |
|---|---|---|
| host-sim, every device | `make test MODULE=pic16f5x-hal` + each `build/example_blink_<PART>` | all exit 0 (ctest registers no tests in a HAL family) |
| mdb toggle gate, 16F54 | `make mdb-test MODULE=pic16f5x-hal MCU=16F54 DEVICE=PIC16F54 MODE=toggle STEPI=50000` | PASS (11 transitions) |
| mdb toggle gate, 16F57 | same, `MCU=16F57 DEVICE=PIC16F57` | PASS (11 transitions) |
| mdb toggle gate, 16F505 | same, `MCU=16F505 DEVICE=PIC16F505` | PASS (11 transitions) |
| mdb toggle gate, 16F59 | same, `MCU=16F59 DEVICE=PIC16F59` | PASS (10 transitions) |
| mdb toggle gate, 16F506 | same, `MCU=16F506 DEVICE=PIC16F506` | **FAIL: PORTB bit 0 never changes** |
| SFR map audit | `python3 scripts/sfr-map-audit.py --family PIC16F5x` | clean, all five parts |
| config-key audit | `python3 scripts/config-key-audit.py --family PIC16F5x` | 10 TUs link clean on every supported MCU |
| SFR drift | `python3 scripts/gen-sfr.py --family PIC16F5x --check` | self-skips without a local XC8 (CI covers it) |

So #157 is not "add four parts to the manifest"; that part is done. It is
(a) one real defect on the 16F506, (b) per-part verification runs whose
results nothing in the tree records, and (c) the three hand-kept lists
that give each part an mdb gate in CI.

## 2. The defect: the 16F506 cannot drive RB0

Read back under MPLAB SIM after the family's own blink init, via
`EXTRA_MDB`/`x /1xbr` probes on the sim-variant hex:

| address | register | value | meaning |
|---|---|---|---|
| 0x01 | TMR0 | 84 | the program is running, Timer0 counting |
| 0x06 | PORTB | 0x08 | only RB3 (MCLR/VPP pin) reads high; RB0 never moves |
| 0x08 | CM1CON0 | 0xFF | C1ON = 1: comparator 1 enabled |
| 0x09 | ADCON0 | 0xFC | ANS0/ANS1 set: the analog selects are on |
| 0x0B | CM2CON0 | 0x7F | C2ON = 1: comparator 2 enabled |
| 0x0C | VRCON | 0x00 | reference off |
| 0x05 | OSCCAL | 0x00 | factory calibration not loaded |

DFP proc header (`pic16f506.h`) confirms the block: CM1CON0 0x08,
ADCON0 0x09 (ADON, ANS0, ANS1), ADRES 0x0A, CM2CON0 0x0B, VRCON 0x0C.
The 16F506 is the only part of the five with the comparator/ADC, and on
it those enables come out of reset set: the shared pins are analog, so a
digital write to RB0 is masked. The 16F505 has no comparator/ADC
(`PIC16F5X_FAMILY_HAS_COMP_ADC 0`), which is why it passes the same gate.

This is the family's `HAS_COMP_ADC` macro with nothing behind it, and
`pic16f5x-hal/README.md` currently defers the whole analog surface to a
follow-up. A part that cannot toggle a pin is not onboarded, so the
minimum fix belongs in this ticket; the full comparator/ADC driver API
does not (see §5).

## 3. Design

### 3.1 16F506 analog-off (the only source change)

- `include/pic16f5x_sfr.h`: add `PIC_REG_CM1CON0` 0x08, `PIC_REG_ADCON0`
  0x09, `PIC_REG_ADRES` 0x0A, `PIC_REG_CM2CON0` 0x0B, `PIC_REG_VRCON`
  0x0C under `#if PIC16F5X_FAMILY_HAS_COMP_ADC`, addresses from the DFP
  proc header, same style as the existing port block.
- `src/peripherals/pic16f5x_gpio.c` port configuration: on a
  `HAS_COMP_ADC` part, clear CM1CON0, CM2CON0 and ADCON0 (comparators
  off, all channels digital) using literal `PIC_REG_*` tokens, never a
  computed address (§4.9's first high-risk pattern). This is the same
  shape every classic-midrange family uses to take a shared pin back from
  an analog function.
- `src/sim/pic16f5x_sim.c`: model the mask. While C1ON/C2ON or the ANS
  bits are set, the affected PORTB/PORTC bits ignore digital writes and
  read 0; clearing them restores digital I/O. Without this the host rung
  cannot distinguish a working fix from a masked pin, and §4.2 asks for
  exactly this kind of peripheral stepping in the model.
- `tests/sim_control_probe.c` (the host-only register-image probe) gains
  the `HAS_COMP_ADC` case: assert the exact post-init image
  (CM1CON0/CM2CON0/ADCON0 == 0) and that RB0 toggles afterwards. The
  family smoke `tests/example_blink.c` stays untouched: it is written to
  the 512 W/25 B 16F54 budget and is the manifest example for all five
  parts.
- `MANUAL.md`: register rows for the 506's analog block, the POR-default
  fact with its datasheet citation (DS41236E, the 505/506 datasheet, not
  the 16F54's DS41213D), and what the GPIO init does about it.
- `README.md`: the finding in the family's live-gotcha section, with the
  command that exposed it.

### 3.2 Per-part gates in CI (three hand-kept lists, one entry per part)

The mdb gate is never auto-enumerated; it is three lists that must stay
in sync, and today all three carry 16F54 only:

- `scripts/ci-target-sim.sh`: one `run_one pic16f5x <PART> PIC<PART>
  pic16f5x-hal 5000 toggle "" PORTB <STEPI>` line per part, next to the
  16F54 line at 151. `TOGGLE_REG=PORTB` is forced by the parts: 16F505
  and 16F506 have no PORTA, and PORTB exists on all five.
- `scripts/ci-local-emit.py` `SIM_VARIANTS`: `("pic16f5x-hal", "<PART>")`
  per part.
- `.github/workflows/family-check.yml` inline sim list (line 232): same
  pair per part.

`STEPI=50000` is the measured period for this family's blink at 4 MHz; it
is re-measured per part and only changed if a part's sequence needs it.

Consequence for PR CI: `epic_build.py`'s matrix is canonical-only under
`pull_request` (`CANONICAL["PIC16F5x"] = "16F54"`), so the real-target
XC8 matrix builds only the 16F54 on a PR. The four parts still get a real
XC8 compile on every PR through the mdb gate, which builds the sim
variant with the pinned XC8 before driving MPLAB SIM. The full per-part
real-target matrix runs on push/nightly and locally in §6's regression.
The canonical stays 16F54; changing a shared build-driver contract for
one family's batch is not this ticket.

### 3.3 Per-part data, verified against the DFP before it is trusted

The header's per-part constants are consumed by the host sim and the
docs, and one of them is challenged from outside:

- 16F506 RAM: `pic16f5x_hal.h` says 72 B; epic-cc's catalog says 67 B and
  epic-cc#422 measured a 3-byte common window (0x0D-0x0F) against the
  505's 8. Re-derive flash/RAM/FSR bank bits for all five parts from the
  DFP EDC and the linker script, correct the header, `MANUAL.md` and any
  audit row that disagrees. Record the corrected numbers, not the
  disputed ones.
- Pins: 16F54 18, 16F57 28, 16F59 40, 16F505/506 20, as `MANUAL.md` and
  `sfr-map-audit.py:445` already say. The #151 plan doc on disk claims
  14/14/28 and is stale; it is not ground truth and is deleted at takeoff.
- 16F505/506 `OSCCAL` (0x05) stays a documented register with no driver:
  the family's demo config is XT, and a calibration API is not this
  ticket.

## 4. Commits (one per part, single PR)

1. `feat(pic16f5x-hal): onboard 16F505` - gate lines, docs row.
2. `feat(pic16f5x-hal): onboard 16F506` - analog SFRs, GPIO-init clear,
   sim mask, host assertions, MANUAL/README, gate lines.
3. `feat(pic16f5x-hal): onboard 16F57` - gate lines, docs row.
4. `feat(pic16f5x-hal): onboard 16F59` - gate lines, docs row.
5. `docs(pic16f5x-hal): per-part verification closeout` - MANUAL/README
   tables and the per-part ladder commands, plan doc removed.

## 5. Non-goals (explicit)

- Comparator/ADC **driver API** beyond the analog-off that digital I/O
  needs, and an OSCCAL calibration driver.
- `supported`/`excluded` rows for consumer modules: no `epic-*` module
  classifies PIC16F5x, and every requested part is at least as large as
  the exemplar (16F54: 512 W, 25 B), so §3.2's "smaller variant, check
  the budget" risk does not arise and no sizecheck override is needed.
  The ticket's "sizecheck overrides where modules do not fit" has nothing
  to attach to; this is recorded in the PR body with the numbers.
- `epiccc_sources` growth: epic-cc has `p16f505`/`p16f57` targets, but
  `p16f506`/`p16f59` are documented non-goals upstream (epic-cc#422,
  epic-cc#423) and #423 flags the landed `p16f505`'s banks 2-3 as never
  verified. The family's epic-cc slice stays the p16f54 blink.
- The `epiccc-gate` legs in `ci.yml` stay commented. epic-cc#437 landed
  the p16f54 RAM model, so they can be enabled, but that is its own
  ticket and not this batch. Noted in the PR.
- `docs/adding-a-device.md` drift: §4.6 does not mention `MODE=toggle`,
  and §5.9/§6 still cite `ci-discover-xc8-matrix.py`/`xc8-build.yml`,
  deleted by the CI consolidation. Recorded in the PR body, not fixed
  here.

## 6. Verification (per part, then the §6 regression)

| # | rung | command | pass criterion |
|---|---|---|---|
| 1 | host sim, all devices | `make test MODULE=pic16f5x-hal` then each `build/example_blink_<PART>` and `build/example_control` | exit 0 each |
| 2 | audits | `python3 scripts/sfr-map-audit.py --family PIC16F5x`, `config-key-audit.py --family PIC16F5x`, `gen-sfr.py --family PIC16F5x --check` (with the EDC copied to a host dir, or via CI), `statics-audit.py` | exit 0 |
| 3 | real XC8, per part | `make xc8-build MODULE=pic16f5x-hal MCU=<PART>` | exit 0, exactly one hex |
| 4 | real mdb, per part | `make mdb-test MODULE=pic16f5x-hal MCU=<PART> DEVICE=PIC<PART> MODE=toggle STEPI=50000` plus the §4.7 control-register readback (`EXTRA_MDB='print TMR0'`, `x /1xbr` for the port and the analog block) | PASS, hand-computed register image |
| 5 | §6 regression | `make test`, `make audit`, `make target-ci` (full matrix, every variant, sim gates, bundle gate), `make pre-pr-check TEST=1` | every summary row PASS |

§4 step 6's exit criterion ("host and real mdb both pass") applies per
part; the 16F506 additionally needs its analog registers asserted before
its toggle can be trusted.

## 7. Open questions for approval

1. Include the 16F506 analog-off fix (SFR defines + GPIO-init clear + sim
   mask + host assertions) as the minimum for the part to pass the
   family's own gate? The alternative, gating the 506 on a pin the
   analog default does not hold and documenting the trap, leaves a part
   onboarded whose digital I/O does not work as shipped.
2. Leave `epiccc_sources` and the commented `epiccc-gate` legs alone,
   noting the upstream non-goals in the PR body?
3. Limit data corrections to what the DFP contradicts (16F506 RAM/common
   window), rather than re-deriving all five parts' constants?
