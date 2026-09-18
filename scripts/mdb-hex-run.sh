#!/usr/bin/env bash
# Program an existing hex under MPLAB SIM and run EXTRA_MDB register
# reads after the first wait; shared by make mdb-hex and epiccc-gate
# (one source of truth for the mdb sequence, same shape as
# sim-mdb-run.sh). No HARNESS=sim rebuild: gates a hex another
# toolchain produced. Container-only (mdb.sh, no python3).
#
# Usage: mdb-hex-run.sh <hex> <device> [wait_ms] [extra_mdb] [capture_uart_to] [eeprom_writes]
#   capture_uart_to/eeprom_writes: optional, same meaning and defaults
#   (empty / 0) as sim-mdb-run.sh's uart mode and eeprom_writes arg;
#   see scripts/compare-toolchains.sh for a capture_uart_to caller.

set -euo pipefail

# hex/device as the container sees them; wait_ms is wall-clock (SIM
# slower than real time); extra_mdb is \n-escaped, inserted before quit.
hex="$1"; device="$2"
wait_ms="${3:-2000}"; extra_mdb="${4:-}"
capture_uart_to="${5:-}"
eeprom_writes="${6:-0}"
eeprom_econ1_addr="${EECON1_ADDR:-0xFA6}"
eeprom_econ2_addr="${EECON2_ADDR:-0xFA7}"

if [ ! -f "$hex" ]; then
  echo "error: no such hex: $hex" >&2
  exit 1
fi

eeprom_cycles=""
i=0
while [ "$i" -lt "$eeprom_writes" ]; do
  eeprom_cycles="${eeprom_cycles}
run
wait 500
halt
write /r ${eeprom_econ1_addr} 0x04
write /r ${eeprom_econ2_addr} 0x55
write /r ${eeprom_econ2_addr} 0xAA
write /r ${eeprom_econ1_addr} 0x06"
  i=$((i + 1))
done

mdb_script="/tmp/mdb-hex-$$.txt"
{
  echo "device ${device}"
  if [ -n "$capture_uart_to" ]; then
    echo "set uart1io.uartioenabled true"
    echo "set uart1io.output file"
    echo "set uart1io.outputfile ${capture_uart_to}"
  fi
  echo "hwtool SIM"
  echo "program ${hex}"
  [ -n "$eeprom_cycles" ] && echo "${eeprom_cycles}"
  echo "run"
  echo "wait ${wait_ms}"
  echo "halt"
  if [ -n "$extra_mdb" ]; then
    printf "%b\n" "$extra_mdb"
  fi
  echo "quit"
} > "$mdb_script"
mdb.sh "$mdb_script"
