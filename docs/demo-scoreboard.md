# Demo scoreboard: XC8 vs epic-cc on the 18F4550 flagships (epic-hal#279)

Status: 2026-09-23. One table for the three product-shaped PIC18F4550
demos: menu (`epic-menu-demo`), control (`epic-control-demo`,
epic-hal#277, PR #281), bridge (`epic-bridge-demo`, epic-hal#278,
PR #283). Control and bridge are still open branches, so their rows
carry verified rerun numbers, not live CI output yet.

## Normalization

XC8 reports PIC18 program space in bytes while epic-cc reports words,
so every flash figure below is normalized to words: XC8 program bytes
divided by 2. RAM is bytes on both sides. The 18F4550 budgets are
16384 flash words and 2048 RAM bytes.

## Scoreboard

| Demo | XC8 build | XC8 gate | XC8 flash (words) | XC8 RAM (bytes) | epic-cc build | epic-cc gate | epic-cc flash | epic-cc RAM | UART trace diff | Rerun |
|---|---|---|---|---|---|---|---|---|---|---|
| menu | PASS | PASS | 9068/16384 (55.3%) | 639/2048 (31.2%) | FAIL (flash overflow, STALE) | not reached | 19497/16384 (119%) | not reached | not reached (no epic-cc hex) | `scripts/compare-toolchains.sh epic-menu-demo 18F4550 PIC18F4550 60000 4` |
| control | PASS | PASS | 8765/16384 (53.5%) | 666/2048 (32.5%) | BLOCKED (epic-cc#607) | blocked | blocked | blocked | pending (no epic-cc hex yet) | `scripts/compare-toolchains.sh epic-control-demo 18F4550 PIC18F4550 60000 20` |
| bridge | PASS | PASS | 11181/16384 (68.2%) | 955/2048 (46.6%) | BLOCKED (epic-cc#608) | blocked | blocked | blocked | pending (no epic-cc hex yet) | `scripts/compare-toolchains.sh epic-bridge-demo 18F4550 PIC18F4550 60000` |

Raw XC8 figures behind the normalized words: control Program 447Ah
(8765 words) with Data 29Ah (666 B); bridge Program 575Ah (11181
words) with Data 3BBh (955 B). Menu XC8 figures come from
`docs/pic18f4550-menu-demo.md`, kept as the source of truth there.

## Why each epic-cc cell is not green

Menu overflow (19497 words, about 2.15x XC8) predates the epic-cc size
work in apojomovsky/epic-cc#474, #475, #483 and #485, so the row is
marked STALE instead of refreshed here: a pinned-driver rerun is its
own job and stays out of this scoreboard.

Control is BLOCKED by apojomovsky/epic-cc#607: epic-math bcd.c uses
NULL without a definition, a pre-existing compiler gap this demo
reached first. No further compiler assertion was hit past it.

Bridge is BLOCKED by apojomovsky/epic-cc#608: irparse rejects a 259
byte struct over its 255 cap. Same shape as control: the first panic
is filed and on the board, nothing past it was reached.

## CI

PRs stay XC8-only through the existing family jobs. The full
dual-toolchain size plus UART trace comparison runs nightly in
`.github/workflows/nightly.yml` (`demo-scoreboard`, one matrix leg per
row above). Legs report instead of gating while scoreboard cells are
STALE or BLOCKED: a module missing from the checkout (sibling branch
unmerged) skips, and the compare step carries `continue-on-error` with
its size table appended to the step summary.
