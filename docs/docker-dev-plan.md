# Docker-first local dev, and pushing CI's toolchain image from the CLI

Status: **implemented and fully verified against real builds**,
including the full image (XC8 + all three DFPs + MPLAB X) and a real
`mdb` gate run. The user supplied both installers (see "What the user
must provide" below; the Akamai bot-challenge is a hard, confirmed wall,
not a gap in effort). Root `Makefile` covers host tests, real-target XC8
builds, the `mdb` gate, and a dev shell, all through the existing
`docker/ci-toolchain/` image built from locally-supplied vendor
installers. CI now pulls a pre-pushed private image instead of building
it; the `ci-assets`
blob-carrier mechanism is dormant, not deleted. Two real bugs (root-owned
build output, a missing `cmake`/`build-essential` in the image) were
found and fixed by actually running the flow, see "Verification
performed this session" below.

## Motivation

Contributing to this repo's real-target/`mdb` work required installing
XC8, MPLAB X, and their DFPs by hand: proprietary, license-gated,
interactive installers, explicitly out of scope for `scripts/
bootstrap.sh` (host-sim only). `docker/ci-toolchain/Dockerfile` already
builds exactly the environment needed (XC8 v4.00 + MPLAB X/`mdb` + DFPs)
from installer files placed under `docker/ci-toolchain/vendor/`, it was
just wired for CI's asset-fetching flow (pull installers out of a
private GHCR "blob carrier" image), not for a human dropping files in
directly. This plan makes that same image the single local-dev
mechanism too, and, since a locally-built image is a normal Docker
image, also lets it be pushed straight to the private GHCR package CI
already pulls from, closing the loop so CI doesn't need to build the
image at all.

## Decisions (user-approved)

1. **Reuse `docker/ci-toolchain/Dockerfile`, don't fork a second one.**
   One toolchain definition (XC8 version, DFP set, MPLAB X version) for
   both CI and local dev; a fork would drift.
2. **Single image, no lighter host-sim-only tier.** Every root-Makefile
   target, including plain host-sim tests, runs inside the full
   toolchain image. Simpler mental model, at the cost of requiring the
   Microchip installers even for `make test`.
3. **Root Makefile always builds locally from `vendor/`, never pulls the
   private CI image.** One code path, no GHCR auth needed for local
   dev, works for any contributor regardless of GHCR package access.
4. **`make check-vendor`** verifies the expected installer files are
   present before any build, printing what's missing and where to get it
   (Microchip's own download pages; we still can't fetch them
   ourselves, same bot-challenge as documented in `docs/ci-plan.md`).
5. **`make test` defaults to every module, `MODULE=` scopes to one.**
   Matches what CI's own matrix already does.
6. **XC8/`mdb` targets included now**, not deferred: `make xc8-build`,
   `make mdb-test`, on top of `make test` and `make shell`.
7. **Add the `PIC12-16F1xxx_DFP` pin now** (version 1.9.258, the version
   already verified working for `pic16f193x-hal`), so the new family's
   real-target/`mdb` work is reachable through this flow from day one.
8. **CLI push, as its own explicit target (`make ci-image-push`).** Since
   the locally-built image is a normal image, pushing it to the same
   tag CI already resolves and pulls closes the loop: CI's `xc8-build.yml`
   / `sim-tests.yml` are simplified to `docker pull` only, no build
   fallback, no `ci-assets`/`ci-assets-mplabx` jobs. `docker/ci-assets/`
   stays in the repo, dormant, as a fallback mechanism if CI ever needs
   to rebuild the image itself again, not deleted.

## What does NOT change

- The EULA/redistribution constraint from `docs/ci-plan.md`: the GHCR
  image stays **private**. `make ci-image-push` pushes to the same
  private tag, it does not change visibility. A human with a
  `write:packages`-scoped PAT runs it deliberately; this Makefile never
  pushes as a side effect of any other target.
- `scripts/bootstrap.sh`, `scripts/sim-test-local.sh`,
  `scripts/sim-mdb-run.sh` are unchanged; the root Makefile's `mdb-test`
  target calls the same `scripts/sim-mdb-run.sh`, so CI, the old local
  script, and the new Makefile path all share one source of truth for
  the actual `mdb` command sequence.
- No top-level *build*: `AGENTS.md`'s "no top-level build, build each
  module directly" still holds inside the container; the Makefile loops
  over modules by shelling into per-module `cmake`/`make` invocations,
  it does not introduce a unified CMake super-build.

## Root Makefile targets

