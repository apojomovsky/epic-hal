# Developing Epicurus

The toolchain, build, and CI workflows for working *on* Epicurus. If you
just want to use it in a project, see the [README](README.md): grab a
release bundle and go. This document is for contributors and for local
verification.

## Overview

- **Host simulation** is the fast inner loop: every module builds as a
  host program under CMake/ctest, so logic gets exercised before it
  touches a programmer.
- **Real-target builds** cross-compile with XC8, driven by a manifest
  (`epic-common/manifest/modules.toml`) rather than per-module
  Makefiles.
- **The mdb gate** runs the real compiled firmware headlessly under
  MPLAB SIM and checks actual register and UART output.
- **Docker** wraps all of the above in one toolchain image, so nothing
  but two vendor installer files needs to be installed by hand.

## Native toolchain

`./scripts/bootstrap.sh` sets up a fresh clone: installs the host
toolchain the CMake builds need and a pre-commit hook (trailing
newline/whitespace, no em-dash, `cppcheck` on staged `.c` files), then
verifies the Docker toolchain: it checks Docker is installed and
reachable, handles the two Microchip installer files
(`docker/ci-toolchain/vendor/`) self-instructively, and builds the
toolchain image once they are in place. `--check-only` reports what's
missing without installing anything. See
[scripts/README.md](scripts/README.md) for what the hook checks.

Docker is the default path for real-target work (next section); the
native exception path needs MPLAB X IDE and MPLAB XC8 (`xc8-cc`)
installed by hand (proprietary, license-gated): PIC18 additionally
needs the PIC18Fxxxx DFP, PIC16F193X the PIC12-16F1xxx DFP (neither
ships with XC8).

Build a real target:

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
python3 scripts/epic_build.py build --module epic-tick --mcu 16F877A --run
```

Run the mdb gate:

```sh
scripts/sim-mdb-run.sh pic16f87xa 16F877A PIC16F877A epic-tick
```

## Docker (no local installs)

The root `Makefile` runs the whole workflow, host tests, real-target
XC8 builds, the `mdb` verification gate, and a dev shell, inside one
Docker image:

```sh
make check-vendor    # one-time: tells you which 2 files to grab from Microchip
make image           # build the toolchain image locally (once; cached after)
make test            # host-sim build + test, every module
make test MODULE=epic-lcd   # ... or just one

make xc8-build MODULE=epic-tick MCU=16F877A   # real-target build
make mdb-test MODULE=epic-tick MCU=16F877A DEVICE=PIC16F877A  # the mdb gate

make shell           # interactive shell, repo mounted at /repo
```

The image tag is resolved from `docker/ci-toolchain/Dockerfile`'s ARG
lines as `xc8-v${XC8_VERSION}-dfp${PIC16}-${PIC18}-${PIC1216F1}-mplabx${MPLABX_VERSION}`;
`make ci-image-push` and the CI jobs reuse the same grep so the formula
cannot drift.

The tag formula above is the single source of truth for the image
version; the targets in this section are the full command reference.

## CI

CI runs two jobs on every push (`.github/workflows/ci.yml`):

- **host**: every module's CMake/ctest on a bare runner, plus the
  pre-commit checks.
- **target**: one Docker pull, then every real XC8 build across all
  three families, the mdb/MPLAB SIM runs, and the isolated bundle-gate
  build.

Maintainers with `write:packages` access to this repo's GHCR packages
can publish an updated toolchain image with `make ci-image-push
GHCR_OWNER=<owner>` (after `docker login ghcr.io`), the same private tag
CI's workflows pull from; CI itself never builds this image. See
[.github/workflows/ci.yml](.github/workflows/ci.yml) for the job
definitions and the consolidation tradeoff in its header comment.

## Releases

Tagging `v*` triggers `release-bundles.yml`: it builds one source bundle
per family, verifies checksums, gates every bundle from a scratch
directory outside any repo checkout, and only then attaches the
tarballs to a GitHub Release. See
[.github/workflows/release-bundles.yml](.github/workflows/release-bundles.yml).

## install.sh

The README's one-command getting started runs `install.sh` (repo root).
To check it locally, build a family bundle with its checksum, then point
the installer at the result:

```sh
python3 scripts/make_bundle.py --family PIC16F87XA --version ci-test
cd bundles && sha256sum ./*.tar.gz > SHA256SUMS && cd ..
EPICURUS_BASE_URL=file://$PWD/bundles sh install.sh pic16f87xa ci-test
```

`--family` takes the manifest name (`PIC16F87XA`, `PIC18Fxx5x`,
`PIC16F193X`); the bundle slug and the installer's family are the
lowercase form (`pic16f87xa`, `pic18fxx5x`, `pic16f193x`).
`EPICURUS_BASE_URL` is a flat asset dir, so `<version>` is required. The
release gate runs the same flow for all three families and builds the
scaffolds, see
[.github/workflows/release-bundles.yml](.github/workflows/release-bundles.yml).

## epicurus CLI

The `epicurus` CLI (`scripts/epicurus.py`, with `epicurus_init.py`,
`epicmanifest.py`, and `bundlegen.py`) scaffolds a consumer project from
a bundle: `main.c`, a filled `Makefile`, and a patched MPLAB X `.X` for
the chosen part and module subset. Run it from a checkout or an unpacked
bundle:

```sh
python3 scripts/epicurus.py init --bundle path/to/epicurus-pic16f87xa-v0.1.0
```

Its tests live with the rest of the scripts tests:

```sh
python3 scripts/tests/test_epicurus_init.py
```
