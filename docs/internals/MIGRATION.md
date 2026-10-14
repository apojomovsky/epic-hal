# Migration map: internal docs

Internal design history moved from `docs/` to `docs/internals/` (epic-hal#431).
Each row maps a path that existed on `master` before the move to its new path.
Inbound links from other repositories or old bookmarks can use this table to
redirect. Content is unchanged except for the link and path fixes inside the
moved files, the scripts and the code comments that name them.

| Old path | New path |
|---|---|
| `docs/combo-epiccc-conformance.md` | `docs/internals/combo-epiccc-conformance.md` |
| `docs/demo-scoreboard.md` | `docs/internals/demo-scoreboard.md` |
| `docs/pic18f4550-menu-demo.md` | `docs/internals/pic18f4550-menu-demo.md` |
| `docs/experiments/README.md` | `docs/internals/experiments/README.md` |
| `docs/experiments/math-cycle-benchmark/README.md` | `docs/internals/experiments/math-cycle-benchmark/README.md` |
| `docs/experiments/math-cycle-benchmark/bench.c` | `docs/internals/experiments/math-cycle-benchmark/bench.c` |
| `docs/experiments/math-cycle-benchmark/matrix.sh` | `docs/internals/experiments/math-cycle-benchmark/matrix.sh` |
| `docs/experiments/math-cycle-benchmark/pic16dis.py` | `docs/internals/experiments/math-cycle-benchmark/pic16dis.py` |
| `docs/experiments/math-cycle-benchmark-epiccc/README.md` | `docs/internals/experiments/math-cycle-benchmark-epiccc/README.md` |
| `docs/experiments/math-cycle-benchmark-epiccc/bench_epiccc.c` | `docs/internals/experiments/math-cycle-benchmark-epiccc/bench_epiccc.c` |
| `docs/experiments/math-cycle-benchmark-epiccc/cap.mdb` | `docs/internals/experiments/math-cycle-benchmark-epiccc/cap.mdb` |
| `docs/superpowers/specs/2026-08-22-pic-variants-design.md` | `docs/internals/superpowers/specs/2026-08-22-pic-variants-design.md` |

Not moved: `docs/superpowers/plans/` (both plans stay in place, see
[index.md](index.md#plans)). The takeoff plan gate matches that exact path, so
moving the plans would bypass it.

Also not moved, by design: `docs/layout-budgets.md` (a contributor reference that
`common/manifest/modules.toml` cites), `docs/adding-a-device.md` (the device
playbook), `docs/examples.md`, `docs/getting-started.md`, `docs/reference/`,
`docs/snippets/` and `docs/assets/`.
