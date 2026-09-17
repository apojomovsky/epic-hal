# Design note: scoped cppcheck include sets in the pre-commit hook (epic-hal#216)

Status: pending approval (design gate). Ephemeral plan, deleted before merge.

## Problem, reproduced

`cppcheck_check()` in `scripts/pre-commit-checks.sh` analyzes all staged
`.c` files in one invocation whose include path is every directory named
`include`, `include/host` or `include/target` in the tree: **124 `-I`
dirs** on today's master. Headers that share a basename across families
then resolve to whichever directory sorts first:

- `pic14_midrange.h` / `pic14_midrange_sfr.h`: one copy per midrange
  family (628a, 63x_67x_68x, 7x, 818_819, 83_84, 87xa, 88x), each with
  that family's register addresses.
- `epic_hal.h`, `hal_gpio.h`, `hal_adc.h`, ...: one copy per family.

The ticket's evidence stands as written: with the flattened list, the
818_819 EEPROM example's `EPIC_REG8(PIC_REG_EEADR)` read lands in
`pic16f5x_sim_sfr[256]` at index 269 and cppcheck reports
`arrayIndexOutOfBounds`, and the same class of finding fires on
`pic16f87xa-hal/tests/example_eeprom.c`, a file not even in the diff.

## Design

Partition the staged `.c` files by their **top-level directory**, and run
one cppcheck invocation per partition with that partition's own include
set. Same binary, flags, suppressions and `-D'__at(x)='` as today; only
the `-I` list and the 1-to-N invocations change.

| partition | include set |
|---|---|
| `pic*-hal` (family HAL) | that family's `include`, `include/host`, `include/target` (the ones that exist) + `epic-common/include` + `pic14-midrange-core/include` (its `pic14_*.h` basenames are unique repo-wide, so they cannot shadow a family header; family sources include them directly) |
| `pic14-midrange-core` | its own `include` + `epic-common/include` + the include dirs of the alphabetically-first family that provides `include/pic14_midrange.h` (`pic16f628a-hal`) |
| everything else (`epic-*`, `tests/`, `examples/`, root) | `epic-common/include` + every `epic-*/include` + the include dirs of the alphabetically-first family that provides `include/epic_hal.h` |

Rationale per rule:

- A family file is family-specific, so its own headers are the correct
  resolution for every basename it uses, including `pic14_midrange.h`.
  Cross-family dirs are simply absent, so the mixing mechanism is gone
  rather than reordered.
- `pic14-midrange-core`'s drivers include the family-blind
  `pic14_midrange.h`, which only families provide. They are analyzed
  against one family's register map, deterministically chosen. The
  per-family truth stays with the family builds and ctest; the hook is a
  logic-bug net, and deterministic is what the ticket asks for.
- Consumer modules (`epic-bus` includes `epic_hal.h`; its `sim_bus.c`
  guards two families' platform headers behind `#if defined(...)`) get
  the same treatment: one family's surface, chosen deterministically,
  plus the family-free module headers. Headers pulled in only under a
  per-family guard resolve nowhere without a family define (suppressed,
  as before for genuinely missing headers); the analyzable surface is
  the family-agnostic logic, whose per-family truth stays with the
  builds. No family `-D` is passed to change that: it would contradict
  the hook's no-defines rule and risk `#error` paths the builds own.

The deterministic family picks are constants in the script with a
comment stating why they exist, so a future family rename edits one
place.

Runtime: one cppcheck process per represented partition (a typical
change touches 1-2 partitions), not per file; the ticket explicitly
allows slower.

## Verification (acceptance)

1. Reproduce the two evidence findings with the current flattened
   invocation (cppcheck 2.13, the toolchain/CI spelling).
2. After the fix: both files clean under their partitions' include sets.
3. Plant an out-of-bounds `EPIC_REG8` read above `0xFF` in a family
   file, confirm cppcheck reports it against **that family's own array**,
   then remove the plant.
4. Time the hook on a 4-file change before/after; report the numbers in
   the PR.
5. Simplify the workaround comment in
   `pic16f818_819-hal/tests/example_eeprom.c` (assertions unchanged, per
   the ticket).

## Prerequisite: local cppcheck

`cppcheck` is in the repo's bootstrap package set but is not installed
on this host (no passwordless sudo), and neither container image carries
it (probed `epic-hal-toolchain:local` and `epic-cc-dev:local`). Two
ways to prove acceptance locally:

- you run `sudo apt-get install -y cppcheck` (also unblocks `make
  bootstrap`'s other missing packages), or
- I extract the distro `cppcheck` package into `~/.local` without root
  and put it on `PATH` for verification only (nothing committed).

Alternatively the acceptance runs land in CI (the `host` job runs this
hook with cppcheck 2.13), with the planted-OOB proof being one red CI
run on a pushed commit before the plant is removed.

## Non-goals

- No per-file invocation (per-partition is enough and faster).
- No cppcheck configuration files, no new suppressions, no change to
  the existing suppression list or reasons.
- No change to the flagged 818_819 assertions (ticket: the file needs
  no further change beyond the comment simplification).
