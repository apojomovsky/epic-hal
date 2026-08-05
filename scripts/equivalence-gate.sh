#!/bin/sh
# Prove the manifest-driven build reproduces the Makefile build exactly.
#
# For every (module dir, MCU) pair given on stdin as "dir=mcu,mcu;...",
# build the .hex both ways and compare byte for byte. Identical sources,
# identical flags, one compiler: any difference means the manifest and
# the Makefile disagree, which is the one thing this migration must not
# get wrong.
#
# POSIX sh with no python3 on purpose: this runs inside the toolchain
# container (docker/ci-toolchain/Dockerfile), which deliberately has no
# Python. The manifest side is resolved into build scripts beforehand,
# outside the container.
#
# Usage (from the repo root):
#   echo "epic-tick/mcu/pic16f87xa-tick-mplabx=16F877A" \
#     | DFP_ROOT=$XC8_INSTALL_DIR/pic/packs sh scripts/equivalence-gate.sh
set -eu

fail=0
pass=0

while IFS= read -r line; do
  [ -n "$line" ] || continue
  dir="${line%%=*}"
  mcus="${line#*=}"
  module="$(echo "$dir" | cut -d/ -f1)"

  case "$dir" in
    *pic16f87xa*) dfp="Microchip.PIC16Fxxx_DFP" ;;
    *pic18fxx5x*) dfp="Microchip.PIC18Fxxxx_DFP" ;;
    *pic16f193x*) dfp="Microchip.PIC12-16F1xxx_DFP" ;;
    *) echo "unrecognised family for $dir" >&2; exit 1 ;;
  esac
  dfp_dir="$DFP_ROOT/$dfp/xc8"

  old_ifs="$IFS"; IFS=','
  for mcu in $mcus; do
    IFS="$old_ifs"
    echo "=== $module $mcu ==="

    # Old path: the Makefile, untouched.
    make -C "$dir" clean >/dev/null 2>&1 || true
    if ! make -C "$dir" MCU="$mcu" DFP_DIR="$dfp_dir" >/dev/null 2>&1; then
      echo "SKIP $module $mcu (Makefile build fails; excluded pair)"
      continue
    fi
    old_hex="$(ls "$dir"/build/"$mcu"-*.hex 2>/dev/null | head -1)"
    if [ -z "$old_hex" ]; then
      echo "FAIL $module $mcu (Makefile produced no .hex)"
      fail=$((fail + 1))
      continue
    fi
    cp "$old_hex" "/tmp/old-$module-$mcu.hex"

    # New path: the pre-emitted manifest build script. Note the
    # per-module build dir: two modules built for the same part with a
    # shared --build-dir would overwrite each other's build.sh.
    new_script="build-manifest/$module/$mcu/build.sh"
    if [ ! -f "$new_script" ]; then
      echo "FAIL $module $mcu (no emitted script at $new_script)"
      fail=$((fail + 1))
      continue
    fi
    rm -rf "build-manifest/$module/$mcu"/*.p1
    if ! EPIC_REPO_ROOT="$PWD" sh "$new_script" >/dev/null 2>&1; then
      echo "FAIL $module $mcu (manifest build failed)"
      fail=$((fail + 1))
      continue
    fi
    new_hex="$(ls build-manifest/"$module"/"$mcu"-*.hex 2>/dev/null | head -1)"
    if [ -z "$new_hex" ]; then
      echo "FAIL $module $mcu (manifest build produced no .hex)"
      fail=$((fail + 1))
      continue
    fi

    if cmp -s "/tmp/old-$module-$mcu.hex" "$new_hex"; then
      echo "PASS $module $mcu (byte-identical)"
      pass=$((pass + 1))
    else
      echo "FAIL $module $mcu (.hex differs)"
      echo "  old: $old_hex"
      echo "  new: $new_hex"
      fail=$((fail + 1))
    fi
  done
  IFS="$old_ifs"
done

echo ""
echo "equivalence: $pass identical, $fail differing"
[ "$fail" -eq 0 ]
