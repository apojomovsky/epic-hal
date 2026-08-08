# epic-math: pin pic16_mscratch to fix a real PIC16F876A/877A link failure

Status: **implemented 2026-08-08, verified in the CI Docker toolchain
(`ghcr.io/apojomovsky/pic8-hal-ci:xc8-v4.00-dfp1.7.162-1.7.171-1.9.258-mplabx6.35`)**.

## Verification outcome (2026-08-08)

- PIC16F877A reproduced the quoted fixup overflow byte-for-byte with the
  unpinned buffer (`bssBANK1 (0xA2)`, `pic16_isr_vector.c:30`, spanning
  `0xA0-0xAD`). PIC16F876A linked even unpinned in the current tree (one
  static of allocator margin), same mechanism, same cure.
- All four PIC16F87XA variants now link in the CI toolchain:
  `16F876A` (1262 words flash, 108 B data), `16F877A` (1287/109),
  `16F873A` (1174/108), `16F874A` (1194/109).
- The 16F873A/16F874A manifest `excluded` entries (root cause 3 in
  `docs/mplabx-link-gaps-plan.md`, stale even without this fix: control
  builds of the unpinned buffer linked at 62% RAM) were removed and the
  parts added back to `supported`, per that document's own procedure.
- Host-sim: all 8 ctest tests pass unchanged.
- Known cost of the chosen address, accepted per the disclosure below:
  the epic-math selftest image (`hal = true`) links the HAL core, so
  every PIC16 build emits XC8 warning 1482 (`_epic_irq_pie_scratch`,
  `_epic_bank1_scratch`, and XC8's own `btemp`/`wtemp` at `0x7E`/`0x7F`
  overlap the 16-byte window). Runtime-benign in the selftest by
  construction: no HAL macro touching those bytes runs there, and every
  math routine rewrites its working bytes (offsets 0-7) before reading
  them. Constraint recorded in `pic_math_scratch.c`: offsets 8-15 must
  stay unused (they cover XC8's temporaries).

## Problem

`epic-math/src/pic16/pic_math_scratch.h:16` declares
`extern volatile uint8_t pic16_mscratch[16];` with no `__at()` address
pin. Every hand-written PIC16 inline-assembly math routine
(`pic_math_div.c`, `pic_math_mul.c`, `pic_math_addsub.c`,
`pic_math_bcd.c`) references it directly by symbol, entering each
asm block with `asm("banksel _pic16_mscratch");` before touching
offsets within it.

Being unpinned, XC8's linker places it via best-fit allocation. On
PIC16F873A/874A (192 bytes RAM), it lands in Bank 0 and everything
works. On PIC16F876A/877A (368 bytes RAM, more other statics pushing
the allocator further), it lands at `0xA0`, inside Bank 1. Building
epic-math's real-target self-test for either of those two devices then
fails with a real XC8 linker error:

```
pic16f87xa-hal/src/core/pic16_isr_vector.c:30:: error: (1356) fixup
overflow referencing psect bssBANK1 (0xA2) into 1 byte at 0x54A/0x2
-> 0x2A5 (build/epic-math/16F876A-math-selftest.o ...)
```

repeated roughly 30 times across the `0xA0`-`0xAD` range, exactly
`pic16_mscratch`'s own footprint. Confirmed reproducible twice in the
exact CI Docker toolchain
(`ghcr.io/apojomovsky/pic8-hal-ci:xc8-v4.00-dfp1.7.162-1.7.171-1.9.258-mplabx6.35`),
not a local-toolchain artifact, and confirmed already present on
`origin/master`, not introduced by any other work this session.

This is exactly the class of bug `AGENTS.md` documents by name: "The
linker scatters unpinned static by best-fit, not declaration order,
pin anything bank-sensitive with `__at(addr)`." `pic16_mscratch` is
precisely this kind of symbol and was simply never pinned.

## Decision

Pin `pic16_mscratch` into PIC16 mid-range's common RAM, addresses
`0x70`-`0x7F` (16 bytes), via `__at(0x70)`. Common RAM is a real
hardware feature of this chip generation: the same physical addresses
regardless of which bank is currently selected, so code touching it
never needs a `banksel` at all. One address works identically across
all four PIC16F87XA variants (873A/874A/876A/877A); common RAM's size
and location are architectural constants of this family, not
per-device values, unlike `PIC16F87XA_FAMILY_RAM_BYTES`.

**A real, disclosed tradeoff, not an unexamined assumption.** This
repo already has a working precedent for common-RAM pinning:
`pic16f87xa-hal/src/core/pic16_isr_vector.c` pins
`epic_irq_pie_scratch __at(0x70)` and `epic_bank1_scratch __at(0x71)`
for an unrelated, already-merged `epic-swuart` fix. `pic16_mscratch`
needs the entire 16-byte common RAM window, including those same two
addresses. There is no manifest dependency between `epic-math` and
`epic-swuart` (confirmed by reading `epic-common/manifest/modules.toml`),
but the two scratch bytes live in the **HAL core**, which epic-math's
own selftest links (`hal = true`), so the overlap is present in this
repo's builds, not just manual combinations: XC8 emits warning 1482 for
`_epic_irq_pie_scratch`/`_epic_bank1_scratch` in every PIC16 epic-math
build. The selftest never writes those bytes concurrently with a math
call, so this is a documented hazard for user firmware (a PIE-enable or
bank1-SFR macro running while a math routine is mid-computation
corrupts `pic16_mscratch[0]`/`[1]`), same class as the already-
documented interrupt re-entrancy limitation, not a defect in the
selftest. One further occupant of the window the design did not
anticipate: XC8's own `btemp`/`wtemp`/`btemp1` temporaries at
`0x7E`/`0x7F`. Safe because the asm routines use only offsets 0-7;
offsets 8-15 are now off-limits for future routines (recorded in
`pic_math_scratch.c`). Documented as a known, narrow limitation, not
fixed here.

## Testing

Real-target: rebuild all four PIC16F87XA variants
(873A/874A/876A/877A) in the actual CI Docker toolchain, the same
discipline that found the bug in the first place, not a local build.
876A/877A must now link successfully; 873A/874A were found to build
already (their manifest `excluded` entries were stale) and are added
back to `supported` so CI covers all four.

Host-sim: the existing math test suite (`epic-math`'s CMake/ctest
build) must pass unchanged, since this only changes a target-side
declaration (the host build has no `__at()` mechanism and no bank
concept at all), not any arithmetic behavior.

## What this design deliberately does not do

- Change any math routine's logic or its hand-written assembly body.
  Only the storage declaration for `pic16_mscratch` changes.
- Resolve the disclosed common-RAM collision with `epic-swuart`'s own
  scratch registers. Narrow, currently harmless (no manifest link
  between the two modules), documented rather than engineered around.
- Address PIC18Fxx5x or PIC16F193X. This bug and fix are specific to
  PIC16F87XA's classic mid-range banking model; the other two families
  have different addressing architectures entirely (PIC18's flat
  access bank, PIC16F193X's BSR) and were not found to have this
  problem.
