#!/usr/bin/env bash
# Build a manifest module's sim variant under both XC8 and epic-cc,
# compare flash/RAM size, and diff the mdb-captured UART behavioral
# trace between the two builds. Not specific to epic-menu-demo: takes
# any module/MCU/device with a sim variant. Local dev tool: needs
# `make image` (XC8 + mdb) built once, and EPIC_CC_BIN pointing at a
# freshly-built epic-cc release binary, not a stale cached one
# (epic-hal#240 documents the trap and the build-inside-the-container
# recipe).
#
# Usage: compare-toolchains.sh <module> <mcu> <device> [wait_ms] [eeprom_writes]
#   e.g. scripts/compare-toolchains.sh epic-menu-demo 18F4550 PIC18F4550 60000 4

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

module="${1:?usage: compare-toolchains.sh <module> <mcu> <device> [wait_ms] [eeprom_writes]}"
mcu="${2:?usage: compare-toolchains.sh <module> <mcu> <device> [wait_ms] [eeprom_writes]}"
device="${3:?usage: compare-toolchains.sh <module> <mcu> <device> [wait_ms] [eeprom_writes]}"
wait_ms="${4:-60000}"
eeprom_writes="${5:-0}"

local_image="epic-hal-toolchain:local"
epic_cc_image="${EPIC_CC_IMAGE:-epic-cc-dev:local}"
epic_cc_bin="${EPIC_CC_BIN:-/tmp/cargo-target/release/epic-cc}"
home_mount="$HOME/.cache/epic-hal-toolchain-home"
cargo_cache="$HOME/.cache/epic-cc/target"
mkdir -p "$home_mount" "$cargo_cache"

hal_run() {
  docker run --rm --user "$(id -u):$(id -g)" \
    -v /etc/passwd:/etc/passwd:ro -v /etc/group:/etc/group:ro \
    -v "$home_mount:$HOME" \
    -v "$repo_root:/repo" -w /repo \
    "$local_image" "$@"
}

epiccc_run() {
  docker run --rm --user "$(id -u):$(id -g)" \
    -v /etc/passwd:/etc/passwd:ro -v /etc/group:/etc/group:ro \
    -v "$home_mount:$HOME" \
    -v "$cargo_cache:/tmp/cargo-target" \
    -v "$repo_root:/repo" -w /repo \
    -e PIC8_CLANG_UNWRAPPED=/opt/clang/bin/clang \
    -e PIC8_CLANG_RESOURCE_DIR=/opt/clang/lib/clang/20 \
    "$epic_cc_image" "$@"
}

xc8_version="$(grep -m1 '^ARG XC8_VERSION=' docker/ci-toolchain/Dockerfile | cut -d= -f2)"
dfp_dir="$(python3 -c "
import sys; sys.path.insert(0, 'scripts')
import epicmanifest as e
m = e.load(e.default_path())
fam = m.family_of('${mcu}')
print('/opt/microchip/xc8/v${xc8_version}/pic/packs/' + fam.dfp + '/xc8')
")"

xc8_dir="build-cmp/${module}-xc8"
epiccc_dir="build-cmp/${module}-epiccc"
mkdir -p "$xc8_dir" "$epiccc_dir"

echo "== XC8 build (${module} ${mcu}, sim variant) ==" >&2
python3 scripts/epic_build.py build --module "$module" --mcu "$mcu" \
  --variant sim --dfp-dir "$dfp_dir" --build-dir "$xc8_dir"
xc8_log="$(hal_run sh "${xc8_dir}/${mcu}/build.sh" 2>&1)"
echo "$xc8_log"
xc8_hex="$(ls "${xc8_dir}/${mcu}"-*.hex)"

echo "== epic-cc build (${module} ${mcu}, sim variant) ==" >&2
python3 scripts/epic_build.py build --module "$module" --mcu "$mcu" \
  --variant sim --toolchain epic-cc --epic-cc "$epic_cc_bin" \
  --build-dir "$epiccc_dir"
# A failed epic-cc build (compiler panic, flash overflow, ...) is an
# expected, valid outcome this script must report, not abort on: don't
# let set -e kill the script on this specific command's exit status.
epiccc_log="$(epiccc_run sh "${epiccc_dir}/${mcu}/build.sh" 2>&1)" || true
echo "$epiccc_log"
epiccc_hex="$(ls "${epiccc_dir}/${mcu}"-*.hex 2>/dev/null | head -1 || true)"

