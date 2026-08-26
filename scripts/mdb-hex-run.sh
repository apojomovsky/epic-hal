#!/usr/bin/env bash
# Program an existing hex under MPLAB SIM and run EXTRA_MDB register
# reads after the first wait; shared by make mdb-hex and the epiccc-gate
# CI job (the mdb command sequence has one source of truth, same shape
# as sim-mdb-run.sh's harness gates). No HARNESS=sim rebuild: this gates
# hexes another toolchain produced. Container-only (mdb.sh, no python3).
#
# Usage: mdb-hex-run.sh <hex> <device> [wait_ms] [extra_mdb]

set -euo pipefail

# hex/device as the container sees them; wait_ms is wall-clock (SIM
# slower than real time); extra_mdb is \n-escaped, inserted before quit.
hex="$1"; device="$2"
wait_ms="${3:-2000}"; extra_mdb="${4:-}"

if [ ! -f "$hex" ]; then
  echo "error: no such hex: $hex" >&2
  exit 1
fi

mdb_script="/tmp/mdb-hex-$$.txt"
{
  echo "device ${device}"
  echo "hwtool SIM"
  echo "program ${hex}"
  echo "run"
  echo "wait ${wait_ms}"
  echo "halt"
  if [ -n "$extra_mdb" ]; then
    printf "%b\n" "$extra_mdb"
  fi
  echo "quit"
} > "$mdb_script"
mdb.sh "$mdb_script"
