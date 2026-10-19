#!/usr/bin/env bash
# Local reproduction of the MPLAB SIM gate for fast iteration: emits the
# HARNESS=sim build script on the host (python3), pulls CI's toolchain
# image (tag formula read from the Dockerfile ARGs), then runs
# scripts/sim/sim-mdb-run.sh inside it with this repo bind-mounted
# (needs `docker login ghcr.io` with read:packages first).
# Usage: sim-test-local.sh <family> <mcu> <device> <module> [wait_ms] [mode]
#   e.g. scripts/sim/sim-test-local.sh pic16f87xa 16F877A PIC16F877A lib/tick
# A peer example passes as <module@example>, e.g. lib/lcd@gpio8, and the
# emit below splits it back into --module/--example.

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

family="$1"; mcu="$2"; device="$3"; module="$4"

xc8_version="$(grep -m1 '^ARG XC8_VERSION=' docker/ci-toolchain/Dockerfile | cut -d= -f2)"
pic16_dfp_version="$(grep -m1 '^ARG PIC16FXXX_DFP_VERSION=' docker/ci-toolchain/Dockerfile | cut -d= -f2)"
pic18_dfp_version="$(grep -m1 '^ARG PIC18FXXXX_DFP_VERSION=' docker/ci-toolchain/Dockerfile | cut -d= -f2)"
pic1216f1_dfp_version="$(grep -m1 '^ARG PIC12_16F1XXX_DFP_VERSION=' docker/ci-toolchain/Dockerfile | cut -d= -f2)"
pic16f1xxxx_dfp_version="$(grep -m1 '^ARG PIC16F1XXXX_DFP_VERSION=' docker/ci-toolchain/Dockerfile | cut -d= -f2)"
mplabx_version="$(grep -m1 '^ARG MPLABX_VERSION=' docker/ci-toolchain/Dockerfile | cut -d= -f2)"
tag="xc8-v${xc8_version}-dfp${pic16_dfp_version}-${pic18_dfp_version}-${pic1216f1_dfp_version}-${pic16f1xxxx_dfp_version}-mplabx${mplabx_version}"

owner="$(git remote get-url origin | sed -E 's#.*[:/]([^/]+)/[^/]+(\.git)?$#\1#')"
image="ghcr.io/${owner}/epic-hal-ci:${tag}"

xc8_install_dir="/opt/microchip/xc8/v${xc8_version}"
dfp_dir="$(python3 -c "
import sys; sys.path.insert(0, 'scripts/build')
import epicmanifest as e
m = e.load(e.default_path())
fam = m.family_of('${mcu}')
print('${xc8_install_dir}/pic/packs/' + e.dfp_name(fam) + '/xc8')
")"

echo "Emitting the HARNESS=sim build script for ${module} ${mcu} (host, python3)..." >&2
name="${module%%@*}"; example="${module#*@}"
extra_args=()
[ "$example" != "$module" ] && extra_args+=(--example "$example")
python3 scripts/build/epic_build.py build --module "$name" --mcu "$mcu" \
  --variant sim --build-dir "build-sim/${module}" --dfp-dir "$dfp_dir" \
  "${extra_args[@]:+${extra_args[@]}}"

echo "Pulling ${image} (same tag CI resolves, cache hit expected)..." >&2
docker pull "${image}"
echo "Running: scripts/sim/sim-mdb-run.sh $* (inside the container)" >&2
docker run --rm \
  -v "$repo_root:/repo" -w /repo \
  "$image" \
  scripts/sim/sim-mdb-run.sh "$@"