echo ""
echo "== Size comparison (both figures in flash words) =="
echo "(scripts/epic_build.py's parse_memory_summary already converts XC8's"
echo "byte-addressed PIC18 'Program space' figure to words; epic-cc reports"
echo "words directly.)"

# Logs go through temp files, not shell-quoted into a python -c string:
# XC8's warnings are full of quotes and other characters that make
# inline string interpolation into a python literal unreliable.
xc8_log_file="build-cmp/${module}-xc8-build.log"
epiccc_log_file="build-cmp/${module}-epiccc-build.log"
printf '%s' "$xc8_log" > "$xc8_log_file"
printf '%s' "$epiccc_log" > "$epiccc_log_file"

xc8_usage="$(python3 -c "
import sys
sys.path.insert(0, 'scripts')
import epic_build
with open('${xc8_log_file}') as f:
    log = f.read()
u = epic_build.parse_memory_summary(log)
print('' if u is None else f\"{u['flash_words']} {u['flash_total_words']} {u['ram_bytes']} {u['ram_total_bytes']}\")
")"

# epic-cc's own stdout: "flash: X/Y words (Z%)" / "RAM: X/Y bytes (Z%) ...".
epiccc_usage="$(python3 -c "
import re
with open('${epiccc_log_file}') as f:
    log = f.read()
f = re.search(r'flash:\s*(\d+)/(\d+)\s*words', log)
r = re.search(r'RAM:\s*(\d+)/(\d+)\s*bytes', log)
print('' if not (f and r) else f'{f.group(1)} {f.group(2)} {r.group(1)} {r.group(2)}')
")"

printf '%-10s %-14s %-10s %-10s %-10s\n' "toolchain" "flash(words)" "flash%" "ram" "ram%"
if [ -n "$xc8_usage" ]; then
  read -r fw ft rb rt <<<"$xc8_usage"
  printf '%-10s %-14s %-10s %-10s %-10s\n' "xc8" "${fw}/${ft}" "$(awk -v a="$fw" -v b="$ft" 'BEGIN{printf "%.1f%%",100*a/b}')" "${rb}/${rt}" "$(awk -v a="$rb" -v b="$rt" 'BEGIN{printf "%.1f%%",100*a/b}')"
else
  echo "xc8: (no Memory Summary found in build output)"
fi
if [ -n "$epiccc_usage" ]; then
  read -r fw ft rb rt <<<"$epiccc_usage"
  printf '%-10s %-14s %-10s %-10s %-10s\n' "epic-cc" "${fw}/${ft}" "$(awk -v a="$fw" -v b="$ft" 'BEGIN{printf "%.1f%%",100*a/b}')" "${rb}/${rt}" "$(awk -v a="$rb" -v b="$rt" 'BEGIN{printf "%.1f%%",100*a/b}')"
else
  echo "epic-cc: (build failed or produced no size report -- see log above)"
fi

if [ -z "$epiccc_hex" ]; then
  echo ""
  echo "== Behavioral comparison: skipped (no epic-cc hex; build failed above) =="
  exit 1
fi

echo ""
echo "== Behavioral comparison (real MPLAB SIM, UART trace) =="

xc8_capture="build-cmp/${module}-xc8-uart.txt"
epiccc_capture="build-cmp/${module}-epiccc-uart.txt"
rm -f "$xc8_capture" "$epiccc_capture"

echo "-- running XC8 hex under mdb --" >&2
hal_run scripts/mdb-hex-run.sh "$xc8_hex" "$device" "$wait_ms" "" "$xc8_capture" "$eeprom_writes" >&2

echo "-- running epic-cc hex under mdb --" >&2
hal_run scripts/mdb-hex-run.sh "$epiccc_hex" "$device" "$wait_ms" "" "$epiccc_capture" "$eeprom_writes" >&2

if [ ! -s "$xc8_capture" ] || [ ! -s "$epiccc_capture" ]; then
  echo "::error::one or both mdb runs produced no UART capture" >&2
  exit 1
fi

if diff -u "$xc8_capture" "$epiccc_capture"; then
  echo "IDENTICAL: XC8 and epic-cc UART traces match byte-for-byte."
else
  echo ""
  echo "DIFFERS: XC8 and epic-cc UART traces diverge (see diff above)."
  exit 1
fi
