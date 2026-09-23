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

scripts/xc8-size-baseline.sh epic-encoder 16F877A  # flash/RAM size baseline
                                                    # for epic-cc#200's table

make shell           # interactive shell, repo mounted at /repo
```

The image tag is resolved from `docker/ci-toolchain/Dockerfile`'s ARG
lines as `xc8-v${XC8_VERSION}-dfp${PIC16}-${PIC18}-${PIC1216F1}-mplabx${MPLABX_VERSION}`;
`make ci-image-push` and the CI jobs reuse the same grep so the formula
cannot drift.

The tag formula above is the single source of truth for the image
version; the targets in this section are the full command reference.

## The two device-pack trees

The image carries every device pack twice, and the two copies are not
the same version. The Dockerfile downloads the three DFPs at its ARG
versions into the XC8 tree
(`/opt/microchip/xc8/v${XC8_VERSION}/pic/packs/Microchip.PIC16Fxxx_DFP`);
the manifest's per-family `dfp_version`, the bundle QUICKSTART
download URL, and the `make TOOLCHAIN=xc8` consumer path all use that
copy. MPLAB X (installed for `mdb.sh`) bundles its own packs under
`/opt/microchip/mplabx/v${MPLABX_VERSION}/packs/` at whatever version
ships with that MPLAB X release (PIC16Fxxx_DFP 1.8.167 on 6.35), and
the reference projects' nbproject pins
(`examples/epic-hal-demo-*.X`, copied verbatim into every bundle as
`examples/epic-hal-demo.X`) must reference the MPLAB X tree, because
the CI isolated bundle gate builds them there. The two versions
coincide for PIC12-16F1xxx and PIC18Fxxxx today and diverge for
PIC16Fxxx; aligning one side to the other breaks either the bundle
gate or the consumer install, so treat the split as deliberate.

A reference project pins the pack in three places, and all three must
agree: `default.Pack.dfplocation` in
`nbproject/Makefile-genesis.properties`, `DFP_DIR` in
`nbproject/Makefile-local-default.mk`, and the `<pack>` element in
`nbproject/configurations.xml`. `scripts/dfp-pin-audit.py` (in
`make audit` and each family's CI job) checks that, and checks the
manifest's `dfp_version` against the Dockerfile ARG, which is the pair
that decides what a bundle tells a consumer to download.

### What MPLAB X regenerates on open (epic-hal#230)

Opening a reference project in MPLAB X can rewrite the makefiles, so the
pins above are not purely ours once a developer opens one of these trees.
Per Microchip's MPLAB X IDE User's Guide (DS-50002027E, §12.16.2 "Project
Options Tab", §5.23.1 "Saving Project Files" with Table 5-10, and §8.2
"Files Window View", checked 2026-09-21):

- **Regeneration is conditional, not automatic.** "Force makefile
  regeneration when opening a project" is unchecked by default, which
  tells the IDE to "determine whether or not a makefile should be
  regenerated when a project has opened (e.g., opening a project on a
  different computer)". Microchip's stated example is a different
  computer, not a pack mismatch, so a different pack set is our
  inference from "the IDE decides": expect a rewrite and do not treat a
  regenerated `DFP_DIR` as corruption.
- **`configurations.xml` is the authoritative surface.** Table 5-10
  marks it with a green check, "required to generate the project image",
  alongside `project.xml` and `project.properties`. It is the metadata a
  regeneration reads; reconcile that file, not its outputs.
- **`Makefile-*` and `private/` are regenerated.** The same table marks
  both with a red X, "regenerated and therefore do not need to be
  saved", and §8.2 calls `nbproject/private/` "the user and computer
  specific settings of the project". A regenerated `DFP_DIR` in
  `Makefile-local-default.mk` is therefore an output difference, not
  drift to chase. Some demos do commit `nbproject/private/configurations.xml`
  (it holds the toolchain dir and run profile, not a pack pin) and it can
  legitimately differ per machine.
- **The project root `Makefile` is never regenerated**: "generated at
  project creation time and it is not touched after that".

The make side agrees: of the three pins, only `DFP_DIR` is consumed, by
`-mdfp="${DFP_DIR}/xc8"` in `Makefile-default.mk`. Nothing in the make
chain reads `dfplocation` or `private/configurations.xml`, so a
regenerated project still builds whatever the IDE left behind.

This is documented behaviour, not an observed diff: the toolchain image
cannot run the IDE at all (`docker/ci-toolchain/Dockerfile` deletes the
`mplab_ide` cluster to keep the image small, and MPLAB X's headless
makefile generator resolves its classpath to an empty module dir, so it
dies on a missing class), so the experiment in epic-hal#230's recipe
still needs a human with a licensed IDE. What CI can hold to is the pin
set itself, which is what `dfp-pin-audit.py` checks on every family.

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
review. The prose step lints mechanically and prints every added block;
`TEST=1` also runs the host-sim suite. See `scripts/README.md` for the
details.

## CI

CI runs two jobs on every push (`.github/workflows/ci.yml`):

- **host**: every module's CMake/ctest on a bare runner, plus the
  pre-commit checks.
- **target**: one Docker pull, then a real XC8 build for every family
  the job covers, the mdb/MPLAB SIM runs, and the isolated bundle-gate
  build.

Maintainers with `write:packages` access to this repo's GHCR packages
can publish an updated toolchain image with `make ci-image-push
GHCR_OWNER=<owner>` (after `docker login ghcr.io`), the same private tag
CI's workflows pull from; CI itself never builds this image. See
[.github/workflows/ci.yml](.github/workflows/ci.yml) for the job
definitions and the consolidation tradeoff in its header comment.

## The epiccc gate pin

The `epiccc-gate` CI job (`.github/workflows/ci.yml`) builds
`pic16f87xa-hal` for the 877A, `pic16f88x-hal` for the 887,
`epic-settings` and `epic-sdcard` for the 4550 (build-only),
`epic-pic16f193x-firmware` for the 16F1937,
`pic16f63x_67x_68x-hal` for the 16F677, `pic16f7x-hal` for the 16F77,
the #150 exemplars `pic18f2520-hal` for the 18F2520, `pic18f1320-hal`
for the 18F1320 and `pic18f6520-hal` for the 18F6520 (build-only),
`pic16f5x-hal` for the 16F54, `pic16f818_819-hal` for the 16F819,
`pic16f628a-hal` for the 16F628A, `pic16f83_84-hal` for the 16F84A
(build-only, a baseline die like the 16F54) and `pic18fxx5x-hal` for
the 18F4550
with a pinned
epic-cc compiler, and runs the deterministic
PORTB toggle gate on the classic-PIC16 blink hexes (the 677 leg watches
PORTB bit 4, the only implemented low bit on this family's PORTB), the LATB toggle gate on
the 1937 (its GPIO driver toggles the latch, DS41364E), plus the `mdb-hex`
register read. The 16F54 leg runs its gate with no `--irq-every`: the
die has no interrupt and its epic-cc example toggles from a software
loop. The set of families with a declared `epiccc_sources` slice is held
equal to this job's coverage by `scripts/epiccc-slice-audit.py` (in
`make audit` and every family-check audit step), so a new slice without
a leg fails CI (epic-hal#257). It is
the "did a HAL change break against a known good compiler" direction;
epic-cc's own `hal-887` job asks the reverse question in its tree.

The pin has two halves, both deliberate:

- `EPIC_CC_PIN` (job env): the epic-cc driver's source sha. The job
  checks out `apojomovsky/epic-cc` at that sha and runs
  `cargo build --release --locked -p driver` (a Rust 1.97.1 toolchain
  only, no clang build). The job asserts the checkout sha equals the
  pin and prints that sha, so a failure names the compiler.
- `EPIC_CC_CLANG_TAG` (job env): a tagged epic-cc release whose Linux
  bundle supplies `clang`, `llvm-link` and `opt`. The job downloads
  `epic-cc-<ver>-x86_64-linux.zip` and verifies it against the
  release's own `SHA256SUMS`.

Why two: rolling `ci-<sha>` prereleases (ADR-023) are being retired, and the
v0.0.3 tag release was cut from an epic-cc master whose driver panicked
on the 887 slice (`isel: call to unknown function @8`), a regression
from the smax/smin isel change (epic-cc#136) that master has since
fixed (epic-cc#142, #153). The stable v0.3.0 release supplies the clang
bundle (clang 20.1.8, cut from a driver that satisfies the `opt`
requirement of epic-cc#198), so `EPIC_CC_CLANG_TAG` pins that stable
release and the driver is pinned separately from source.

Bumping the pin:

The pin holds epic-cc master at ef2632b (the epic-hal#215 bump), which
carries the p16f819 registry entry (epic-cc#439, merged by PR #440)
this job's
16F819 leg needs, on top of the p16f54 flat 25-byte GPR fix
(epic-cc#437 / PR #438, the epic-hal#213 bump) the 16F54 leg needs, the
p16f677 target (epic-cc#421) on
top of the #129 PIC14E port (the isel-pic14e backend, the
`Pic14e` sim core, `parse_hex_pic14e`, i1 loads/stores as bytes and the
PIC14E config-field defaults) on top of everything the 877A/887 slices
need plus the PIC18 fixes for the 4550 slice (epic-cc#180, #189, #194).
The previous pin (883ae52, the #213 bump) introduced the p16f54 fix;
the one before it (e4a4e78, the #146 bump) predates it: that driver
caps the part's GPR, so the 16F54 slice links against a wrong RAM
model. The pin comment in ci.yml's `EPIC_CC_PIN`
records the reasoning for the current sha.

1. Pick a new `EPIC_CC_PIN` that still builds the 887 slice. A quick
   check: build the driver at that sha (`cargo build --release -p
   driver` in a checkout) and run `make epiccc-build
   MODULE=pic16f88x-hal MCU=16F887 EPIC_CC_HOST=1
   EPIC_CC_BIN=<checkout>/target/release/epic-cc` with the stable
   release bundle's clang exported. `EPIC_CC_BIN` is what selects the
   driver; its default names the container-side path, so without the
   override the check runs whatever the image would have. A host build
   like this stays in the checkout and must not be copied into the
   shared `~/.cache/epic-cc/target` cache, which the container has to
   load (see "The shared driver binary and the image's glibc").
2. Change `EPIC_CC_PIN` in `.github/workflows/ci.yml` to the new sha.
   The pin is a chosen, deliberate bump: a compiler regression shows up
   as a bump that fails the gate, not as a mystery. The clang bundle
   stays on the stable release (`EPIC_CC_CLANG_TAG`/`EPIC_CC_CLANG_VER`)
   unless a driver change forces a front-end or `opt` change that
   requires a re-cut.
3. The job prints the driver sha and clang version in its step summary,
   so a failure names the compiler.

## The sim gate (HAL-4)

The epiccc-gate CI job verifies its hexes in epic-cc's own ISA
simulator (`crates/sim`, via `scripts/sim-runner/`), so the epic-cc
path needs neither mdb nor the private toolchain image; the XC8 + mdb
jobs stay as the differential oracle. The runner is a thin wrapper:
it loads a hex, raises the firmware's interrupt source the way the
peripheral would (flag through its enable bit, vector when GIE
allows), and requires the watched register bit to toggle across
step-counted samples, the same contract the mdb toggle gate runs.

crates/sim models the CPU, not the peripherals or the oscillator, so a
gate's timer interrupt is injected on a step schedule rather than
counted from a clock. The tick counter in the firmware counts
interrupts, not cycles, which is what makes that honest: the gate
proves the interrupt path, not a timing figure. It also needs every
register the gate touches to be reachable in the simulator; INTCON is
mirrored on the PIC14 core, which is why the blink gates run today
while the tick gates wait on epic-cc#173 (banked SFR writes do not
land in the simulator yet).

Local run of the same gate the CI job runs:

```sh
ln -s ~/projects/epic-cc epic-cc        # once per worktree
make epiccc-build MODULE=pic16f87xa-hal MCU=16F877A
make sim-epiccc HEX=build/epiccc/16F877A-blink.hex DEVICE=PIC16F877A \
  SAMPLES=12 STEPS=200000 \
  IRQ_EVERY=65536 IRQ_FLAG=INTCON:2 IRQ_ENABLE=INTCON:5
