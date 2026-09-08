#!/usr/bin/env bash
# One-command XC8 flash/RAM size baseline for epic-cc#200's reference
# table (epic-hal#122). Builds a manifest module+MCU through the existing
# `make xc8-build` path and prints the Memory Summary in the shape
# epic-cc's xc8_reference.md columns expect, so re-baselining is a single
# command instead of manual .hex/map digging.
#
# Usage: scripts/xc8-size-baseline.sh <module> <mcu>
#   e.g. scripts/xc8-size-baseline.sh epic-encoder 16F877A
#   scripts/xc8-size-baseline.sh --log-file build/16F877A/build.log
#
# The build runs in the docker toolchain image (make xc8-build owns the
# plumbing); this script only captures its output and parses the summary.
# --log-file skips the build and re-formats an existing build log, which
# is also how the formatting is tested without a toolchain.
set -euo pipefail

log_file=""
if [ "$1" = "--log-file" ]; then
    log_file="$2"
    shift 2
fi

if [ "$#" -ne 2 ] && [ -z "$log_file" ]; then
    echo "usage: $0 <module> <mcu>" >&2
    echo "  or:  $0 --log-file <path>" >&2
    echo "  e.g. $0 epic-encoder 16F877A" >&2
    exit 2
fi

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$repo_root"

if [ -n "$log_file" ]; then
    out="$(cat "$log_file")"
else
    module="$1"
    mcu="$2"
    # Capture the full build output so the Memory Summary can be parsed
    # from it. make xc8-build streams the xc8-cc link step's summary.
    out="$(make xc8-build MODULE="$module" MCU="$mcu" 2>&1)"
fi

# Reuse epic_build's parser so the regex has one home.
usage="$(python3 -c "
import sys
sys.path.insert(0, 'scripts')
import epic_build
u = epic_build.parse_memory_summary(sys.stdin.read())
if u is None:
    sys.exit('no Memory Summary found in build output')
print(u['flash_words'], u['flash_total_words'], u['ram_bytes'], u['ram_total_bytes'])
" <<<"$out")" || {
    echo "$out" >&2
    exit 1
}

read -r flash_words flash_total ram_bytes ram_total <<<"$usage"
flash_pct=$(awk -v a="$flash_words" -v b="$flash_total" 'BEGIN { printf "%.1f", 100*a/b }')
ram_pct=$(awk -v a="$ram_bytes" -v b="$ram_total" 'BEGIN { printf "%.1f", 100*a/b }')

echo "flash: $flash_words/$flash_total words ($flash_pct%)"
echo "RAM: $ram_bytes/$ram_total bytes ($ram_pct%)"
