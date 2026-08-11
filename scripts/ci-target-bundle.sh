#!/usr/bin/env bash
# Isolated bundle build for ci.yml's "target" job and the local
# `make target-ci` replica: extracts each generated bundle to /isolated
# (no repo above it) and builds there, so a missing file cannot resolve
# through this checkout's own sibling layout; also builds each bundle's
# reference MPLAB X project headlessly (prjMakefilesGenerator.sh + make).
# Needs $XC8_INSTALL_DIR/$MPLABX_INSTALL_DIR (set by the toolchain image's
# ENV). Exits 1 if anything failed.
#
# Usage: ci-target-bundle.sh [bundles-dir] [summary.md]

set -uo pipefail

bundles_dir="${1:-bundles}"
summary="${2:-ci-summary-bundle.md}"
repo_root="$PWD"

fail=0
{
  echo "| Bundle | Module | Part | Result |"
  echo "|---|---|---|---|"
} > "$summary"

mkdir -p /isolated

for tarball in "$repo_root/$bundles_dir"/*.tar.gz; do
  name="$(basename "$tarball" .tar.gz)"
  rm -rf "/isolated/$name"
  tar xzf "$tarball" -C /isolated
  cd "/isolated/$name"

  # Reference MPLAB X project. Built headlessly via the same generated
  # makefile MPLAB X itself uses; see
  # docs/superpowers/plans/probe-mplabx-headless.md for why this is
  # possible in this image. Runs for every bundle, including HAL-only
  # ones, since every family ships one regardless of whether it has a
  # higher-level module to link.
  if [ -d examples/epicurus-demo.X ]; then
    (
      cd examples/epicurus-demo.X
      "$MPLABX_INSTALL_DIR/mplab_platform/bin/prjMakefilesGenerator.sh" . \
        >/dev/null 2>&1 || true
      if make -f nbproject/Makefile-default.mk SUBPROJECTS= .build-conf \
           >project.log 2>&1; then
        echo "| $name | epicurus-demo.X | (project) | PASS |" >> "$repo_root/$summary"
      else
        echo "| $name | epicurus-demo.X | (project) | FAIL |" >> "$repo_root/$summary"
        echo "::group::$name project log"; cat project.log; echo "::endgroup::"
        exit 1
      fi
    ) || fail=1
  else
    echo "| $name | epicurus-demo.X | (project) | FAIL: missing |" >> "$repo_root/$summary"
    fail=1
  fi

  # Pick the first supported (module, part) pair straight out of the
  # bundle's own SUPPORT.md table, so this tests what a consumer would
  # actually be told to do.
  module=""
  part=""
  while IFS= read -r row; do
    case "$row" in
      '| `epic-'*)
        m="$(echo "$row" | sed 's/^| `\([^`]*\)`.*/\1/' | sed 's/^epic-//')"
        col=0
        IFS='|' read -ra cells <<< "$row"
        for cell in "${cells[@]:2}"; do
          trimmed="$(echo "$cell" | tr -d ' ')"
          if [ "$trimmed" = "yes" ]; then
            part="$(sed -n 's/^| Module | //p' SUPPORT.md \
              | head -1 | tr -d ' ' | cut -d'|' -f$((col + 1)))"
            module="$m"
            break
          fi
          col=$((col + 1))
        done
        ;;
    esac
    [ -n "$module" ] && break
  done < SUPPORT.md

  if [ -z "$module" ]; then
    echo "| $name | (none) | (none) | SKIP: HAL-only bundle |" >> "$repo_root/$summary"
    cd /isolated
    continue
  fi

  dfp_name="$(grep -m1 '^EPICURUS_DFP' epicurus.mk | awk '{print $3}')"
  cat > Makefile <<EOF
EPICURUS_DIR := .
EPICURUS_MCU := $part
EPICURUS_MODULES := $module
include \$(EPICURUS_DIR)/epicurus.mk
DFP := $XC8_INSTALL_DIR/pic/packs/$dfp_name/xc8
all:
	xc8-cc -mdfp=\$(DFP) -mcpu=\$(shell echo \$(EPICURUS_MCU) | tr A-Z a-z) \\
	  -O2 -std=c99 -Wall -Wextra \$(EPICURUS_CFLAGS) -DFOSC_HZ=20000000 \\
	  \$(EPICURUS_SRCS) main.c -o app.hex -ginhx32
EOF
  printf 'void main(void) { for (;;) { } }\n' > main.c

  if make >build.log 2>&1 && [ -f app.hex ]; then
    echo "| $name | $module | $part | PASS |" >> "$repo_root/$summary"
  else
    echo "| $name | $module | $part | FAIL |" >> "$repo_root/$summary"
    echo "::group::$name build log"; cat build.log; echo "::endgroup::"
    fail=1
  fi
  cd /isolated
done

exit "$fail"
