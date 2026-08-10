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
  family="$1"; mcu="$2"; device="$3"; module="$4"; wait_ms="$5"; mode="$6"; eeprom_writes="${7:-}"
  # Args 7 (extra_mdb) and 8 (eeprom_writes) of sim-mdb-run.sh: no gate
  # here uses extra_mdb, so it stays empty and the 7th run_one arg maps
  # to the 8th runner arg (eeprom_writes), which epic-settings needs.
  if scripts/sim-mdb-run.sh "$family" "$mcu" "$device" "$module" "$wait_ms" "$mode" "" "$eeprom_writes"; then
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
run_one pic16f87xa 16F877A PIC16F877A pic16f87xa-hal 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 pic18fxx5x-hal 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 epic-pid 5000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-fsm 5000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-adcfilter 5000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-encoder 5000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-bus 5000 uart
run_one pic16f87xa 16F877A PIC16F877A epic-serial 60000 uart
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

exit "$fail"
