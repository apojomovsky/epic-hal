#!/usr/bin/env bash
# MPLAB SIM run loop for the target job and `make target-ci`, over
# scripts/sim/sim-mdb-run.sh. wait_ms is wall-clock. Exits 1 on any failure.
# Usage: ci-target-sim.sh [summary.md]; REPEAT=N; FAMILY=<fam> (sharded CI);
# PARALLEL=N (sim-mdb-run.sh temps are PID-suffixed).
# run_one takes an optional 10th arg naming an extra manifest sim gate
# (empty: the default gate); one line per gate for multi-gate MCUs.

set -uo pipefail

summary="${1:-ci-summary-sim.md}"
repeat="${REPEAT:-1}"
family_filter="${FAMILY:-}"
parallel="${PARALLEL:-1}"

# TOGGLE_REG=LATB (harmless: no other run_one line uses MODE=toggle):
# PIC18F1320's simulated PORTB does not mirror LATB for an output pin
# under MPLAB SIM, unlike every other family's gate. Exported at the
# top so the parallel path's xargs subshells still inherit it.
export TOGGLE_REG=LATB

fail=0
{
  echo "| Family | MCU | Module | Result |"
  echo "|---|---|---|---|"
} > "$summary"

run_one() {
  family="$1"; mcu="$2"; device="$3"; module="$4"; wait_ms="$5"; mode="$6"; eeprom_writes="${7:-}"
  # Arg 10 names an extra manifest sim gate (empty: the default gate).
  # A module whose MCU resolves several gates carries one run_one line
  # per gate; the extra lines name their gate here.
  sim_gate="${10:-}"
  gate_label=""
  [ -n "$sim_gate" ] && gate_label=" ${sim_gate}"
  # Per-call toggle-gate overrides: TOGGLE_REG / TOGGLE_STEPI. The
  # global TOGGLE_REG=LATB export above serves the PIC18F1320 gate; a
  # family whose gate must sample a different register or step size
  # overrides it here (arg 8 = TOGGLE_REG, arg 9 = TOGGLE_STEPI).
  tog_reg="${8:-}"
  tog_stepi="${9:-}"
  local tog_env=""
  if [ "$mode" = "toggle" ] && [ -n "$tog_reg" ]; then
    tog_env="TOGGLE_REG=${tog_reg}"
    if [ -n "$tog_stepi" ]; then
      tog_env="${tog_env} TOGGLE_STEPI=${tog_stepi}"
    fi
  fi
  # The manifest family names (PIC16F87XA etc.) are uppercase; the
  # run_one labels below are lowercase, so compare case-insensitively.
  [ -z "$family_filter" ] \
    || [ "$(printf '%s' "$family" | tr 'A-Z' 'a-z')" \
         = "$(printf '%s' "$family_filter" | tr 'A-Z' 'a-z')" ] \
    || return 0
  # Args 7 (extra_mdb) and 8 (eeprom_writes) of sim-mdb-run.sh: no gate
  # here uses extra_mdb, so it stays empty and the 7th run_one arg maps
  # to the 8th runner arg (eeprom_writes), which lib/settings needs.
  # PIC18 does not reflect a driven LATx latch back into the PORTx input
  # register under MPSIM (verified on PIC18F2520 2026-09-14,
  # PIC18F6520 2026-09-15, and PIC18F2320 2026-09-17), so the PIC18
  # gpio gates read the latch LATA; the PIC16 families keep the PORTA
  # default (sim-mdb-run.sh's GPIO_REG). See sim-mdb-run.sh's gpio
  # block.
  local gpio_reg_env=""
  if [ "$mode" = "gpio" ] && { [ "$device" = "PIC18F2520" ] || [ "$device" = "PIC18F6520" ] \
       || [ "$device" = "PIC18F2320" ]; }; then
    gpio_reg_env="GPIO_REG=LATA"
  fi
  local n pass=0
  for n in $(seq 1 "$repeat"); do
    if [ -n "$tog_env" ]; then
      if env $tog_env scripts/sim/sim-mdb-run.sh "$family" "$mcu" "$device" "$module" "$wait_ms" "$mode" "" "$eeprom_writes" "$sim_gate"; then
        pass=$((pass + 1))
      else
        echo "FAIL (run ${n}/${repeat}): ${family} ${mcu} ${module}"
      fi
    elif [ -n "$gpio_reg_env" ]; then
      if env "$gpio_reg_env" scripts/sim/sim-mdb-run.sh "$family" "$mcu" "$device" "$module" "$wait_ms" "$mode" "" "$eeprom_writes" "$sim_gate"; then
        pass=$((pass + 1))
      else
        echo "FAIL (run ${n}/${repeat}): ${family} ${mcu} ${module}"
      fi
    elif scripts/sim/sim-mdb-run.sh "$family" "$mcu" "$device" "$module" "$wait_ms" "$mode" "" "$eeprom_writes" "$sim_gate"; then
      pass=$((pass + 1))
    else
      echo "FAIL (run ${n}/${repeat}): ${family} ${mcu} ${module}"
    fi
  done
  if [ "$pass" -eq "$repeat" ]; then
    echo "PASS: ${family} ${mcu} ${module}${gate_label}"
    echo "| ${family} | ${mcu} | ${module}${gate_label} | PASS (${pass}/${repeat}) |" >> "$summary"
  else
    echo "FAIL: ${family} ${mcu} ${module}${gate_label} (${pass}/${repeat} runs passed)"
    echo "| ${family} | ${mcu} | ${module}${gate_label} | FAIL (${pass}/${repeat}) |" >> "$summary"
    fail=1
    return 1
  fi
  return 0
}

