# pic8-common

The shared layer reused by every 8-bit PIC HAL family. Nothing here
references a register, a bank, an interrupt vector, or any other detail
that differs between families; that is the whole point. Adding a new
family (PIC16F87XA and PIC18F2455 are both done; see
[../docs/adding-a-device.md](../docs/adding-a-device.md) for the next one)
implements a fixed contract defined here and in each family's own headers,
without re-deriving the status enum, the harness, or the build
boilerplate.

## What lives here

- **`include/core/hal_status.h`** — `HAL_StatusTypeDef` / `HAL_OK` /
  `HAL_ERROR` / `HAL_BUSY` / `HAL_TIMEOUT` / `HAL_INVALID`, and the
  `PIC8_BIT` / `PIC8_BIT_SET` / `PIC8_BIT_CLR` / `PIC8_BIT_TGL` /
  `PIC8_BIT_READ` helpers. Identical on every family.
- **`include/core/pic8_harness.h`** — the four-function test/firmware
  harness contract (`pic8_harness_init` / `_tick` / `_running` / `_log` /
  `_report`) that lets one example source build for the host simulator
  and a real XC8 target with no `#ifdef`. Also declares
  `pic8_dispatch_all_irqs`, the per-family interrupt fan-out each family
  implements under that shared name.
- **`src/core/pic8_harness_target.c`** — the real-target harness
  implementation: four no-ops (the CPU starts itself, time advances on
  its own, firmware runs forever, no stdout). Genuinely family-blind, so
  the same object links against every family. Each family supplies its
  own host `*_harness_sim.c` that pumps its own simulator.
- **`cmake/pic8_family.cmake`** — shared CMake helpers
  (`pic8_add_hal_library`, `pic8_add_example`, `pic8_add_example_per_device`)
  so a family's `CMakeLists.txt` is a thin caller.
- **`mk/pic8_family.mk`** — shared Makefile fragment (the `.c` -> `.p1`
  p-code pattern rule, the generated config-word translation unit, the
  single `.hex` link step, and clean) so each family's XC8 Makefile only
  states what is family-specific.

## What does NOT live here

Anything register-specific: SFR maps, bank/BSR addressing, the
`platform.h` SFR-access spelling, the IRQ enum and vector layout,
peripheral driver bodies, config-word directives. Those stay in each
family's tree (`pic16f87xa-hal/`, `pic18fxx5x-hal/`, …), implementing the
contract this layer defines. See
[../docs/multi-family-plan.md](../docs/multi-family-plan.md) for the full
design and the per-phase plan, and
[../docs/adding-a-device.md](../docs/adding-a-device.md) for the operational
guide to adding another device or family.
