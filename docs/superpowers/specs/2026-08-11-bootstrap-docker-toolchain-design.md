# Bootstrap: Docker-first real-target setup, self-instructive vendor handling

Status: **approved 2026-08-11, not started**.

## Problem

`scripts/bootstrap.sh` ends by checking whether `xc8-cc` is on `PATH`
and, when it is not, prints instructions to install MPLAB X + XC8 v3.x
manually and add the compiler's `bin/` to `PATH`. That message is wrong
for the repo's own current posture: since `docs/docker-dev-plan.md`
landed, real-target XC8 builds and the `mdb`/MPLAB SIM gate run inside
the `docker/ci-toolchain/` image, and nothing MPLAB-ish needs to be
installed on the host at all. The user wants this setup to assume no
local MPLAB install and rely 100% on the Docker toolchain image, with
bootstrap doing the vendor-installer handoff graciously and
self-instructively: what to download, from where, where to place it,
and what to do next.

## Goal

`scripts/bootstrap.sh` becomes the Docker-first setup for real-target
work:

- verifies Docker is present and the daemon is reachable (does not
  install Docker itself, user-approved),
- handles the two vendor-sourced Microchip installers graciously,
  telling the user exactly what to download, from where, and where to
  place it, then builds the toolchain image once the files are there,
- builds the image eagerly during bootstrap when the installers are
  already in place (idempotent: re-runs skip the build via a cached
  image check),
- deletes the manual-install prose and the `xc8-cc` PATH check
  entirely, and
- updates every doc that describes the old bootstrap contract.

## Decisions (user-approved)

1. **Verify-only for Docker itself.** Bootstrap checks `docker` on
   `PATH` and `docker info` (daemon reachability) and prints a hint if
   either fails. It does not apt-install Docker, add group membership,
   or enable a daemon; installing Docker needs a re-login for group
   membership to take effect, which a script cannot finish, and Docker
   is a distro-level choice outside bootstrap's contract.
2. **Delegate to the root Makefile targets.** Bootstrap runs `make
   check-vendor` and `make image`, the existing single sources of truth
   for installer filenames, size thresholds, download URLs, and the
   image build. No filenames, URLs, or thresholds are duplicated into
   `bootstrap.sh` where they could drift.
3. **Eager image build, cached-image skip.** In normal mode, when the
   vendor files pass `check-vendor` and `docker image inspect
   pic8-hal-toolchain:local` shows the image already exists, bootstrap
   prints a ready message and skips the build. Otherwise it runs `make
   image` (layer-cached after the first build).
4. **Native XC8 installs still work, bootstrap just stops checking for
   them.** The `xc8-cc`-on-`PATH` branch and the "install MPLAB X +
   XC8 v3.x manually" instructions are deleted. Anyone with a native
   install keeps using it unchanged.
5. **`--check-only` reports only.** It verifies Docker, vendor files,
   and image presence, exits nonzero on any gap, and never creates
   directories or builds anything. Creating `vendor/` and building the
   image are install actions, out of scope for report-only mode.

## Behavior matrix

| State | Normal run | `--check-only` |
|---|---|---|
| `docker` missing | print hint (install Docker, see `docs/docker-dev-plan.md`), continue, exit 0 | print, `problems=1`, exit 1 |
| docker present, daemon unreachable | print hint (daemon/group), continue | print, `problems=1` |
| `make` missing (non-apt package-manager paths) | print 3-line fallback (`docker build -t pic8-hal-toolchain:local docker/ci-toolchain` by hand), continue | print, `problems=1` |
| docker + make ok, vendor files missing | `mkdir -p docker/ci-toolchain/vendor`, show `make check-vendor` output (per-file what/URL/save-as), print the recap block below, exit 0 | same output, `problems=1`, no `mkdir` |
| docker + make + vendor ok, image already built | "real-target toolchain ready" message, no build | report image present |
| docker + make + vendor ok, image not built | `make image` | report image not built, `problems=1` |

Normal mode still exits 0 when the toolchain is not ready: host-sim
builds and tests do not need it, and the recap block tells the user the
one-time path to readiness.

## Vendor handling block

When `make check-vendor` fails, bootstrap prints, in order:

1. `mkdir -p "$repo_root/docker/ci-toolchain/vendor"` runs first
   (normal mode only), so "place it here" is a real, existing path the
   user can copy into without thinking about it.
2. The `make check-vendor` output itself: per missing file it names the
   file, its target path, the Microchip download URL, and the save
   instruction (including the "tar it up as a single `.tar`" step for
   MPLAB X), plus the bot-challenge explanation of why this step cannot
   be automated. URLs live only here, never in `bootstrap.sh`.
