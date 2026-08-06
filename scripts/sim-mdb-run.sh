#!/usr/bin/env bash
# Run a module's already-emitted HARNESS=sim (--variant sim) build under
# MPLAB SIM (mdb), checking for the EPIC_HARNESS_RESULT marker (see
# epic-common/include/core/epic_harness.h). Shared by
# .github/workflows/sim-tests.yml and scripts/sim-test-local.sh, so CI
# and a local run go through the exact same build+mdb+grep sequence,
# not two copies that can drift apart (docs/ci-plan.md's local
# reproduction plan).
#
# Container-only on purpose: needs xc8-cc and mdb.sh on PATH plus
# $XC8_INSTALL_DIR, nothing else. It does NOT read the manifest itself
# (no python3 call), because it runs inside the toolchain container
# (docker/ci-toolchain/Dockerfile), which deliberately has none. The
# build script it runs (build-sim/<module>/<mcu>/build.sh) must already
# exist, emitted beforehand by `epic_build.py build --variant sim`
# wherever python3 is available: sim-tests.yml's emit job (bare
# runner), or scripts/sim-test-local.sh (the host, before its `docker
# run` into this same container).
#
# Usage: sim-mdb-run.sh <family> <mcu> <device> <module> [wait_ms] [mode] [extra_mdb]
#   family    matrix label, only used to namespace temp files (e.g. pic16f87xa)
#   mcu       manifest MCU value (e.g. 16F877A)
#   device    mdb `device` command's part name (e.g. PIC16F877A)
#   module    manifest module name (e.g. epic-tick, or
#             epic-pic16f193x-firmware for the bare-HAL smoke); looks
#             for its pre-emitted script at build-sim/<module>/<mcu>/build.sh
#   wait_ms   real-time ms to let mdb run before halting (default 2000;
#             MPLAB SIM runs noticeably slower than real-time, see
#             docs/ci-plan.md Phase 4's findings, so this is a wall-clock
#             budget, not a simulated-time one)
#   mode      reporting mode: "uart" (default) or "gpio". uart captures
#             EPIC_HARNESS_RESULT marker from the UART; gpio reads the
#             marker from a PORTA register via mdb `print` and checks
#             bit 0. Only pic16f193x currently uses gpio; pic16f87xa
#             and pic18fxx5x keep uart.
#   extra_mdb extra mdb commands (e.g. `print`s), inserted right before
#             `quit`, for register-level debugging without hardcoding
#             device-specific diagnostics into this generic script.

set -euo pipefail

family="$1"; mcu="$2"; device="$3"; module="$4"
wait_ms="${5:-2000}"; mode="${6:-uart}"; extra_mdb="${7:-}"

if [ "$mode" != "uart" ] && [ "$mode" != "gpio" ]; then
  echo "::error::mode must be 'uart' or 'gpio', got '$mode'" >&2
  exit 1
fi

build_dir="build-sim/${module}"
objdir="${build_dir}/${mcu}"
script="${objdir}/build.sh"
if [ ! -f "$script" ]; then
  echo "::error::no emitted build script at ${script}." \
       "Run 'python3 scripts/epic_build.py build --module ${module}" \
       "--mcu ${mcu} --variant sim --build-dir ${build_dir}'" \
       "first (needs python3, so outside this container)." >&2
  exit 1
fi
rm -f "$objdir"/*.p1
EPIC_REPO_ROOT="$PWD" sh "$script"

hexes=("$build_dir"/"$mcu"-*.hex)
if [ ! -e "${hexes[0]}" ] || [ "${#hexes[@]}" -ne 1 ]; then
  echo "::error::expected exactly one ${build_dir}/${mcu}-*.hex, found: ${hexes[*]}" >&2
  exit 1
fi
hex="${hexes[0]}"

if [ "$mode" = "gpio" ]; then
  out="/tmp/${family}-gpio.txt"
  rm -f "$out"
  mdb_script="/tmp/${family}-mdb.txt"
  cat > "$mdb_script" <<SCRIPT
device ${device}
hwtool SIM
program ${hex}
run
wait ${wait_ms}
halt
print PORTA
${extra_mdb}
quit
SCRIPT
else
  out="/tmp/${family}-uart.txt"
  rm -f "$out"
  mdb_script="/tmp/${family}-mdb.txt"
  cat > "$mdb_script" <<SCRIPT
device ${device}
set uart1io.uartioenabled true
set uart1io.output file
set uart1io.outputfile ${out}
hwtool SIM
program ${hex}
run
wait ${wait_ms}
halt
${extra_mdb}
quit
SCRIPT
fi

mdb_ok=1
if [ "$mode" = "gpio" ]; then
  mdb.sh "$mdb_script" > "$out" 2>&1 || mdb_ok=0
else
  mdb.sh "$mdb_script" || mdb_ok=0
fi

if [ "$mode" = "gpio" ]; then
  echo "---- captured PORTA readback ----"
  cat "$out" 2>/dev/null || echo "(no output file produced)"
  echo "----------------------------------"

  if [ "$mdb_ok" -ne 1 ]; then
    echo "::error::mdb.sh itself exited non-zero"
    exit 1
  fi
  if [ ! -s "$out" ]; then
    echo "::error::mdb produced no PORTA output at all (simulator" \
         "may have errored, or the harness never reached report())"
    exit 1
  fi
  # Read the byte from the captured output. mdb's `print <REG>` outputs
  # "<REG>=NN" (decimal) by default, or "=0xNN" if hex mode is set;
  # accept either form. Strip any 0x prefix, parse as decimal (mdb's
  # default form), and check bit 0.
  por_line=$(grep -E 'PORTA\s*=' "$out" | tail -n 1 | sed -E 's/.*=\s*(0x)?([0-9A-Fa-f]+).*/\2/')
  if [ -z "$por_line" ]; then
    echo "::error::no PORTA value found in mdb output"
    exit 1
  fi
  # Decimal parse (mdb's default).
  por_byte=$((por_line))
  if [ $((por_byte & 0x01)) -ne 0 ]; then
    echo "PASS marker found (PORTA bit 0 set, byte=${por_byte})"
    exit 0
  fi
  echo "::error::PORTA bit 0 was 0 after halt (FAIL marker)"
  exit 1
fi

# uart mode (existing behavior preserved exactly)
echo "---- captured UART output ----"
cat "$out" 2>/dev/null || echo "(no output file produced)"
echo "-------------------------------"

if [ "$mdb_ok" -ne 1 ]; then
  echo "::error::mdb.sh itself exited non-zero"
  exit 1
fi
if [ ! -s "$out" ]; then
  echo "::error::mdb produced no UART output at all (simulator" \
       "may have errored, or the harness never reached report())"
  exit 1
fi
if grep -q "EPIC_HARNESS_RESULT: FAIL" "$out"; then
  echo "::error::sim-target harness reported FAIL"
  exit 1
fi
if ! grep -q "EPIC_HARNESS_RESULT: PASS" "$out"; then
  echo "::error::no PASS/FAIL marker found in captured UART output"
  exit 1
fi
echo "PASS marker found"
