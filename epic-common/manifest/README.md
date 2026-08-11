# `modules.toml`, the build manifest

One declarative description of what every module needs and where it
builds. Three consumers read it and nothing else encodes the knowledge:

- `scripts/epic_build.py`, the real-target build driver
- `scripts/epic_build.py matrix`, which feeds `.github/workflows/ci.yml`'s
  `target` job
- `scripts/make_bundle.py`, which generates release bundles

## Path conventions

Two roots, deliberately:

- **Family-level** paths (`hal_sources`, `includes`,
  `conditional_sources.path`) are **repo-root-relative**, because family
  data spans directories.
- **Module-level** paths (`sources`, `sources_by_family`, `includes`,
  `example.sources`) are relative to that module's own `dir`, so a module
  entry stays short and the module can be moved without rewriting it.

## Families

```toml
[families.PIC16F87XA]
hal_dir  = "pic16f87xa-hal"
variants = ["16F873A", "16F874A", "16F876A", "16F877A"]
dfp      = "Microchip.PIC16Fxxx_DFP"
fosc_hz  = 20000000
includes = ["pic16f87xa-hal/include/target", "pic16f87xa-hal/include",
            "epic-common/include"]
hal_sources = ["pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c"]
```

`includes` order is significant and preserved verbatim:
`include/target` must precede `include` so the platform header resolves
to the real-target (volatile-dereference) version rather than the host
memory-backed one.

`fosc_hz` is the family's oscillator frequency, family-uniform, taken
from the Makefiles' own `FOSC_HZ ?=` defaults. The build driver uses it
for `-DFOSC_HZ`; `--fosc-hz` overrides it for a different crystal.

`hal_sources` is the full peripheral set for the family. It is not
trimmed per module: `pic16_irq_dispatch.c` takes strong references to
every peripheral's `_IRQHandler` specifically to force the linker to
resolve them all, so a partial set is a guaranteed link failure. This
is the "link: irq dispatch needs every peripheral handler" failure
class named in the manifest's `excluded` reason strings.

A source only some variants compile is a conditional:

```toml
[[families.PIC16F87XA.conditional_sources]]
path     = "pic16f87xa-hal/src/peripherals/pic16f87xa_psp.c"
variants = ["16F874A", "16F877A"]   # PSP is 40/44-pin only
```

## Modules

```toml
[modules.epic-modbus]
dir        = "epic-modbus"
sources    = ["src/epic_modbus.c"]
includes   = ["include"]
depends_on = ["epic-serial", "epic-tick"]
```

`depends_on` is resolved transitively, so a consumer naming `epic-modbus`
gets `epic-serial` and `epic-tick` automatically. Cycles are rejected at
load time.

### `needs_hal`: not every module uses the family HAL

```toml
[modules.epic-math]
needs_hal  = false
```

Whether the family HAL is needed is a property of the **program**, not
the module. `epic-math`'s library sources touch no HAL symbol, but its
smoke-test example includes the harness, so its build compiles the whole
HAL. `needs_hal` is therefore `false` for the four modules whose library
is HAL-independent: `epic-adcfilter`, `epic-fsm`, `epic-pid`, `epic-math`.
A build whose effective `hal` is `false` gets no family HAL sources, no
conditional sources, and no family includes, only the module's own and
its dependencies'.

### `sources_by_family`: a module's sources can differ per family

```toml
[modules.epic-math.sources_by_family]
PIC16F87XA = ["src/pic16/pic_math_mul.c", "src/pic16/pic_math_scratch.c"]
PIC18Fxx5x = ["src/pic18/pic_math_mul.c"]
```

`epic-math` compiles a `src/pic16/` inline-asm backend on PIC16 and a
`src/pic18/` one on PIC18, over a shared `src/common/`. The family-blind
`sources` list holds what every family links (the common files); the
per-family table holds the backend. The build concatenates `sources`
then `sources_by_family[family]`.

## Supported and excluded parts

```toml
[modules.epic-serial.supported]
PIC16F87XA = ["16F876A", "16F877A"]
PIC18Fxx5x = ["18F4455", "18F4550"]

[modules.epic-serial.excluded]
"16F873A" = "RAM: 32-byte g_rx_buf does not fit (XC8 error 1250)"
```

Every part of a family must appear in exactly one of the two, and the
loader rejects a part listed in both. A module that builds on no part of
a family simply omits that family from `supported` (see `epic-modbus`,
which has no `PIC16F87XA` entry). `excluded` reasons are user-facing:
they are printed by the build driver and, in a later plan, by the
generated `epicurus.mk` and `SUPPORT.md`.

This pair replaces the `KNOWN_BROKEN` literal that used to live in
`scripts/ci-discover-xc8-matrix.py`. The exit criterion for the
link-gaps work is now "no `excluded` entries remain."

## Examples

Examples are keyed by family, because a module's example program can
differ per family (`epic-math` ships a PIC16 smoke test and a PIC18
self-test), and so can its config words:

```toml
[modules.epic-taskmgr.example.PIC16F87XA]
name    = "multi-blink"
sources = ["examples/example_multi_blink.c"]
config  = { FOSC = "HS", WDTE = "ON" }
```

`name` becomes the `.hex` basename: `build/16F877A-multi-blink.hex`. The
`config` table generates the `#pragma config` translation unit, which is
why the pragmas are per family: PIC16 has one configuration word, PIC18
has several with unrelated fields. An example with **no `config` table**
generates **no** config translation unit and adds no config object to
the link, matching the sizecheck programs that carry no config words.

An example may override the module's `needs_hal` with its own `hal`:

```toml
[modules.epic-math.example.PIC16F87XA]
hal     = true              # the smoke test uses the harness
```

`hal` defaults to the module's `needs_hal`.

### Where the example program lives

Not always under `examples/`. The sizecheck and self-test programs live
under `mcu/` and `tests/`. The bootstrap classifies them mechanically:
`APP_SOURCES` when the Makefile defines it, otherwise any owned `SRCS`
entry outside the module's own `src/`. Everything under `src/` is a
library source; everything else the module owns is its program.

## Adding a module

1. Add a `[modules.<name>]` table with `dir`, `sources`, `includes`,
   `depends_on`, and `needs_hal` (omit for the common `true` case).
2. Add `sources_by_family` if the module has per-family backends.
3. List the parts it builds on under `supported`, and every other part
   of those families under `excluded` with a reason.
4. Add a per-family `example` table for each family it ships a program
   on; set `hal = true` if the example needs the HAL but the library
   does not.
5. `python3 scripts/epic_build.py build --module <name> --mcu <part> --run`
6. Nothing else. CI's matrix picks it up automatically.

## Adding a part to an existing family

Add it to that family's `variants`, then add it to every module's
`supported` or `excluded`. The loader fails until every module has
classified it, which is intentional: a new part should not silently
appear supported everywhere.