3. A recap block so the whole picture is scannable without re-reading
   the make output:

```
bootstrap: real-target toolchain not built yet. One-time manual step:
  1. In a browser, download the two Microchip installers (links in the
     make output above; the bot-challenge blocks scripted downloads):
       docker/ci-toolchain/vendor/xc8-installer.run
       docker/ci-toolchain/vendor/mplabx-installer.tar
  2. Re-run ./scripts/bootstrap.sh (or run `make image`).
  Host-simulation builds and tests work without this.
```

4. A small enrichment to the `make check-vendor` target itself: state
   the minimum-size expectations in the "missing (or too small)"
   messages ("expected at least ~10 MB" for the XC8 installer,
   "~100 MB" for the MPLAB X tarball), so a truncated browser download
   is recognized as such instead of failing mysteriously later at
   `docker build`. The numbers stay in the Makefile (where the
   thresholds already are), bootstrap just shows the output.

## Implementation notes

- New "Docker toolchain" section in `scripts/bootstrap.sh`, placed
  after the pre-commit hook section and before the final exit logic,
  replacing the XC8 section (current lines 73-80) and the header's
  manual-install paragraph (current lines 12-16).
- `Makefile` `check-vendor`: the two "missing (or too small)" messages
  gain the minimum-size expectation ("expected at least ~10 MB" /
  "~100 MB"). Nothing else about the target changes.
- The image tag literal `pic8-hal-toolchain:local` is duplicated from
  the Makefile's `LOCAL_IMAGE` variable, with a comment noting the
  link; a stable literal, not worth a make round-trip to derive.
- `command -v make` guard before the delegate calls; the apt path
  installs `build-essential` (which provides make) earlier in the
  script, the guard covers the dnf/pacman paths.
- `docker info` fails fast when the daemon is unreachable (connection
  refused), no timeout wrapper needed.
- The docker section must not `set -e`-trap failures as fatal: a
  missing toolchain is a printed, non-fatal outcome in normal mode,
  consistent with how the script treats a missing XC8 today.

## Docs to update

- `scripts/bootstrap.sh` header (lines 12-16) and XC8 section (lines
  73-80): replaced by the Docker-first description.
- `scripts/README.md` Bootstrap section (lines ~21-29): the "checks
  whether `xc8-cc` is on `PATH`" and "install MPLAB X + MPLAB XC8 v3.x
  manually" prose becomes the Docker-first description.
- `DEVELOPMENT.md` Native toolchain section: the bootstrap paragraph
  changes; add a "Docker is the default" pointer. The native XC8
  instructions stay as the documented exception path.
- `docs/docker-dev-plan.md`: the "What does NOT change" bullet that
  lists `scripts/bootstrap.sh` as unchanged is updated, and the
  `Status:` line notes the bootstrap integration.
- `AGENTS.md` line 40 ("`./scripts/bootstrap.sh` covers the host-sim
  side only"): updated to mention the Docker toolchain section.
- `docs/ci-plan.md` line ~480: the host-job scope wording that says
  "matching what `scripts/bootstrap.sh` already sets up" is narrowed to
  the host-tooling subset.

## Verification

- `./scripts/bootstrap.sh` on this machine (Docker present, vendor
  files absent): host packages and the pre-commit hook still work, the
  `vendor/` directory is created, `check-vendor` guidance and the recap
  block are printed, exit code 0.
- `./scripts/bootstrap.sh --check-only`: exit code 1, Docker OK and
  vendor-missing reported, no directory created, no build run.
- Re-run idempotency after the user drops the two installer files:
  `make image` runs once, later re-runs report the image as ready and
  skip the build.
- Re-run idempotency with the image already built: ready message, no
  `docker build`.
- `bash -n scripts/bootstrap.sh` and the pre-commit checks
  (`PRE_COMMIT_BASE_REF=master scripts/pre-commit-checks.sh`) pass.
- Full end-to-end (`make xc8-build` + `make mdb-test` PASS) needs the
  two human-fetched installer files and is exercised after the user
  places them in `vendor/`; it is not reachable in this session.

## Out of scope

- Installing Docker itself (verify-only, decision 1).
- Automating the installer downloads: Microchip's download CDN sits
  behind an Akamai bot-challenge that blocks every scripted client
  (confirmed in `docs/ci-plan.md` and `docs/docker-dev-plan.md`), so
  the two files stay a one-time human step; bootstrap's job is to
  instruct, not to fetch.
- Changing any other Makefile target's behavior; `check-vendor` only
  gains the size-expectation wording.
- Removing native-XC8 support from the repo; native installs keep
  working, bootstrap simply no longer checks for them.
