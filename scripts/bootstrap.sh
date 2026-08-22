#!/usr/bin/env bash
# One-time (idempotent) dev-environment setup, run by hand on a fresh
# checkout: installs the host-toolchain packages every module's CMake host
# build needs, then the git hooks (scripts/install-git-hooks.sh).
# Linux only (Debian/Ubuntu apt). Real-target builds run in the
# docker/ci-toolchain/ image; only the vendor/ installers need a human
# (their CDN blocks scripted downloads).
#
#   ./scripts/bootstrap.sh [--check-only]   --check-only: report only, exit
#                                            nonzero if anything is missing

set -euo pipefail

check_only=0
[ "${1:-}" = "--check-only" ] && check_only=1

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
problems=0

# ---- host toolchain packages ----
# apt package name : the binary on PATH that proves it's installed.
packages=(
    "cmake:cmake"
    "build-essential:gcc"
    "cppcheck:cppcheck"
    "clang-format:clang-format"
)

missing_pkgs=()
for entry in "${packages[@]}"; do
    pkg="${entry%%:*}"
    bin="${entry##*:}"
    command -v "$bin" >/dev/null 2>&1 || missing_pkgs+=("$pkg")
done

if [ "${#missing_pkgs[@]}" -eq 0 ]; then
    echo "bootstrap: all host toolchain packages present (cmake, gcc, cppcheck, clang-format)."
elif [ "$check_only" = 1 ]; then
    echo "bootstrap: missing packages: ${missing_pkgs[*]}"
    echo "bootstrap: run: sudo apt-get install -y ${missing_pkgs[*]}"
    problems=1
elif command -v apt-get >/dev/null 2>&1; then
    echo "bootstrap: installing missing packages: ${missing_pkgs[*]}"
    sudo apt-get update
    sudo apt-get install -y "${missing_pkgs[@]}"
else
    echo "bootstrap: missing packages: ${missing_pkgs[*]}"
    echo "bootstrap: apt-get not found. Other package managers:"
    echo "  dnf/rpm:  sudo dnf install cmake gcc gcc-c++ make cppcheck clang-tools-extra"
    echo "  pacman:   sudo pacman -S cmake base-devel cppcheck clang"
    echo "  (package names above are Debian/Ubuntu's; adjust to your distro's naming)"
    problems=1
fi

# ---- git hooks ----
# The hooks dir lives in the common dir, shared by every worktree, so
# this reports the same state from a .worktrees/ checkout as from master.
if [ "$check_only" = 1 ]; then
    hooks_dir="$(cd "$(git -C "$repo_root" rev-parse --git-common-dir)" && pwd)/hooks"
    for hook in pre-commit commit-msg; do
        if [ -e "$hooks_dir/$hook" ]; then
            echo "bootstrap: $hook hook already installed."
        else
            echo "bootstrap: $hook hook not installed (run ./scripts/install-git-hooks.sh)."
            problems=1
        fi
    done
else
    "$repo_root/scripts/install-git-hooks.sh"
fi

# ---- Docker toolchain (real-target builds) ----
# `make check-vendor` and `make image` own the filenames, sizes, and
# URLs; this section only orchestrates them.
toolchain_ok=1
if ! command -v docker >/dev/null 2>&1; then
    echo "bootstrap: docker not found. Real-target builds run in a Docker image, see"
    echo "  DEVELOPMENT.md's Docker section; install Docker first."
    toolchain_ok=0
elif ! docker info >/dev/null 2>&1; then
    echo "bootstrap: docker found but the daemon is not reachable (is it running? are"
    echo "  you in the docker group?). Real-target builds need it."
    toolchain_ok=0
elif ! command -v make >/dev/null 2>&1; then
    echo "bootstrap: make not found, cannot run check-vendor/image. Install make, or"
    echo "  build the image yourself with:"
    echo "    docker build -t epic-hal-toolchain:local docker/ci-toolchain"
    toolchain_ok=0
fi

if [ "$toolchain_ok" = 1 ]; then
    # Create the drop location the guidance names; check-only never writes.
    if [ "$check_only" = 0 ]; then
        mkdir -p "$repo_root/docker/ci-toolchain/vendor"
    fi
    if make -C "$repo_root" check-vendor; then
        # Same tag as the Makefile's LOCAL_IMAGE.
        if docker image inspect epic-hal-toolchain:local >/dev/null 2>&1; then
            echo "bootstrap: docker toolchain image present (epic-hal-toolchain:local);"
            echo "  real-target builds are ready (make xc8-build / make mdb-test)."
        elif [ "$check_only" = 1 ]; then
            echo "bootstrap: docker toolchain image not built yet (run ./scripts/bootstrap.sh"
            echo "  or 'make image')."
            problems=1
        else
            make -C "$repo_root" image
        fi
    else
        echo "bootstrap: real-target toolchain not built yet. One-time manual step:"
        echo "  1. In a browser, download the two Microchip installers (links in the"
        echo "     make output above; the bot-challenge blocks scripted downloads):"
        echo "       docker/ci-toolchain/vendor/xc8-installer.run"
        echo "       docker/ci-toolchain/vendor/mplabx-installer.tar"
        echo "  2. Re-run ./scripts/bootstrap.sh (or run 'make image')."
        echo "  Host-simulation builds and tests work without this."
        [ "$check_only" = 1 ] && problems=1
    fi
elif [ "$check_only" = 1 ]; then
    problems=1
fi

if [ "$check_only" = 1 ] && [ "$problems" -eq 1 ]; then
    exit 1
fi
exit 0
