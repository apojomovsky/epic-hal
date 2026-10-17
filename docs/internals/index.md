# Internals

This directory holds epic-hal's design history: the experiment write-ups,
measurement records and design specs behind decisions the code relies on.
It is for maintainers who want to know why something is the way it is.
User-facing material (the [README](../../README.md), the
[getting started guide](../getting-started.md), the
[reference pages](../reference/modules.md) and the
[device playbook](../adding-a-device.md)) stays in `docs/` and in each module.

Paths that moved here are listed in [MIGRATION.md](MIGRATION.md).

## Experiments

Self-contained probes, each with its own README recording the question, the
method, the results and the conclusion. See the
[experiments index](experiments/README.md) for the full list.

- [math-cycle-benchmark-epiccc](experiments/math-cycle-benchmark-epiccc/README.md):
  the lib/math C path under epic-cc, cycles per operation.

## Measurement records

Dated comparisons of XC8 and epic-cc output on the supported parts.

- [combo-epiccc-conformance](combo-epiccc-conformance.md): which
  `epic-combo-*` firmwares build and gate under epic-cc, with RAM and flash
  headroom per combo.
- [demo-scoreboard](demo-scoreboard.md): one table for the three PIC18F4550
  flagship demos, XC8 against epic-cc.
- [pic18f4550-menu-demo](pic18f4550-menu-demo.md): the dual-toolchain
  comparison of the menu demo, size and UART trace.

## Design specs

- [PIC variant support](superpowers/specs/2026-08-22-pic-variants-design.md):
  device registry, SFR header generation and canonical-per-core CI for the
  HAL.

## Plans

Implementation plans are ephemeral and are removed when the work lands (see
the takeoff rule in [AGENTS.md](../../AGENTS.md)). The two current plans
still live under [docs/superpowers/plans/](../superpowers/plans/), outside
this directory, until the plan-deletion policy for them is settled.

- [HAL: device registry and SFR header generation](../superpowers/plans/2026-08-22-pic-variants-hal-registry-plan.md)
- [HAL: add the PIC16F887 board](../superpowers/plans/2026-08-22-pic-variants-hal-887-plan.md)
