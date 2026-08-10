#!/usr/bin/env bash
# MPLAB SIM (mdb) run loop for ci.yml's "target" job: the original 3
# entries the old sim-tests.yml matrixed (one per family) plus
# epic-swuart's own real mdb gate (docs/superpowers/plans/2026-08-07-
# swuart-v3.md Task 8), now a plain loop calling scripts/sim-mdb-run.sh,
# the exact script scripts/sim-test-local.sh also calls for a local
# repro, so CI and a local run go through one source of truth. wait_ms
# values are wall-clock budgets under MPLAB SIM (MPLAB SIM runs
# noticeably slower than real-time). Most gates run at 5000; the three
# slower scenarios were measured 2026-08-11 (floors: epic-serial ~4s,
# epic-swuart ~8s, 193X firmware ~25s locally) and set with margin:
# epic-serial 10000, epic-swuart 15000. The 193X firmware gate stays at
# 60000: its 40000 budget flaked once in CI (2026-08-11), so the
# proven-safe value wins over the measured floor.
#
# Usage: ci-target-sim.sh [summary.md]
#
# Does not stop at the first failure. Exits 1 if anything failed.
#
# REPEAT=N runs every gate N times (default 1); any failed run fails
# the gate, and the summary records "PASS (n/N)" or "FAIL (n/N)". This
# is the flake-hardening harness (quality task 10): a flake-hunt is
# `REPEAT=5 bash scripts/ci-target-sim.sh`, and the workflow_dispatch
# CI job drives the same command without slowing the PR target job.
#
# FAMILY=<pic16f87xa|pic18fxx5x|pic16f193x> runs only that family's
# gates (the sharded CI jobs); unset runs everything.
#
# PARALLEL=N (default 1) runs N gates concurrently. The runner has 2
# cores and each mdb session is single-threaded; sim-mdb-run.sh's temp
# files are PID-suffixed so concurrent same-family gates do not
# collide.

set -uo pipefail

summary="${1:-ci-summary-sim.md}"
repeat="${REPEAT:-1}"
family_filter="${FAMILY:-}"
parallel="${PARALLEL:-1}"

fail=0
{
  echo "| Family | MCU | Module | Result |"
  echo "|---|---|---|---|"
} > "$summary"

run_one() {
  family="$1"; mcu="$2"; device="$3"; module="$4"; wait_ms="$5"; mode="$6"; eeprom_writes="${7:-}"
  # The manifest family names (PIC16F87XA etc.) are uppercase; the
  # run_one labels below are lowercase, so compare case-insensitively.
  [ -z "$family_filter" ] \
    || [ "$(printf '%s' "$family" | tr 'A-Z' 'a-z')" \
         = "$(printf '%s' "$family_filter" | tr 'A-Z' 'a-z')" ] \
    || return 0
  # Args 7 (extra_mdb) and 8 (eeprom_writes) of sim-mdb-run.sh: no gate
  # here uses extra_mdb, so it stays empty and the 7th run_one arg maps
  # to the 8th runner arg (eeprom_writes), which epic-settings needs.
  local n pass=0
  for n in $(seq 1 "$repeat"); do
    if scripts/sim-mdb-run.sh "$family" "$mcu" "$device" "$module" "$wait_ms" "$mode" "" "$eeprom_writes"; then
      pass=$((pass + 1))
    else
      echo "FAIL (run ${n}/${repeat}): ${family} ${mcu} ${module}"
    fi
  done
  if [ "$pass" -eq "$repeat" ]; then
    echo "PASS: ${family} ${mcu} ${module}"
    echo "| ${family} | ${mcu} | ${module} | PASS (${pass}/${repeat}) |" >> "$summary"
  else
    echo "FAIL: ${family} ${mcu} ${module} (${pass}/${repeat} runs passed)"
    echo "| ${family} | ${mcu} | ${module} | FAIL (${pass}/${repeat}) |" >> "$summary"
    return 1
  fi
  return 0
}

if [ "$parallel" -le 1 ]; then
run_one pic16f87xa 16F877A PIC16F877A epic-tick 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 epic-tick 5000 uart
run_one pic16f193x 16F1937 PIC16F1937 epic-pic16f193x-firmware 60000 gpio
run_one pic16f87xa 16F877A PIC16F877A epic-swuart 15000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-math 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 epic-math 5000 uart
run_one pic16f87xa 16F877A PIC16F877A pic16f87xa-hal 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 pic18fxx5x-hal 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 epic-pid 5000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-fsm 5000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-adcfilter 5000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-encoder 5000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-bus 5000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-mcp23x17 5000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-serial 10000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-lcd 5000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-debounce 5000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-combo-uart-ssp 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 epic-console 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 epic-taskmgr 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 epic-settings 5000 uart 24
run_one pic18fxx5x 18F4550 PIC18F4550 epic-modbus 5000 uart

run_one pic16f87xa 16F877A PIC16F877A epic-combo-multitimer 5000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-combo-adc-uart 5000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-combo-rb-uart 5000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-combo-tick-serial 5000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-combo-encoder-tick 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 epic-combo-lcd-tick 5000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-combo-swuart-tick 5000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-combo-rx-loopback 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 epic-combo-eeprom-isr 5000 uart 32
run_one pic18fxx5x 18F4550 PIC18F4550 epic-combo-tick-settings 5000 uart 32
run_one pic18fxx5x 18F4550 PIC18F4550 epic-combo-taskmgr-serial 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 epic-combo-modbus-full 5000 uart
fi

if [ "$parallel" -gt 1 ]; then
  # The run_one calls above are the single spec list; extract and
  # dispatch them concurrently (run_one applies the FAMILY filter).
  export -f run_one
  export summary repeat family_filter
  specs="$(sed -n 's/^[[:space:]]*run_one //p' "$0")"
  if ! printf '%s\n' "$specs" \
       | xargs -P"$parallel" -L1 bash -c 'run_one "$@"' _; then
    fail=1
  fi
fi

exit "$fail"