```

`sim-epiccc` runs the runner inside the epic-cc dev image, the same
toolchain the compiler itself was built with; `--trace
INTCON,PIR1,PIE1` on the runner prints register values per sample when
a gate needs debugging. The host gcc/ctest build stays the fast inner
loop; the sim gate is the CI verification layer between that and the
mdb oracle.

### The shared driver binary and the image's glibc

`epiccc-build` resolves its driver as a path inside the container
(`EPIC_CC_BIN`, default `/tmp/cargo-target/release/epic-cc`). That path
is not part of the image: the Makefile bind-mounts the host's
`~/.cache/epic-cc/target` onto it, so the release binary last written
there is the one this target runs. (`sim-epiccc` and the mdb gates take
an already built hex and never invoke the driver.)

The container is Ubuntu 22.04 (glibc 2.35), so the binary at that path
must be one the image can load. Building the driver with a host cargo
on a newer glibc drops a binary requiring newer symbols into the shared
cache, and every `epiccc-build` then dies with:

    /tmp/cargo-target/release/epic-cc: /lib/x86_64-linux-gnu/libc.so.6:
    version `GLIBC_2.39' not found

Read that message as a build-location problem, not a corrupt cache: it
names the symbol version, so the driver was built against a newer glibc
than the image carries. Build it inside the dev image instead, with the
same mounts the Makefile uses, and it lands at the shared path with the
image's own glibc:

