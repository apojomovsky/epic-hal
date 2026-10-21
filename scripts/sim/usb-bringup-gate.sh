#!/usr/bin/env bash
# lib/usb silicon bring-up gate: the repeatable half of the silicon
# check, plus the host-side echo round trip when a board is attached.
#
# Two gates, because they prove different things:
#
#   sim  The MPLAB SIM gate (the CI one): the firmware's SIE register
#        image and its buffer descriptor table, read back from the
#        real compiled firmware under the simulator. MPLAB SIM models
#        no USB SIE (no bus, no host, no token engine), so this proves
#        the SIE is programmed and the buffers are in USB RAM, and
#        proves nothing about enumeration.
#   hw   The hardware gate (run by hand, needs a board): the device
#        enumerates as a CDC-ACM port, the host opens it, and the
#        bytes written come back byte-exact. This is the enumeration
#        evidence.
#
# The sim half runs inside the toolchain container (mdb.sh lives
# there); the hw half runs on the host, because the board is plugged
# into the host's USB bus, which the container cannot see.
#
# Usage: usb-bringup-gate.sh sim <module> <mcu> <device>
#        usb-bringup-gate.sh hw [--timeout <seconds>]
#
# Exit codes: 0 pass, 1 fail, 2 skipped (the hw half with no board, or
# without pyserial installed).

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