if [ "$parallel" -le 1 ]; then
run_one pic16f87xa 16F877A PIC16F877A lib/tick 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 lib/tick 5000 uart
run_one pic16f193x 16F1939 PIC16F1939 hal/pic14e/16f193x 60000 gpio
run_one pic16f193x 16F1939 PIC16F1939 lib/tick 5000 gpio
run_one pic16f193x 16F1939 PIC16F1939 lib/fsm 5000 gpio
run_one pic16f1508 16F1509 PIC16F1509 hal/pic14e/16f1508 60000 gpio
run_one pic16f188xx 16F18877 PIC16F18877 hal/pic14e/16f188xx 60000 gpio
run_one pic16f87xa 16F877A PIC16F877A lib/swuart 15000 uart
run_one pic16f87xa 16F877A PIC16F877A lib/math 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 lib/math 5000 uart
run_one pic16f87xa 16F877A PIC16F877A hal/pic14/16f87xa 5000 uart
run_one pic16f7x 16F77 PIC16F77 hal/pic14/16f7x 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 hal/pic18/18fxx5x 5000 uart
run_one pic18f2520 18F2320 PIC18F2320 hal/pic18/18f2520 5000 gpio
run_one pic18f2520 18F2520 PIC18F2520 hal/pic18/18f2520 5000 gpio
run_one pic18f6520 18F6520 PIC18F6520 hal/pic18/18f6520 5000 gpio
run_one pic18f6520 18F6520 PIC18F6520 hal/pic18/18f6520 5000 gpio "" "" "" lvd-readback
run_one pic18fxx5x 18F4550 PIC18F4550 lib/pid 5000 uart
run_one pic16f87xa 16F877A PIC16F877A lib/fsm 5000 uart
run_one pic16f87xa 16F877A PIC16F877A lib/adcfilter 5000 uart
run_one pic16f87xa 16F877A PIC16F877A lib/encoder 5000 uart
run_one pic16f87xa 16F877A PIC16F877A lib/bus 5000 uart
run_one pic16f87xa 16F877A PIC16F877A lib/mcp23x17 5000 uart
run_one pic16f87xa 16F877A PIC16F877A lib/serial 10000 uart
run_one pic16f87xa 16F877A PIC16F877A lib/lcd 5000 uart
run_one pic16f87xa 16F877A PIC16F877A lib/debounce 5000 uart
run_one pic16f87xa 16F877A PIC16F877A tests/epic-combo-uart-ssp 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 lib/console 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 lib/taskmgr 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 lib/settings 5000 uart 24
run_one pic18fxx5x 18F4550 PIC18F4550 lib/modbus 5000 uart

