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
#
# A second, optional file of the same "dir=mcu,mcu;..." pairs, named by
# $SIM_PAIRS_FILE, is checked the same way but against the Makefiles'
# HARNESS=sim build (sim-tests.yml's variant) instead of the default
# HARNESS=target one, and the new path's --variant sim. Only the three
# (module, MCU) combinations sim-tests.yml actually drives have a sim
# variant, hence a separate, much shorter list rather than folding this
# into every target pair above.
set -eu

fail=0
pass=0

check_pair() {
  # $1=dir $2=mcu $3=variant ("target" or "sim")
  dir="$1"; mcu="$2"; variant="$3"
  module="$(echo "$dir" | cut -d/ -f1)"

  case "$dir" in
    *pic16f87xa*) dfp="Microchip.PIC16Fxxx_DFP" ;;
    *pic18fxx5x*) dfp="Microchip.PIC18Fxxxx_DFP" ;;
    *pic16f193x*) dfp="Microchip.PIC12-16F1xxx_DFP" ;;
    *) echo "unrecognised family for $dir" >&2; exit 1 ;;
  esac
  dfp_dir="$DFP_ROOT/$dfp/xc8"

  echo "=== $module $mcu (variant=$variant) ==="

  # Old path: the Makefile, untouched. HARNESS=sim for the sim variant,
  # matching sim-tests.yml's own build invocation.
  make -C "$dir" clean >/dev/null 2>&1 || true
  if [ "$variant" = "sim" ]; then
    old_ok=0
    make -C "$dir" MCU="$mcu" HARNESS=sim DFP_DIR="$dfp_dir" >/dev/null 2>&1 && old_ok=1 || true
  else
    old_ok=0
    make -C "$dir" MCU="$mcu" DFP_DIR="$dfp_dir" >/dev/null 2>&1 && old_ok=1 || true
  fi
  if [ "$old_ok" -ne 1 ]; then
    echo "SKIP $module $mcu $variant (Makefile build fails; excluded pair)"
    return
  fi
  old_hex="$(ls "$dir"/build/"$mcu"-*.hex 2>/dev/null | head -1)"
  if [ -z "$old_hex" ]; then
    echo "FAIL $module $mcu $variant (Makefile produced no .hex)"
    fail=$((fail + 1))
    return
  fi
  cp "$old_hex" "/tmp/old-$module-$mcu-$variant.hex"

  # New path: the pre-emitted manifest build script. Note the per-module
  # build dir: two modules built for the same part with a shared
  # --build-dir would overwrite each other's build.sh, and the sim
  # variant needs its own build dir too (not just a different .hex
  # basename): its generated config_<mcu>.c has different pragmas than
  # the target variant's, and both would otherwise collide on the same
  # path within a shared build dir.
  build_key="$module"
  [ "$variant" = "sim" ] && build_key="$module-sim"
  new_script="build-manifest/$build_key/$mcu/build.sh"
  if [ ! -f "$new_script" ]; then
    echo "FAIL $module $mcu $variant (no emitted script at $new_script)"
    fail=$((fail + 1))
    return
  fi
  rm -rf "build-manifest/$build_key/$mcu"/*.p1
  if ! EPIC_REPO_ROOT="$PWD" sh "$new_script" >"/tmp/new-$module-$mcu-$variant.log" 2>&1; then
    echo "FAIL $module $mcu $variant (manifest build failed)"
    tail -20 "/tmp/new-$module-$mcu-$variant.log"
    fail=$((fail + 1))
    return
  fi
  new_hex="$(ls build-manifest/"$build_key"/"$mcu"-*.hex 2>/dev/null | head -1)"
  if [ -z "$new_hex" ]; then
    echo "FAIL $module $mcu $variant (manifest build produced no .hex)"
    fail=$((fail + 1))
    return
  fi

  if cmp -s "/tmp/old-$module-$mcu-$variant.hex" "$new_hex"; then
    echo "PASS $module $mcu $variant (byte-identical)"
    pass=$((pass + 1))
  else
    echo "FAIL $module $mcu $variant (.hex differs)"
    echo "  old: $old_hex"
    echo "  new: $new_hex"
    fail=$((fail + 1))
  fi
}

process_pairs() {
  # $1=variant, reads "dir=mcu,mcu;..." lines from stdin
  variant="$1"
  while IFS= read -r line; do
    [ -n "$line" ] || continue
    dir="${line%%=*}"
    mcus="${line#*=}"
    old_ifs="$IFS"; IFS=','
    for mcu in $mcus; do
      IFS="$old_ifs"
      check_pair "$dir" "$mcu" "$variant"
    done
    IFS="$old_ifs"
  done
}

process_pairs target

if [ -n "${SIM_PAIRS_FILE:-}" ] && [ -f "$SIM_PAIRS_FILE" ]; then
  process_pairs sim < "$SIM_PAIRS_FILE"
fi

echo ""
echo "equivalence: $pass identical, $fail differing"
[ "$fail" -eq 0 ]