```sh
ln -s ~/projects/epic-cc epic-cc        # if not linked yet, see above
cd epic-cc
make exec TARGET_CACHE=$HOME/.cache/epic-cc/target \
  CMD='cargo build --release -p driver'
```

`TARGET_CACHE` is what points the build at the shared path; without it
`make exec` uses epic-cc's own per-worktree cache and the fix does not
land where `epiccc-build` looks. `EPIC_CC_HOST=1` is the exception
rather than the trap: it runs the emitted script on the host, so a
host-built driver is what that mode wants. CI uses it with its own
target directory, never this cache.

### The staleness guard

Because that binary is a cache nobody invalidates, it can silently lag
the checkout: a driver one commit behind reruns yesterday's compiler
bugs under today's failure signatures, which reads like a fresh
miscompile and costs a root-cause session (epic-hal#240).

`epiccc-build` therefore refuses to run when all of these hold: the
driver is still the shared default (`EPIC_CC_BIN` unset), the epic-cc
checkout is discoverable at `<repo>/epic-cc`, and the cached binary is
not the checkout's binary. The failure names both shas and the rebuild
command.

The check is identity, not timestamps: the driver stamps its commit
into `epic-cc --version` (epic-cc#525), and the guard compares that
stamp against `rev-parse --short HEAD` of the checkout. A reset behind
the binary or a rebase that rewrites commit times still changes HEAD,
so both are detected where the old mtime comparison went the wrong
way. The comparison is against full HEAD, not just compiler sources,
so even a docs-only epic-cc commit invalidates the cached driver:
rebuild or set `EPIC_CC_ALLOW_STALE=1`.

A full-sha stamp matches its short prefix.

Two overrides, for when the check is not what you want:

- `EPIC_CC_BIN=<path>`: any driver of your own, never second-guessed.
  CI uses this, together with its own target directory.
- `EPIC_CC_ALLOW_STALE=1`: build with the cached binary anyway.

A driver that predates sha stamping reports a bare version with no
commit, and falls back to the old mtime proxy: refuse when the newest
commit touching `crates/`/`Cargo.toml`/`Cargo.lock` is newer than the
binary. Rebuild that driver once to get the exact check.

## Releases

**Automated (preferred)**: run the `cut-release` workflow
(`workflow_dispatch`, optional `bump: auto|patch|minor|major`, default
`auto`, which reads the bump size off the Conventional Commits since the
last tag). It regenerates `CHANGELOG.md` with `git-cliff` (`cliff.toml`),
commits it to `master` as `chore(release): vX.Y.Z`, and pushes the
annotated tag, no local checkout required. See
[.github/workflows/cut-release.yml](.github/workflows/cut-release.yml).

**Manual**: `scripts/release.sh` still works for a local, previewed cut:

    scripts/release.sh patch     # 0.3.7 -> 0.3.8
    scripts/release.sh minor     # 0.3.7 -> 0.4.0
    scripts/release.sh major     # 0.3.7 -> 1.0.0
    scripts/release.sh v0.5.0    # or name the version outright

It syncs with the remote, refuses to run on a dirty tree, off `master`,
or when `master` is not level with the remote, computes the next version
from the newest tag, prints a `scripts/release_notes.py` preview of what
would publish, and asks before pushing. Everything up to that prompt is
local: declining deletes the tag it made to preview with. `-y` skips the
prompt, `--dry-run` stops before tagging, `--watch` follows the run.
Unlike `cut-release.yml`, it never touches `CHANGELOG.md`: a tag cut this
way leaves the changelog stale until the next automated cut catches up,
so prefer the workflow unless you specifically need the local preview or
dry-run.

Pushing the tag (either way) is the point of no return. Tagging `v*`
triggers `release-bundles.yml`: it builds one source bundle per family,
verifies checksums, gates every bundle from a scratch directory outside
any repo checkout, and only then attaches the tarballs to a GitHub
Release, with notes generated from the tag's `git-cliff` section. See
[.github/workflows/release-bundles.yml](.github/workflows/release-bundles.yml).

`scripts/release.sh -y` also skips the warning about a tag with no
commits behind it, so a scripted `-y` run can republish an unchanged tree
the way v0.3.3 and v0.3.4 did.

The published release notes are generated, not written: `cliff.toml`
groups the Conventional Commit subjects between the previous tag and
this one (`feat` -> Features, `fix` -> Bug Fixes, and so on; see the file
for the full grouping), so a release can never disagree with the history
it was cut from, and no commit is silently dropped, either it lands
under a real heading or under the catch-all "Other" group.

That means a commit subject is release-notes copy: write it for someone
reading the release page. And a change that breaks consumers has to say
so, either `type(scope)!:` in the subject or a `BREAKING CHANGE:` footer
in the body, or nothing flags it (the Epicurus -> Epic HAL rename did
exactly this, and `EPICURUS_DIR` breaking for every existing consumer
went unmarked).

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
`parts.txt`) or a family slug (`pic16f87xa`; `install.sh --list` prints
them all). `make_bundle.py --family` takes the manifest name
(`PIC16F87XA`). `EPIC_HAL_BASE_URL` is a flat asset dir, so `<version>`
is required. The release gate runs this flow end to end and builds the
scaffolds, see
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

`epic-hal --version` reports the release stamp (the `VERSION` file
shipped in the release CLI asset and in every family bundle), mirroring
how `epic-cc --version` prints the compiler's build stamp. In a source
checkout, where no `VERSION` file exists, it prints `epic-hal dev
(source checkout)` instead of fabricating a release number.

Its tests live with the rest of the scripts tests:

```sh
python3 scripts/tests/test_epic_hal_init.py
```
