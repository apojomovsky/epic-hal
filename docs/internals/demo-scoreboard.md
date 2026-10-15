# Demo scoreboard: XC8 vs epic-cc on the 18F4550 flagships (epic-hal#279)

Status: 2026-09-23. One table for the three product-shaped PIC18F4550
demos: menu (`demos/menu`), control (`demos/control`,
epic-hal#277, PR #281), bridge (`demos/bridge`, epic-hal#278,
PR #283). Control and bridge are still open branches, so their rows
carry verified rerun numbers, not live CI output yet.

## Normalization

XC8 reports PIC18 program space in bytes while epic-cc reports words,
so the comparison normalizes flash to words. RAM is bytes on both
sides. The 18F4550 budgets are 16384 flash words and 2048 RAM bytes.
The XC8 figures themselves are not published here: they live in the
private epic-benchmarks repository.

## Scoreboard

| Demo | XC8 build | XC8 gate | epic-cc build | epic-cc gate | epic-cc flash | epic-cc RAM | UART trace diff | Rerun |
|---|---|---|---|---|---|---|---|---|
| menu | PASS | PASS | PASS | FAIL (epic-cc#632) | 11888/16384 (72.6%) | 743/2048 (36.3%) | no capture (epic-cc hex silent, see below) | `scripts/sim/compare-toolchains.sh demos/menu 18F4550 PIC18F4550 1500 4` |
| control | PASS | PASS | PASS | FAIL (epic-cc#613, epic-cc#614) | 13081/16384 (79.8%) | 961/2048 (46.9%) | DIVERGES (both hexes run; traces differ, see below) | `scripts/sim/compare-toolchains.sh demos/control 18F4550 PIC18F4550 60000 20` |
| bridge | PASS | PASS | BLOCKED (epic-cc#608) | blocked | blocked | blocked | pending (no epic-cc hex yet) | `scripts/sim/compare-toolchains.sh demos/bridge 18F4550 PIC18F4550 60000` |

## Why each epic-cc cell is not green

Menu fits again (11888 words, was 19497 and over budget):
the overflow predates the epic-cc size work and is resolved on current
master. What remains is behavioral: the epic-cc hex runs the full SIM
wait but captures no UART where XC8 captures a session, filed as
apojomovsky/epic-cc#632.

Control builds clean under epic-cc (driver 885fe58, built in-image per
DEVELOPMENT.md) but its gate does not reach the report: the mdb run
boots the shell, emits one truncated heartbeat (`HB sp=2 meas=0 ou`)
and stalls. Baud runs ~183 because SPBRG reads back 0xFFFF
(apojomovsky/epic-cc#613), and the TX callback dispatch spins on
no-match (apojomovsky/epic-cc#614).

Bridge is BLOCKED by apojomovsky/epic-cc#608: irparse rejects a 259
byte struct over its 255 cap, so no hex is emitted. The first panic
is filed and on the board; nothing past it was reached.

## CI

PRs stay XC8-only through the existing family jobs. The full
dual-toolchain size plus UART trace comparison runs nightly in
`.github/workflows/nightly.yml` (`demo-scoreboard`, one matrix leg per
row above). Legs report instead of gating while scoreboard cells are
STALE or BLOCKED: a module missing from the checkout (sibling branch
unmerged) skips, and the compare step carries `continue-on-error` with
its size table appended to the step summary.
