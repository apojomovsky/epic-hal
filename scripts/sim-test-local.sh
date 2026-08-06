#!/usr/bin/env bash
# Local reproduction of .github/workflows/sim-tests.yml's sim-test job,
# for fast iteration without a GitHub Actions round trip (docs/ci-
# plan.md's local-reproduction plan). Resolves the HARNESS=sim build
# script on the host (needs python3, see scripts/epic_build.py), pulls
# the exact same private GHCR toolchain image xc8-build.yml/sim-
# tests.yml build+cache (same tag formula, read straight from
# docker/ci-toolchain/Dockerfile's ARGs, so a cache hit is expected:
# this never rebuilds the image locally), then runs
# scripts/sim-mdb-run.sh inside it, bind-mounting this repo so the
# .hex output lands in the real working tree (inspectable after,
# gitignored either way). sim-mdb-run.sh only ever executes a
# pre-emitted script plus mdb.sh, since the container deliberately has
# no python3.
#
# Needs `docker login ghcr.io` first, with a PAT that has at least
# read:packages (this repo's GHCR packages are private, see
# docs/ci-plan.md's redistribution-fix account for why).
#
# Usage: sim-test-local.sh <family> <mcu> <device> <module> [wait_ms] [mode]
# Example (the epic-tick pilot, PIC16 side):
#   scripts/sim-test-local.sh pic16f87xa 16F877A PIC16F877A epic-tick

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

family="$1"; mcu="$2"; device="$3"; module="$4"

xc8_version="$(grep -m1 '^ARG XC8_VERSION=' docker/ci-toolchain/Dockerfile | cut -d= -f2)"
pic16_dfp_version="$(grep -m1 '^ARG PIC16FXXX_DFP_VERSION=' docker/ci-toolchain/Dockerfile | cut -d= -f2)"
pic18_dfp_version="$(grep -m1 '^ARG PIC18FXXXX_DFP_VERSION=' docker/ci-toolchain/Dockerfile | cut -d= -f2)"
pic1216f1_dfp_version="$(grep -m1 '^ARG PIC12_16F1XXX_DFP_VERSION=' docker/ci-toolchain/Dockerfile | cut -d= -f2)"
mplabx_version="$(grep -m1 '^ARG MPLABX_VERSION=' docker/ci-toolchain/Dockerfile | cut -d= -f2)"
tag="xc8-v${xc8_version}-dfp${pic16_dfp_version}-${pic18_dfp_version}-${pic1216f1_dfp_version}-mplabx${mplabx_version}"

owner="$(git remote get-url origin | sed -E 's#.*[:/]([^/]+)/[^/]+(\.git)?$#\1#')"
image="ghcr.io/${owner}/pic8-hal-ci:${tag}"

xc8_install_dir="/opt/microchip/xc8/v${xc8_version}"
dfp_dir="$(python3 -c "
import sys; sys.path.insert(0, 'scripts')
import epicmanifest as e
m = e.load(e.default_path())
fam = m.family_of('${mcu}')
print('${xc8_install_dir}/pic/packs/' + fam.dfp + '/xc8')
")"

echo "Emitting the HARNESS=sim build script for ${module} ${mcu} (host, python3)..." >&2
python3 scripts/epic_build.py build --module "$module" --mcu "$mcu" \
  --variant sim --build-dir "build-sim/${module}" --dfp-dir "$dfp_dir"

echo "Pulling ${image} (same tag CI resolves, cache hit expected)..." >&2
docker pull "$image"

echo "Running: scripts/sim-mdb-run.sh $* (inside the container)" >&2
docker run --rm \
  -v "$repo_root:/repo" -w /repo \
  "$image" \
  scripts/sim-mdb-run.sh "$@"