run_one pic16f87xa 16F877A PIC16F877A tests/epic-combo-multitimer 5000 uart
run_one pic16f87xa 16F877A PIC16F877A tests/epic-combo-adc-uart 5000 uart
run_one pic16f87xa 16F877A PIC16F877A tests/epic-combo-rb-uart 5000 uart
run_one pic16f87xa 16F877A PIC16F877A tests/epic-combo-tick-serial 5000 uart
run_one pic16f87xa 16F877A PIC16F877A tests/epic-combo-encoder-tick 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 tests/epic-combo-lcd-tick 5000 uart
run_one pic16f87xa 16F877A PIC16F877A tests/epic-combo-swuart-tick 5000 uart
run_one pic16f87xa 16F877A PIC16F877A tests/epic-combo-rx-loopback 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 tests/epic-combo-eeprom-isr 5000 uart 32
run_one pic18fxx5x 18F4550 PIC18F4550 tests/epic-combo-tick-settings 5000 uart 32
run_one pic18fxx5x 18F4550 PIC18F4550 tests/epic-combo-taskmgr-serial 5000 uart
run_one pic18fxx5x 18F4550 PIC18F4550 tests/epic-combo-modbus-full 5000 uart
run_one pic16f88x 16F887 PIC16F887 hal/pic14/16f88x 5000 uart
run_one pic16f88x 16F887 PIC16F887 lib/tick 5000 uart
run_one pic16f88x 16F887 PIC16F887 lib/swuart 15000 uart
run_one pic16f88x 16F887 PIC16F887 lib/math 5000 uart
run_one pic16f88x 16F887 PIC16F887 lib/fsm 5000 uart
run_one pic16f88x 16F887 PIC16F887 lib/adcfilter 5000 uart
run_one pic16f88x 16F887 PIC16F887 lib/encoder 5000 uart
run_one pic16f88x 16F887 PIC16F887 lib/bus 5000 uart
run_one pic16f88x 16F887 PIC16F887 lib/mcp23x17 5000 uart
run_one pic16f88x 16F887 PIC16F887 lib/serial 10000 uart
run_one pic16f88x 16F887 PIC16F887 lib/debounce 5000 uart
run_one pic16f628a 16F628A PIC16F628A hal/pic14/16f628a 15000 uart
run_one pic16f628a 16F648A PIC16F648A lib/tick 5000 uart
run_one pic16f83_84 16F84A PIC16F84A hal/pic14/16f83_84 5000 gpio
run_one pic16f818_819 16F819 PIC16F819 hal/pic14/16f818_819 5000 gpio
run_one pic16f63x_67x_68x 16F677 PIC16F677 hal/pic14/16f63x_67x_68x 15000 gpio
# MODE=toggle on PORTB bit 0 with a 50000-instruction sample step.
# The blink's toggle period is ~50000 steps at 4 MHz (Timer0 Fosc/4,
# 1:256 prescaler, reload 0: 65536 instr per overflow, one toggle per
# overflow), so 50000 alternates; the 200000 default aliases to a
# constant phase (verified 2026-09-15) and would fail the gate.
# TOGGLE_REG=PORTB overrides the script-wide LATB export (that export
# exists for PIC18F1320, whose simulated PORTB does not mirror LATB).
run_one pic16f5x 16F54 PIC16F54 hal/pic12/16f5x 5000 toggle "" PORTB 50000
# The four sibling parts take the same parameters: same 4 MHz config and
# blink period (measured per part 2026-09-17), and PORTB because the
# 505/506 have no PORTA.
run_one pic16f5x 16F505 PIC16F505 hal/pic12/16f5x 5000 toggle "" PORTB 50000
run_one pic16f5x 16F506 PIC16F506 hal/pic12/16f5x 5000 toggle "" PORTB 50000
run_one pic16f5x 16F57 PIC16F57 hal/pic12/16f5x 5000 toggle "" PORTB 50000
run_one pic16f5x 16F59 PIC16F59 hal/pic12/16f5x 5000 toggle "" PORTB 50000
run_one pic18f1320 18F1320 PIC18F1320 hal/pic18/18f1320 2000 toggle
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
