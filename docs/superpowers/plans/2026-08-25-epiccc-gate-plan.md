# epic-hal#80: CI gate on the epic-cc build path

## Goal

A CI job in this repository runs `make epiccc-build MODULE=pic16f88x-hal
MCU=16F887` against a pinned epic-cc compiler, then proves the firmware
runs under MPLAB SIM: the deterministic PORTB toggle gate plus the
register-read `mdb-hex` gate, exactly the smoke commands the issue names.

This closes the direction the epic-cc `hal-887` job cannot: "did a HAL
change break against a known good compiler".

## The pin: why not the release bundle's binary

The issue was filed expecting epic-cc#118's rolling `ci-<sha>` prereleases.
They never published (rolling-release.yml's publish job has no checkout
and fails; epic-cc#140 fixed only release.yml), and the one tagged
release, v0.0.3, is epic-cc master `ce7c6af` which **panics** on the
current HAL slice:

    isel: call to unknown function @8   (crates/isel/src/lib.rs:1761)

The regression is the smax/smin isel change (epic-cc#136, `9796c62`),
merged onto master after the last known-good commit.

The last known-good driver is the epic-cc dev image's binary. It was
built from `25a98424ee5e783953bd610b46502468d5a970ac` (the commit
before the regression) and a source build of that sha reproduces it
byte-for-byte (`6ca11ff8…` = `~/.cache/epic-cc/target/release/epic-cc`).

So the job pins **two artifacts deliberately**:

1. `EPIC_CC_PIN` : the driver sha, built from source in the job
   (`cargo build --release -p driver` in a checkout at that sha; a Rust
   1.97.1 toolchain, no clang build).
2. `EPIC_CC_CLANG_TAG` : v0.0.3's Linux bundle, used only for its
   `clang/bin/{clang,llvm-link}` + `clang/lib/clang/20` (verified
   working with the pinned driver; the bundle's own driver binary is
   never used).

A compiler regression shows up as a chosen bump of `EPIC_CC_PIN` that
fails the gate, not as a mystery. The bump procedure lives in
DEVELOPMENT.md.

## Job shape

New `epiccc-gate` job in `.github/workflows/ci.yml`, sibling to the
family jobs (same pull of the private toolchain image for the mdb half):

1. checkout, resolve non-code (same classifier the family jobs use).
2. checkout epic-cc at `EPIC_CC_PIN`.
3. install Rust 1.97.1 (rustup; the toolchain `rust-toolchain.toml`
   pins it), `cargo build --release --locked -p driver`.
4. download `epic-cc-0.0.3-x86_64-linux.zip` + `SHA256SUMS`, verify the
   sha256 (the SUMS file lists `linux-bundle/…` paths; normalize),
   unzip.
5. emit the build script (`epic_build.py build --toolchain epic-cc`)
   and run it via `make epiccc-build EPIC_CC_HOST=1` with the bundle's
   clang env vars exported.
6. copy the hex where the toggle gate looks (`build-sim/…`).
7. print the versions used: driver sha (git rev-parse of the epic-cc
   checkout, asserted equal to the pin), clang version.
8. pull the toolchain image (same resolution as the family jobs).
9. run the deterministic toggle gate (stepi, PORTB bit 0, 12 samples)
   on the hex: the "does it actually blink" assertion.
10. run the `mdb-hex` register-read gate (`print PORTB\nprint TMR0`).

The mdb half needs the private XC8 image, so this is not the public
gate (that is HAL-4/#60); it proves the build and the register read on
the existing runner, as the issue scopes.

## Makefile changes

- `epiccc-build` gains an `EPIC_CC_HOST=1` branch that runs the emitted
  script directly on the host (the CI job has no dev image; the driver
  and clang are prepared by the job itself). The docker branch is
  unchanged for local dev.
- `mdb-hex`'s inline docker+mdb command sequence moves to a committed
  `scripts/mdb-hex-run.sh` (the same shape as `sim-mdb-run.sh`, the
  "one source of truth" the Makefile already uses for the other gates),
  so the CI job runs one script, not an inline duplicate.

## Docs

DEVELOPMENT.md: document the job and the deliberate pin-bump procedure
(the two pins, how to bump, what a regression looks like). AGENTS.md's
CI section: add the epiccc-gate job to the job list.

## Verification (done locally)

- Driver at `25a9842` builds in ~12s in the dev image; the binary is
  byte-identical to the dev image's known-good binary.
- Blink built with pin driver + v0.0.3 bundle clang; toggle gate PASS
  (6 transitions); `mdb-hex` reads PORTB/TMR0.
- The v0.0.3 driver alone panics on the same build (the regression).
