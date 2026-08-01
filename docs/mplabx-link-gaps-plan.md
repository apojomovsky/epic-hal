# `mcu/*-mplabx/Makefile` link gaps: real bugs found by first real CI, fix plan

Status: **identified, not started**. Found by `xc8-build.yml` (Phase 1 of
`docs/ci-plan.md`)'s first fully-working run
(https://github.com/apojomovsky/pic8-hal/actions/runs/30719090416): 40 of
112 `(module, MCU)` build legs failed, all with real XC8 compiler/linker
errors, not CI plumbing. Excluded from `xc8-build.yml`'s matrix for now
(`scripts/ci-discover-xc8-matrix.py`'s `KNOWN_BROKEN` list) so Phase 1
could land green; this document is where that exclusion list's debt
actually gets paid down.

## Why this exists

None of these `mcu/*-mplabx/Makefile`s had ever actually been linked
against real XC8 before this CI effort. `pic16f87xa-hal`'s and
`pic18fxx5x-hal`'s own top-level Makefiles had (informally, by whoever
last touched them locally), and both link fine. Every per-module
`pic8-*/mcu/*-mplabx/Makefile` (a `HAL_SOURCES` subset picked to build
just enough of the HAL for that module's example, plus config-word
generation, same pattern as the two HALs) had not, until
`xc8-build.yml` actually ran `make` for every declared MCU variant of
every one of them. Two distinct, real bugs surfaced.

## Root cause 1: missing peripheral sources vs. the IRQ dispatch contract

`pic16_irq_dispatch.c`'s header comment states the design explicitly:
it takes **strong** references to every peripheral's `_IRQHandler`
(not through the peripheral's own header, which would make it a *weak*
reference), specifically so the linker is forced to pull every handler
in. Quoting the file:

> The handlers are declared `PIC8_WEAK` in their own headers (to allow
> optional user override). That makes a reference through those headers
> a *weak* reference, which the linker will NOT use to pull the
> handler's object out of the static library, leaving the call target
> NULL for any handler not already pulled by a strong reference. To
> force the linker to resolve every handler, this file declares them
> with strong prototypes and does not include the peripheral headers.
> (On the XC8 target there is no weak attribute, so this is a no-op
> there; it only matters for the host link.)

The consequence, not spelled out there: **any Makefile that links
`pic16_irq_dispatch.c` (or PIC18's `pic18_irq_dispatch.c`) must compile
every peripheral source that defines one of those handlers, even if the
module using it only exercises one peripheral.** A partial `HAL_SOURCES`
list plus the dispatch file is a guaranteed real-target link failure,
`error: (2096) undefined symbol "_TIMER1_IRQHandler"` and similar, one
per missing peripheral.

**Affected (all MCU variants, both families where applicable, this is
not MCU-specific, it's a fixed set of missing translation units)**:

- `pic8-console/mcu/pic16f87xa-console-mplabx` and
  `pic8-console/mcu/pic18fxx5x-console-mplabx` (only compiles USART)
- `pic8-settings/mcu/pic16f87xa-settings-mplabx` and
  `pic8-settings/mcu/pic18fxx5x-settings-mplabx` (only compiles EEPROM)
- `pic8-taskmgr/mcu/pic18fxx5x-taskmgr-mplabx` **only** (only compiles
  GPIO + Timer0; the PIC16 side already compiles the full peripheral
  set and links fine)

**Fix**: add the missing peripheral `.c` files to each Makefile's
`HAL_SOURCES`, matching what `pic16f87xa-hal/mcu/pic16f87xa-mplabx/
Makefile` (or the PIC18 equivalent) already compiles, the two Makefiles
that do link successfully. Before doing this at scale, worth deciding
whether `pic16_irq_dispatch.c`'s "always require every handler" design
is actually the right contract for a module that only cares about one
peripheral, an alternative is a per-module or per-target dispatch that
only calls the handlers actually compiled in, but that's a bigger
design change than patching `HAL_SOURCES` lists, flagged here, not
decided.

## Root cause 2: real RAM/resource overflow on the smaller MCU variants

Separate from root cause 1, these modules compile and link fine on the
larger part in each family but run out of RAM on the smaller one, a
genuine XC8 diagnostic (`error: (1360) no space for auto/param`, or
`error: (1250) could not find space (N bytes) for variable ...`), not a
missing-symbol problem.

**Affected (only the smaller variant(s) in each family, the larger ones
build fine)**:

| Module | PIC16 fails on | PIC18 fails on |
|---|---|---|
| `pic8-bus` | (builds fine) | 18F2455, 18F2550 |
| `pic8-debounce` | 16F873A, 16F874A | 18F2455, 18F2550 |
| `pic8-encoder` | (builds fine) | 18F2455, 18F2550 |
| `pic8-math` | (builds fine) | 18F2455, 18F2550 |
| `pic8-modbus` | 16F873A, 16F874A | 18F2455, 18F2550 |
| `pic8-serial` | 16F873A, 16F874A | 18F2455, 18F2550 |
| `pic8-tick` | (builds fine) | 18F2455, 18F2550 |

Reproduced locally (`pic8-debounce`, `MCU=16F874A`):

```
../../../pic16f87xa-hal/src/core/pic16_irq.c:56:: error: (1360) no space for auto/param main@db_b
```

and (`pic8-serial`, `MCU=16F873A`):

```
../../src/pic8_serial.c:51:: error: (1250) could not find space (32 bytes) for variable _g_rx_buf
```

**Fix**: needs a real decision, not just a patch, this is either (a) the
module's actual RAM footprint (with XC8's free-tier optimization level,
see `docs/ci-plan.md`'s open question on the v3.10-vs-v4.00 pin) doesn't
fit the smaller part and never did, in which case the smaller variants
should be dropped from that module's supported `MCU=` list rather than
silently listed as supported, or (b) something in the build (unused
peripherals still being compiled in, stack usage in a shared path) is
avoidably wasting RAM and can be trimmed. Distinguishing (a) from (b)
needs looking at each module's actual data/stack usage, not assumed here.

## Next steps

1. Decide, per module, whether root cause 2's smaller-variant failures
   are "never supported, fix the documented MCU list" or "fixable RAM
   waste, fix the build." Not obviously the same answer for every module
   in the table above.
2. Fix root cause 1 by completing each affected Makefile's `HAL_SOURCES`
   (or revisit the dispatch contract first, see above).
3. As each module gets fixed, remove its entries from
   `scripts/ci-discover-xc8-matrix.py`'s `KNOWN_BROKEN` list so
   `xc8-build.yml` starts actually covering it again, that list shrinking
   to empty is this document's exit criterion.
