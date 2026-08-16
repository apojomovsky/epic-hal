# Reference MPLAB X projects

One per family. Each is a minimal demo that proves a bundle is wired up
correctly: initialise the HAL, toggle a pin, loop.

| Project | Family | Device it ships configured for |
|---|---|---|
| `epic-hal-demo-pic16f87xa.X` | PIC16F87XA | PIC16F877A |
| `epic-hal-demo-pic18fxx5x.X` | PIC18Fxx5x | PIC18F4550 |
| `epic-hal-demo-pic16f193x.X` | PIC16F193X | PIC16F1937 (HAL only) |

`scripts/make_bundle.py` copies the matching one into each bundle as
`examples/epic-hal-demo.X`, so inside a bundle the project sits one level
below the bundle root and every path in it resolves as `../..`.

These `.X` projects are also the base templates the `epic-hal init`
scaffolder patches: `scripts/epic_hal_init.py` copies the matching
reference project and rewrites its part and module set, so a scaffolded
project starts from a known-good reference instead of a blank one.

Three projects, not one per module. `nbproject` XML is generated, pins
DFP versions, and rots; mirroring the old per-module `mcu/` layout would
mean 29 of these. If you find yourself wanting a fourth, add a
configuration to an existing project instead.

To change which device a project targets, use Project Properties rather
than editing `nbproject/configurations.xml` by hand.

Each project links every module folder that family supports (matching
what a real bundle ships), even though `main.c` only exercises one or
two of them: the HAL's interrupt dispatch takes strong references to
every peripheral handler, so a partial source set will not link. See
each family's `MPLABX.md` (generated into a bundle by `make_bundle.py`)
for the authoritative source/include list.

Verified buildable headlessly in the toolchain container (`make shell`,
then `prjMakefilesGenerator.sh` + `make -f nbproject/Makefile-default.mk
SUBPROJECTS= .build-conf` from inside each `.X` directory).
