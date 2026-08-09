#!/usr/bin/env bash
# MPLAB SIM (mdb) run loop for ci.yml's "target" job: the original 3
# entries the old sim-tests.yml matrixed (one per family) plus
# epic-swuart's own real mdb gate (docs/superpowers/plans/2026-08-07-
# swuart-v3.md Task 8), now a plain loop calling scripts/sim-mdb-run.sh,
# the exact script scripts/sim-test-local.sh also calls for a local
# repro, so CI and a local run go through one source of truth. wait_ms
# values (5000, 60000 for pic16f193x and epic-swuart) are confirmed-
# passing wall-clock budgets under MPLAB SIM (MPLAB SIM runs noticeably
# slower than real-time); see git history on the old sim-tests.yml for
# how the first 3 were established, and epic-swuart's own entry's own
# commit for how 60000 was confirmed for it.
#
# Usage: ci-target-sim.sh [summary.md]
#
# Does not stop at the first failure. Exits 1 if anything failed.

set -uo pipefail

summary="${1:-ci-summary-sim.md}"

fail=0
{
  echo "| Family | MCU | Module | Result |"
  echo "|---|---|---|---|"
} > "$summary"

run_one() {
  family="$1"; mcu="$2"; device="$3"; module="$4"; wait_ms="$5"; mode="$6"
  if scripts/sim-mdb-run.sh "$family" "$mcu" "$device" "$module" "$wait_ms" "$mode"; then
    echo "PASS: ${family} ${mcu} ${module}"
    echo "| ${family} | ${mcu} | ${module} | PASS |" >> "$summary"
  else
    echo "FAIL: ${family} ${mcu} ${module}"
    echo "| ${family} | ${mcu} | ${module} | FAIL |" >> "$summary"
    fail=1
  fi
}

run_one pic16f87xa 16F877A PIC16F877A epic-tick 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 epic-tick 5000 uart
run_one pic16f193x 16F1937 PIC16F1937 epic-pic16f193x-firmware 60000 gpio
run_one pic16f87xa 16F877A PIC16F877A epic-swuart 60000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-math 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 epic-math 5000 uart

exit "$fail"
