#!/usr/bin/env bash
# Real-target XC8 build loop, driven by matrix.txt (one "family module
# mcu" triple per line, emitted by ci.yml's "target" job before this
# script runs, since building that list needs python3/the manifest and
# this script runs inside the toolchain container, which has neither
# python3 nor jq, see docker/ci-toolchain/Dockerfile). Each pre-emitted
# build.sh already has its DFP path baked in from emission time, so this
# script never touches DFP_DIR itself.
#
# Usage: ci-target-build.sh [matrix.txt] [summary.md]
#   matrix.txt   default: matrix.txt (in cwd)
#   summary.md   default: ci-summary-build.md (in cwd); ci.yml cats this
#                into $GITHUB_STEP_SUMMARY after this script returns,
#                since GITHUB_STEP_SUMMARY is a path on the runner, not
#                reliably reachable from inside this container.
#
# Does not stop at the first failure (fail-fast:false equivalent): a
# broken module doesn't hide results for the rest of the family, a
# broken family doesn't hide the rest. Exits 1 if anything failed.

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

while read -r family name mcu; do
  [ -z "$family" ] && continue
  echo "::group::${family} ${name} MCU=${mcu}"
  rm -f "build/${name}/${mcu}"/*.p1
  if EPIC_REPO_ROOT="$PWD" sh "build/${name}/${mcu}/build.sh" \
     && [ "$(ls "build/${name}/${mcu}"-*.hex 2>/dev/null | wc -l)" -eq 1 ]; then
    echo "PASS: ${family} ${name} ${mcu}"
    echo "| ${family} | ${name} | ${mcu} | PASS |" >> "$summary"
  else
    echo "FAIL: ${family} ${name} ${mcu}"
    echo "| ${family} | ${name} | ${mcu} | FAIL |" >> "$summary"
    fail=1
  fi
  echo "::endgroup::"
done < "$matrix"

exit "$fail"
