# PIC16F193X higher-level module rollout, design

Status: **agreed 2026-08-06, not started**. Roadmap only; each module
gets its own `docs/superpowers/plans/<date>-pic16f193x-<module>.md`
written and executed one at a time, confirmed with the user before each,
same discipline `docs/pic16f193x-plan.md` §7 used for the 13 peripherals.

## Problem

Every `epic-*` higher-level module (`epic-tick`, `epic-serial`,
`epic-debounce`, `epic-encoder`, `epic-modbus`, `epic-console`,
`epic-settings`, `epic-taskmgr`, `epic-math`, `epic-pid`, `epic-fsm`,
`epic-adcfilter`, `epic-bus`) is wired up for PIC16F87XA and PIC18Fxx5x
in `epic-common/manifest/modules.toml`. None reference `PIC16F193X`. The
family's own entry (`modules.epic-pic16f193x-firmware`) only builds a
bare-HAL smoke example; nothing exercises the family through an actual
application-level module. This is a real, large gap, not a documentation
lag: the HAL itself is complete (all 13 peripherals landed, see
`docs/pic16f193x-plan.md`), so the blocker for each module is a genuine
port, not a missing peripheral driver in the common case.

Two modules are excluded, one permanently:

- **`epic-usb`**: no USB peripheral exists on this family's silicon
  (confirmed in `epic-usb/README.md`: "PIC18Fxx5x-only... no PIC16
  backend and never will be. Not a scope choice, a hardware
  constraint."). Out of scope forever, not just for this rollout.
- **`epic-sdcard`**: needs a contiguous 512-byte `MMC_BLOCK_SIZE`
  buffer. PIC16F1937/1939 have 512 bytes of RAM total, so even those
  variants would need the block to be the *only* thing in RAM; 1933/
  1934/1936/1938 (256 bytes) cannot fit it at all. Marginal at best,
  needs its own feasibility check before a plan doc is written for it,
  not assumed solvable here.

## Decisions

| Question | Decision |
|---|---|
| Unit of work | One module per plan doc, executed and gated individually |
| Order | Dependency order from `modules.toml`'s `depends_on`, foundational modules first |
| Gate | Same as any other module addition: host `ctest`, real XC8 build via the manifest, `HARNESS=sim` build where the module already has one for the other families |
| `epic-usb` | Excluded permanently, hardware fact |
| `epic-sdcard` | Deferred; feasibility (RAM fit on 1937/1939 only) checked before a plan doc is written |
| `epic-console` / `epic-settings` | Included, but landed last: both are currently excluded on *every* variant of both existing families for a link reason (`docs/mplabx-link-gaps-plan.md` root cause 1) that may not apply to PIC16F193X (its manifest entry already compiles the full `hal_sources` set unconditionally, which is the fix that root cause needed elsewhere). Worth checking as a possible fix location, but not assumed until verified. |

## Module order and per-module notes

| # | Module | Depends on | HAL readiness | Real work |
|---|---|---|---|---|
| 1 | `epic-fsm` | none | `needs_hal=false`, pure logic | Near-zero: add to `supported`, add an example. Warm-up, proves the manifest-only (no HAL glue) path. |
| 2 | `epic-tick` | none | Timer0/Timer1 already landed | Family-specific tick source selection, mirrors the other two families' `epic_tick.c` backend split. |
| 3 | `epic-serial` | none | EUSART already landed | Family-specific ring-buffer wiring to `pic16f193x_usart`. |
| 4 | `epic-math` | none | `needs_hal=false`, but has `sources_by_family` | Real: new enhanced-mid-range assembly for addsub/bcd/div/mul (49-instruction ISA, not a copy of the PIC16 classic or PIC18 asm; XC8 inline-asm rules per `epic-math/docs/ARCHITECTURE.md`). |
| 5 | `epic-debounce` | `epic-tick` | GPIO + tick, both available | Should be close to a straight port once #2 lands. |
| 5 | `epic-encoder` | `epic-tick` | GPIO + tick, both available | Same shape as `epic-debounce`. |
| 6 | `epic-pid` | `epic-math` | `needs_hal=false` | Straight port once #4 lands. |
| 7 | `epic-adcfilter` | none | ADC already landed | Straight port. |
| 7 | `epic-bus` | none | MSSP already landed | Straight port. |
| 8 | `epic-modbus` | `epic-serial`, `epic-tick` | covered by #2, #3 | Port once both land. |
| 9 | `epic-taskmgr` | none | GPIO + Timer, available | Check whether PIC18's exclusion reason (root cause 1) actually applies here; may be a straight port. |
| 10 | `epic-console` | `epic-serial` | covered by #3 | Currently excluded everywhere; investigate root cause 1's applicability first. |
| 10 | `epic-settings` | none (EEPROM) | EEPROM already landed | Same investigation as `epic-console`. |
| 11 | `epic-sdcard` | none | MSSP + tick available, RAM marginal | Feasibility check before a plan doc; likely 1937/1939-only if it proceeds at all. |
| - | `epic-usb` | n/a | n/a | Excluded permanently, no hardware. |

Rows sharing a number (`epic-debounce`/`epic-encoder`, `epic-adcfilter`/
`epic-bus`, `epic-console`/`epic-settings`) have no dependency on each
other and can land in either order within that step.

## Per-module process

Each module's plan doc follows the same shape as the peripheral plans
under `docs/superpowers/plans/2026-08-04-pic16f193x-*.md`:

1. Add the module's entry to `epic-common/manifest/modules.toml`
   (`supported`, `example.PIC16F193X`, and `.sim` variant if the module
   has one for the other families).
2. Port any family-specific source (tick backend, math asm, etc.).
3. Host build + `ctest` for the family.
4. Real-target XC8 build via `epic_build.py` / `make xc8-build`.
5. `HARNESS=sim` build + `mdb` run where the module already has a sim
   variant elsewhere, following the same `MODE=gpio` PASS-marker
   protocol the peripheral plans used.
6. Update `epic-common/manifest/README.md` / the module's own
   `README.md` if its supported-family table is documented there.
7. Commit per module (Conventional Commits, `feat(pic16f193x): <module>
   port` shape).

## Testing

Same as any other manifest-driven module addition: `epic_build.py`
resolves the family's `hal_sources` plus the module's own sources, no
new build machinery. `scripts/ci-discover-xc8-matrix.py` and
`bundle-gate.yml`/`xc8-build.yml`/`sim-tests.yml` pick up new
`(module, MCU)` legs automatically once `modules.toml` lists them,
consistent with how the peripheral roadmap's Timer1/Timer2/4/6 landings
were picked up.

## What this design deliberately does not do

- Decide `epic-sdcard`'s fate. That is its own feasibility check.
- Fix `docs/mplabx-link-gaps-plan.md` root cause 1 for the other two
  families. If the PIC16F193X port of `epic-console`/`epic-settings`
  reveals the fix, that is a bonus, not this design's goal.
- Touch the `examples/epicurus-demo-pic16f193x.X` MPLAB X project from
  `docs/superpowers/plans/2026-08-07-mplabx-projects-and-release.md`
  Task 2. That plan's PIC16F193X project is explicitly HAL-only and
  stays that way regardless of this rollout's progress, since it is
  already in flight on a separate track.
- Implement anything. This document is the roadmap; execution is
  per-module plan docs, written and run one at a time.
