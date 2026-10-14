#!/usr/bin/env bash
# Real-target XC8 build loop for ci.yml's "target" job and the local
# `make target-ci` replica: reads matrix.txt (emitted beforehand, since
# this script runs in the toolchain container with no python3/jq) and runs
# each pre-emitted build.sh, whose DFP path was baked in at emission time.
# Does not stop at the first failure (fail-fast:false equivalent); exits 1
# if anything failed.
#
# Usage: ci-target-build.sh [matrix.txt] [summary.md]

set -uo pipefail

matrix="${1:-matrix.txt}"
summary="${2:-ci-summary-build.md}"

if [ ! -f "$matrix" ]; then
  echo "::error::no ${matrix}; the emit step must run before this script" >&2
  exit 1
fi

fail=0
{
  echo "| Family | Module | MCU | Result |"
  echo "|---|---|---|---|"
} > "$summary"

# PARALLEL=N (default 1) runs N builds concurrently. The runner has 2
# cores and each xc8 build is single-threaded, so -P2 nearly halves
# this loop; build/$(name)/$(mcu) dirs are disjoint so no collision.
PARALLEL="${PARALLEL:-1}"

build_one() {
  family="$1"; name="$2"; mcu="$3"
  echo "::group::${family} ${name} MCU=${mcu}"
  rm -f "build/${name}/${mcu}"/*.p1
  if EPIC_REPO_ROOT="$PWD" sh "build/${name}/${mcu}/build.sh" \
     && [ "$(ls "build/${name}/${mcu}"-*.hex 2>/dev/null | wc -l)" -eq 1 ]; then
    echo "PASS: ${family} ${name} ${mcu}"
    echo "| ${family} | ${name} | ${mcu} | PASS |" >> "$summary"
  else
    echo "FAIL: ${family} ${name} ${mcu}"
    echo "| ${family} | ${name} | ${mcu} | FAIL |" >> "$summary"
    return 1
  fi
  echo "::endgroup::"
}

if [ "$PARALLEL" -gt 1 ]; then
  # Parallel unit is the MODULE, not the (module, mcu) leg: XC8 writes
  # its auto-generated startup object (startup.rlf/.s) next to the -o
  # hex at build/<module>/, so concurrent legs of one module corrupt
  # each other's startup file (observed: 505/380/876 errors on the
  # 193X fsm legs at -P2). The legs of one module run sequentially
  # inside its unit; different modules run concurrently.
  module_one() {
    module="$1"
    local fail_here=0
    while read -r family name mcu; do
      [ -z "$family" ] && continue
      build_one "$family" "$name" "$mcu" || fail_here=1
    done < <(awk -v mod="$module" '$2 == mod' "$matrix")
    return "$fail_here"
  }
  export -f build_one module_one
  export summary matrix
  if ! awk 'NF {print $2}' "$matrix" | sort -u \
       | xargs -P"$PARALLEL" -n1 bash -c 'module_one "$@"' _; then
    fail=1
  fi
else
  while read -r family name mcu; do
    [ -z "$family" ] && continue
    build_one "$family" "$name" "$mcu" || fail=1
  done < "$matrix"
fi

exit "$fail"
