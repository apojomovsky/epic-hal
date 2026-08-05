#!/usr/bin/env bash
# Local reproduction of .github/workflows/sim-tests.yml's sim-test job,
# for fast iteration without a GitHub Actions round trip (docs/ci-
# plan.md's local-reproduction plan). Pulls the exact same private GHCR
# toolchain image xc8-build.yml/sim-tests.yml build+cache (same tag
# formula, read straight from docker/ci-toolchain/Dockerfile's ARGs, so
# a cache hit is expected: this never rebuilds the image locally), and
# runs scripts/sim-mdb-run.sh inside it, bind-mounting this repo so
# make's .hex output lands in the real working tree (inspectable after,
# gitignored either way).
#
# Needs `docker login ghcr.io` first, with a PAT that has at least
# read:packages (this repo's GHCR packages are private, see
# docs/ci-plan.md's redistribution-fix account for why).
#
# Usage: sim-test-local.sh <family> <mcu> <device> <dir> <dfp> [wait_ms]
# Example (the epic-tick pilot, PIC16 side):
#   scripts/sim-test-local.sh pic16f87xa 16F877A PIC16F877A \
#     epic-tick/mcu/pic16f87xa-tick-mplabx Microchip.PIC16Fxxx_DFP

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

xc8_version="$(grep -m1 '^ARG XC8_VERSION=' docker/ci-toolchain/Dockerfile | cut -d= -f2)"
pic16_dfp_version="$(grep -m1 '^ARG PIC16FXXX_DFP_VERSION=' docker/ci-toolchain/Dockerfile | cut -d= -f2)"
pic18_dfp_version="$(grep -m1 '^ARG PIC18FXXXX_DFP_VERSION=' docker/ci-toolchain/Dockerfile | cut -d= -f2)"
mplabx_version="$(grep -m1 '^ARG MPLABX_VERSION=' docker/ci-toolchain/Dockerfile | cut -d= -f2)"
tag="xc8-v${xc8_version}-dfp${pic16_dfp_version}-${pic18_dfp_version}-mplabx${mplabx_version}"

owner="$(git remote get-url origin | sed -E 's#.*[:/]([^/]+)/[^/]+(\.git)?$#\1#')"
image="ghcr.io/${owner}/pic8-hal-ci:${tag}"

echo "Pulling ${image} (same tag CI resolves, cache hit expected)..." >&2
docker pull "$image"

echo "Running: scripts/sim-mdb-run.sh $* (inside the container)" >&2
docker run --rm \
  -v "$repo_root:/repo" -w /repo \
  "$image" \
  scripts/sim-mdb-run.sh "$@"