- `make check-vendor`: verifies `docker/ci-toolchain/vendor/xc8-installer.run`
  and `.../mplabx-installer.tar` exist and are plausibly sized (same
  sanity thresholds the Dockerfile's own `RUN` steps already assert),
  prints what's missing and a pointer to Microchip's download pages if
  not. Every other target that needs the image depends on this first.
- `make image`: `docker build -t pic8-hal-toolchain:local docker/ci-toolchain`,
  depends on `check-vendor`.
- `make ci-image-push`: resolves the exact tag `xc8-build.yml`/
  `sim-tests.yml` compute (same `ARG`-reading formula, kept in one
  place, see Implementation notes), tags the locally-built image with
  it, `docker login ghcr.io` (expects the operator to already be logged
  in or provide credentials via the normal `docker login` flow, this
  target does not embed or request a token itself), `docker push`.
  Never run automatically by any other target.
- `make test [MODULE=<name>]`: host-sim build + run for every module
  with a top-level `CMakeLists.txt` (or just `MODULE`), inside the
  container, bind-mounting the repo. Reuses the same module-discovery
  approach `host-tests.yml` uses (`git ls-files -- '*/CMakeLists.txt'`).
- `make xc8-build MODULE=<name> MCU=<mcu>`: real-target build for one
  module/MCU pair inside the container; output lands in that module's
  `mcu/*/build/` on the host (bind-mounted).
- `make mdb-test MODULE=<name> MCU=<mcu> DEVICE=<device> DFP=<pack> [WAIT_MS=<ms>]`:
  runs `scripts/sim-mdb-run.sh` inside the container with those
  arguments.
- `make shell`: interactive shell in the container, repo bind-mounted at
  `/repo`, `$XC8_INSTALL_DIR`/`PATH` already set by the image.

## CI simplification

`xc8-build.yml` and `sim-tests.yml` each lose their `ci-assets`,
`ci-assets-mplabx`, and the build-on-cache-miss half of `toolchain-image`;
that job becomes a straight `docker pull` of the version-pinned tag,
failing loudly (with `make ci-image-push` instructions) if the tag isn't
there, the same "fail loudly, don't silently refetch from somewhere"
posture `ci-assets` already established for exactly this reason. Every
downstream job (`build`, `sim-test`) is unaffected, they only ever
consumed `toolchain-image`'s resolved `image` output.

## Implementation notes

- The tag-resolution formula (`xc8-v${XC8_VERSION}-dfp${PIC16}-${PIC18}-${PIC1216F1}-mplabx${MPLABX_VERSION}`,
  now three DFP versions instead of two once the 193X pin lands) is
  read out of `docker/ci-toolchain/Dockerfile`'s `ARG` lines by grep, the
  same technique `scripts/sim-test-local.sh` and every CI job already
  use; the root Makefile's `ci-image-push` and `xc8-build`/`mdb-test`
  targets reuse that exact grep so there is one formula, not a
  duplicated one that can drift.
- `docker/ci-toolchain/vendor/` stays gitignored (`*.run`, `*.tar`
  already ignored per the existing Dockerfile's own comments); nothing
  about this plan changes that.

## Sign-off

- [x] `docker/ci-toolchain/Dockerfile` pins `PIC12-16F1xxx_DFP`.
- [x] Root `Makefile` implements all six targets above.
- [x] `xc8-build.yml`/`sim-tests.yml` simplified to pull-only.
- [x] Root `README.md` documents the Docker quick start.
- [x] `docs/ci-plan.md` and `scripts/README.md` reference the new flow.

## What the user must provide (cannot be automated)

Microchip's download pages sit behind an Akamai bot-challenge (returns
403/a JS challenge page to any non-browser client, confirmed again
directly in this session: a plain `curl -I` against
`microchip.com/mplab/mplab-x-ide` came back `403 AkamaiGHost`). No agent
working on this repo can fetch these; do not spend time retrying a
scripted download, it is a dead end already documented in
`docs/ci-plan.md` (curl-impersonate/TLS-fingerprint spoofing was tried
there too and also failed). The two files below are the **only** inputs
this whole flow needs a human for:

- `docker/ci-toolchain/vendor/xc8-installer.run`: the XC8 v4.00 Linux
  installer, from https://www.microchip.com/mplab/compilers (a browser
  download; log in with a (free) myMicrochip account if prompted).
- `docker/ci-toolchain/vendor/mplabx-installer.tar`: the MPLAB X IDE
  v6.35 Linux installer, from https://www.microchip.com/mplab/mplab-x-ide,
  tar'd up as a single `.tar` (matching the Dockerfile's own extraction
  step, `tar -xf mplabx.tar` expecting the installer `.sh` inside).

