# Developing Epic HAL

The toolchain, build, and CI workflows for working *on* Epic HAL. If you
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
toolchain the CMake builds need and the git hooks (`pre-commit`:
trailing newline/whitespace, no em-dash, `cppcheck` on staged `.c`
files; `commit-msg`: no attribution trailers, no em-dash), then
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

## Worktrees and the pre-PR ritual

Feature work happens in a worktree under `.worktrees/`, never on
`master` (AGENTS.md's "Worktrees"):

```sh
git fetch origin master
git worktree add .worktrees/<name> -b feat/<description> origin/master

make setup-hooks     # once per clone; the hooks dir is shared by all worktrees
make pre-pr-check    # the gate, before opening the PR
```

The toolchain image is shared by every worktree (it is a docker tag, not
a file in the tree) and `make check-vendor` hard-links the gitignored
vendor installers in from the main checkout, so container targets work
from a worktree with no extra setup.

`make pre-pr-check` checks the whole `origin/master...HEAD` range where
the pre-commit hook only sees one commit's staged content: plan docs
that must not reach master, commit hygiene, whitespace, em-dashes,
docstring compliance on the C files touched, and the comment/doc prose
review. `PROSE=1` attests the prose review happened, `TEST=1` also runs
the host-sim suite. See `scripts/README.md` for the details.

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

## The epiccc gate pin

The `epiccc-gate` CI job (`.github/workflows/ci.yml`) builds
`pic16f88x-hal` for the 887 with a pinned epic-cc compiler and runs the
deterministic PORTB toggle gate plus the `mdb-hex` register read. It is
the "did a HAL change break against a known good compiler" direction;
epic-cc's own `hal-887` job asks the reverse question in its tree.

The pin has two halves, both deliberate:

- `EPIC_CC_PIN` (job env): the epic-cc driver's source sha. The job
  checks out `apojomovsky/epic-cc` at that sha and runs
  `cargo build --release --locked -p driver` (a Rust 1.97.1 toolchain
  only, no clang build). The job asserts the checkout sha equals the
  pin and prints that sha, so a failure names the compiler.
- `EPIC_CC_CLANG_TAG` (job env): a tagged epic-cc release whose Linux
  bundle supplies `clang` and `llvm-link`. The job downloads
  `epic-cc-<tag>-x86_64-linux.zip` and verifies it against the
  release's own `SHA256SUMS`.

Why two: the rolling `ci-<sha>` prereleases epic-cc#118 planned never
published (their publish job has no checkout; epic-cc#140 fixed only
`release.yml`), and the one tagged release, v0.0.3, is epic-cc master
whose driver panics on the 887 slice (`isel: call to unknown function
@8`), a regression from the smax/smin isel change (epic-cc#136). The
last known-good driver is the commit before that change; the release
bundle's clang is known good against it. This split is what lets the
job consume a real user-facing artifact (the bundle) while pinning a
working compiler.

Bumping the pin:

1. Pick a new `EPIC_CC_PIN` that still builds the 887 slice. A quick
   check: build the driver at that sha (`cargo build --release -p
   driver` in a checkout) and run
   `make epiccc-build MODULE=pic16f88x-hal MCU=16F887 EPIC_CC_HOST=1`
   with the v0.0.3 bundle's clang exported.
2. Change `EPIC_CC_PIN` in `.github/workflows/ci.yml` to the new sha.
   The pin is a chosen, deliberate bump: a compiler regression shows up
   as a bump that fails the gate, not as a mystery. (When the rolling
   prereleases start publishing, `EPIC_CC_CLANG_TAG` can move to a
   `ci-<sha>` tag and the driver to the same tag's binary; until then
   the split stays.)
3. The job prints the driver sha and clang version in its step summary,
   so a failure names the compiler.

## Releases

Cutting one is a single command:

    scripts/release.sh patch     # 0.3.7 -> 0.3.8
    scripts/release.sh minor     # 0.3.7 -> 0.4.0
    scripts/release.sh major     # 0.3.7 -> 1.0.0
    scripts/release.sh v0.5.0    # or name the version outright

It syncs with the remote, refuses to run on a dirty tree, off `master`,
or when `master` is not level with the remote, computes the next version
from the newest tag, prints the notes that would publish, and asks
before pushing. Everything up to that prompt is local: declining deletes
the tag it made to preview with. `-y` skips the prompt, `--dry-run`
stops before tagging, `--watch` follows the run.

Pushing the tag is the point of no return. Tagging `v*` triggers
`release-bundles.yml`: it builds one source bundle per family, verifies
checksums, gates every bundle from a scratch directory outside any repo
checkout, and only then attaches the tarballs to a GitHub Release. See
[.github/workflows/release-bundles.yml](.github/workflows/release-bundles.yml).

`-y` also skips the warning about a tag with no commits behind it, so a
scripted `-y` run can republish an unchanged tree the way v0.3.3 and
v0.3.4 did.

The release notes are generated, not written. `scripts/release_notes.py`
reads the Conventional Commit subjects between the previous version tag
and this one, groups them (`feat` -> Added, `fix` -> Fixed,
`refactor`/`perf` -> Changed, `docs` -> Documentation), and puts
everything else in a collapsed "Internal changes" block so no commit is
silently dropped. Commit subjects are the only source, so a release can
never disagree with the history it was cut from.

Two things follow from that. A commit subject is release-notes copy:
write it for someone reading the release page. And a change that breaks
consumers has to say so, either `type(scope)!:` in the subject or a
`BREAKING CHANGE:` footer in the body, or it lands under Changed with
nothing to flag it (the Epicurus -> Epic HAL rename did exactly this,
and `EPICURUS_DIR` breaking for every existing consumer went unmarked).

Preview what a tag will publish before pushing it:

    python3 scripts/release_notes.py v0.4.0 \
        --previous v0.3.7 \
        --repo-url https://github.com/apojomovsky/epic-hal

## install.sh

The README's one-command getting started runs `install.sh` (repo root).
To check it locally, build a family bundle with its checksum, the
part-to-family map, and the standalone CLI asset, then point the
installer at the result:

```sh
python3 scripts/make_bundle.py --family PIC16F87XA --version ci-test
python3 scripts/make_bundle.py --cli --version ci-test
cd bundles && sha256sum ./*.tar.gz > SHA256SUMS
python3 -c "import sys; sys.path.insert(0, 'scripts'); import bundlegen, epicmanifest; sys.stdout.write(bundlegen.emit_parts_map(epicmanifest.load(epicmanifest.default_path())))" > parts.txt
cd ..
EPIC_HAL_BASE_URL=file://$PWD/bundles sh install.sh 16F877A ci-test
```

The first argument is a part (`16F877A`; its family is resolved from
`parts.txt`) or a family slug (`pic16f87xa`, `pic18fxx5x`,
`pic16f193x`). `make_bundle.py --family` takes the manifest name
(`PIC16F87XA`, `PIC18Fxx5x`, `PIC16F193X`). `EPIC_HAL_BASE_URL` is a
flat asset dir, so `<version>` is required. The release gate runs the
same flow for all three families and builds the scaffolds, see
[.github/workflows/release-bundles.yml](.github/workflows/release-bundles.yml).

## epic-hal CLI

The `epic-hal` CLI (`scripts/epic_hal.py`, with `epic_hal_init.py`,
`epicmanifest.py`, and `bundlegen.py`) scaffolds a consumer project from
a bundle: `main.c`, a filled `Makefile`, and a patched MPLAB X `.X` for
the chosen part and module subset. Run it from a checkout or an unpacked
bundle:

```sh
python3 scripts/epic_hal.py init --bundle path/to/epic-hal-pic16f87xa-v0.1.0
```

Its tests live with the rest of the scripts tests:

```sh
python3 scripts/tests/test_epic_hal_init.py
```
