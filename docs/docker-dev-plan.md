# Docker-first local dev, and pushing CI's toolchain image from the CLI

Status: **implemented**. Root `Makefile` covers host tests, real-target
XC8 builds, the `mdb` gate, and a dev shell, all through the existing
`docker/ci-toolchain/` image built from locally-supplied vendor
installers. CI now pulls a pre-pushed private image instead of building
it; the `ci-assets` blob-carrier mechanism is dormant, not deleted.

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

## Known limitation, not fixed here

`make ci-image-push`/CI's pull step were validated by static inspection
(Makefile dry-runs, YAML parsing, dfp/family-glob logic tested against
every real MCU value in the repo) and by the existing pattern they copy
(the same tag formula and pull-vs-fail-loudly posture `ci-assets` already
proved out in production). They were **not** exercised end-to-end
against a real multi-gigabyte XC8/MPLAB X installer pair in this session
(no such files were available locally), so `make image`'s actual `docker
build` succeeding, and a real `make ci-image-push` round trip landing a
pullable tag, are unverified beyond the Dockerfile change itself being a
small, mechanical extension of an already-working build. Whoever first
runs `make image` with real installers in `vendor/` is the actual
verification of that half.