`make check-vendor` checks for exactly these two files/sizes and prints
this same guidance if either is missing; that is the intended, sole
recovery path, not a bug to route around. Everything else (DFPs, apt
packages, the image build itself, every Makefile target) needs no human
input beyond those two files existing once.

## Verification performed this session

**Fully verified end-to-end, real builds, no simulated results, both
installers eventually supplied by the user:**

- The complete image (XC8 v4.00 + all three DFPs + MPLAB X IDE v6.35,
  ~10.8GB) builds successfully via `make image` from the real
  `docker/ci-toolchain/Dockerfile`, ~2 minutes wall clock.
- `xc8-cc` runs inside the built image and compiles against all three
  DFPs, including the new `PIC12-16F1xxx_DFP` for `pic16f193x-hal`.
- `mdb.sh` runs inside the built image.
- A full real-target build of `pic16f193x-hal` (`MCU=16F1937`),
  `pic8-tick`'s PIC16F87XA leg (`MCU=16F877A`), and its PIC18 leg
  (`MCU=18F4550`) all complete and produce valid `.hex` files via the
  real `make xc8-build`.
- All 19 host-sim modules (`git ls-files -- '*/CMakeLists.txt'`) build
  and pass their `ctest` suite via the real `make test`.
- **The `mdb` gate itself runs and reports real
  `PIC8_HARNESS_RESULT: PASS`** for `pic8-tick`'s pilot module, both
  families (`make mdb-test`, `PIC16F877A` and `PIC18F4550`), the actual
  reason this whole Docker effort exists.
- `make ci-image-push`'s tag resolution and its missing-`GHCR_OWNER`
  guard were exercised (`make -n` plus a real run without `GHCR_OWNER`
  correctly erroring); the real `docker push` to the shared private GHCR
  package was deliberately not run in this session (publishing to a
  shared registry needs the user present, not something to do
  silently), but the tag it resolves was confirmed to match exactly what
  `xc8-build.yml`/`sim-tests.yml` compute.
- **Three real bugs found and fixed by this testing**, all below.

### Bug 1: containers ran as root, corrupting host file ownership

Every `docker run` in the original Makefile draft ran as the container's
default user (root). Any file a target wrote to the bind-mounted repo
(`build/` dirs, `.hex` outputs) came back **root-owned on the host**,
un-removable by the invoking user without `sudo` or another container.
Hit directly while testing: cleaning up a test build required a
throwaway `docker run ... rm -rf` because the host user couldn't. Fixed
by adding `--user $(id -u):$(id -g)` to both `DOCKER_RUN` and `shell`'s
`docker run`; confirmed `xc8-cc` and `cmake`/`ctest` all run fine under
an arbitrary non-root UID (the toolchain install is world-readable), and
that output written under the fix lands owned by the invoking host user.

### Bug 2: the toolchain image had no `cmake`/`build-essential`

`docker/ci-toolchain/Dockerfile` only ever installed the packages XC8/
MPLAB X's own installers need (`curl unzip make tar` + GTK libs); nothing
in it provided a C compiler or CMake. `make test`'s host-sim builds need
both. Confirmed the failure directly (`cmake: command not found` inside
the built image) before fixing it by adding `cmake build-essential` to
the same `apt-get install` line, with a header comment explaining these
are for local dev's `make test` specifically, not for CI (CI's own
`host-tests.yml` installs them itself on a bare runner and never touches
this image).

### Bug 3: `--user` broke `mdb.sh`, corrupting the repo with a literal `?` directory

Once the full image (with real MPLAB X) was available, `make mdb-test`
under Bug 1's `--user` fix alone failed and left a literal `?` directory
at the repo root, inside the bind mount. Root cause: `mdb.sh` is MPLAB
X's JVM-based debugger; Java resolves its preferences directory via
`getpwuid()`-based home-directory lookup, not the `$HOME` environment
variable. An arbitrary `--user <uid>:<gid>` with no matching
`/etc/passwd` entry makes that lookup fail, and whatever fallback
`mdb.sh`'s preference-directory creation takes in that case writes into
a directory literally named `?`, at the container's current working
directory, which is the bind-mounted repo. Setting `HOME` alone did not
fix it (confirmed: the JVM home-directory lookup ignores it). Fixed by
bind-mounting the real `/etc/passwd` + `/etc/group` (so the mapped UID
resolves to a real user with the host's real `$HOME` path) and mounting
a writable directory at that exact path, sourced from `~/.cache` on the
host (not the repo, and not a Docker volume: anonymous/named volumes
default to root-owned and hit the identical permission problem this is
fixing). Confirmed clean against a real `mdb-test` run: no stray `?`,
correct ownership on every artifact, real `PASS` result.
