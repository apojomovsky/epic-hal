#!/usr/bin/env bash
# lib/usb silicon bring-up gate: the repeatable half of the silicon
# check. The sim half runs the real firmware under MPLAB SIM and reads
# back the SIE state (the simulator models no USB SIE, so it proves
# the SIE is programmed, not that the device enumerates). The hw half
# needs a board: it finds the CDC-ACM port and requires the bytes
# written to come back, which is the enumeration evidence. The bring-up
# project's README has what each half proves.

set -uo pipefail
mode="${1:-}"
if [ -z "$mode" ]; then
  echo "usage: usb-bringup-gate.sh sim <module> <mcu> <device>" >&2
  echo "       usb-bringup-gate.sh hw [--timeout <seconds>]" >&2
  exit 1
fi

repo_root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$repo_root"

case "$mode" in
sim)
  module="$2"
  mcu="$3"
  device="$4"
  echo "== lib/usb bring-up gate (simulator): ${module} on ${mcu} =="
  # The emitted build script comes from the manifest, so the firmware
  # under test is exactly what the real-target build produces.
  build_dir="build-sim/${module}"
  if [ ! -f "${build_dir}/${mcu}/build.sh" ]; then
    echo "::error::no emitted sim build at ${build_dir}/${mcu}/build.sh" >&2
    echo "  run: python3 scripts/build/epic_build.py build --module ${module}" >&2
    echo "        --mcu ${mcu} --variant sim --build-dir ${build_dir}" >&2
    echo "        --dfp-dir <pack>/xc8" >&2
    exit 1
  fi
  scripts/sim/sim-mdb-run.sh local "$mcu" "$device" "$module" 5000 uart
  ;;

hw)
  shift
  echo "== lib/usb bring-up gate (hardware): enumeration and CDC echo =="
  python3 scripts/sim/usb-cdc-echo-gate.py "$@"
  ;;

*)
  echo "::error::unknown mode '$mode'" >&2
  exit 1
  ;;
esac
