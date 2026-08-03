# Docker-first entry point for this whole repo: host-sim tests,
# real-target XC8 builds, the mdb (MPLAB SIM) verification gate, and a
# dev shell, all run inside docker/ci-toolchain/'s image so nobody needs
# XC8/MPLAB X/CMake installed on their own machine. See
# docs/docker-dev-plan.md for the design and docs/ci-plan.md for the
# image's own history (why it's built the way it is, the EULA
# redistribution constraint that keeps the pushed image private).
#
# This is NOT a top-level build (AGENTS.md: "No top-level build, build
# each module directly"); every target below shells into per-module
# cmake/make invocations inside the container, it does not introduce a
# unified CMake super-build.
#
# Quick start:
#   1. Drop the Microchip installers in docker/ci-toolchain/vendor/
#      (see `make check-vendor` for exact filenames and where to get them).
#   2. make image        # builds the toolchain image once, locally
#   3. make test          # every module's host-sim tests
#      make shell         # interactive shell, repo mounted at /repo
#
# Targets:
#   check-vendor   verify the vendor/ installer files are present
#   image          build the toolchain image locally (tag: pic8-hal-toolchain:local)
#   ci-image-push  push the local image to the private GHCR tag CI pulls
#                  (needs `docker login ghcr.io` with write:packages first;
#                  never run automatically by any other target)
#   test           host-sim build+test, every module (or MODULE=<name>)
#   xc8-build      real-target XC8 build for MODULE=<name> MCU=<mcu>
#   mdb-test       the mdb/MPLAB SIM gate for MODULE=<name> MCU=<mcu>
#                  DEVICE=<device> DFP=<pack> [WAIT_MS=<ms>]
#   shell          interactive shell in the toolchain container

.PHONY: check-vendor image ci-image-push test xc8-build mdb-test shell

# ─────────────────────────── image identity ─────────────────────────
# Same tag-resolution formula CI and scripts/sim-test-local.sh already
# use (read straight out of the Dockerfile's own ARGs), kept here in one
# place so ci-image-push pushes to the exact tag CI resolves and pulls,
# not a hand-typed guess that can drift from it.
DOCKERFILE      := docker/ci-toolchain/Dockerfile
XC8_VERSION     := $(shell grep -m1 '^ARG XC8_VERSION=' $(DOCKERFILE) | cut -d= -f2)
PIC16_DFP_VER   := $(shell grep -m1 '^ARG PIC16FXXX_DFP_VERSION=' $(DOCKERFILE) | cut -d= -f2)
PIC18_DFP_VER   := $(shell grep -m1 '^ARG PIC18FXXXX_DFP_VERSION=' $(DOCKERFILE) | cut -d= -f2)
PIC1216F1_DFP_VER := $(shell grep -m1 '^ARG PIC12_16F1XXX_DFP_VERSION=' $(DOCKERFILE) | cut -d= -f2)
MPLABX_VERSION  := $(shell grep -m1 '^ARG MPLABX_VERSION=' $(DOCKERFILE) | cut -d= -f2)
IMAGE_TAG       := xc8-v$(XC8_VERSION)-dfp$(PIC16_DFP_VER)-$(PIC18_DFP_VER)-$(PIC1216F1_DFP_VER)-mplabx$(MPLABX_VERSION)

LOCAL_IMAGE     := pic8-hal-toolchain:local
# GHCR_OWNER is not auto-derived from `git remote` here (unlike
# scripts/sim-test-local.sh, which only reads, never writes): pushing is
# a deliberate, infrequent, human-triggered action, so requiring an
# explicit override protects against silently pushing to the wrong
# owner's package if this repo is ever forked/cloned under another name.
GHCR_OWNER      ?=
CI_IMAGE        := ghcr.io/$(GHCR_OWNER)/pic8-hal-ci:$(IMAGE_TAG)

# --user matches the container process to the invoking host user, so
# build artifacts written to the bind-mounted repo (build/ dirs, .hex
# files, etc.) are owned by you, not root. Confirmed the hard way: without
# this, every container write lands as root and `rm -rf` from the host
# fails with Permission denied.
#
# The passwd/group bind-mounts + a writable HOME are needed on top of
# that, specifically for mdb-test/mdb.sh (MPLAB X's JVM): an arbitrary
# --user UID with no /etc/passwd entry makes Java's getpwuid()-based home
# directory lookup fail, and mdb.sh's own preference-directory creation
# then writes into a literal `?` directory at the container's CWD (which
# is the bind-mounted repo, so this corrupted the actual working tree
# during testing). Bind-mounting the real /etc/passwd + /etc/group lets
# the UID resolve to a real user (with the host's real $HOME path), and
# HOME_MOUNT gives that path something writable to land in, kept in
# ~/.cache (not the repo, not a Docker volume, since anonymous/named
# volumes default to root-owned and hit the exact same permission
# problem this is fixing). Confirmed fixed against a real mdb-test run.
HOME_MOUNT := $(HOME)/.cache/pic8-hal-toolchain-home
DOCKER_RUN := mkdir -p $(HOME_MOUNT) && docker run --rm --user $$(id -u):$$(id -g) \
	-v /etc/passwd:/etc/passwd:ro -v /etc/group:/etc/group:ro \
	-v $(HOME_MOUNT):$(HOME) \
	-v $(CURDIR):/repo -w /repo $(LOCAL_IMAGE)

# ─────────────────────────── vendor installers ───────────────────────
VENDOR_DIR := docker/ci-toolchain/vendor
XC8_INSTALLER := $(VENDOR_DIR)/xc8-installer.run
MPLABX_INSTALLER := $(VENDOR_DIR)/mplabx-installer.tar

check-vendor:
	@ok=1; \
	if [ ! -f "$(XC8_INSTALLER)" ] || [ "$$(stat -c%s "$(XC8_INSTALLER)" 2>/dev/null || echo 0)" -lt 10000000 ]; then \
		echo "missing (or too small): $(XC8_INSTALLER)"; \
		echo "  -> download the XC8 v$(XC8_VERSION) Linux installer (.run) from"; \
		echo "     https://www.microchip.com/mplab/compilers"; \
		echo "     and save it as $(XC8_INSTALLER)"; \
		ok=0; \
	fi; \
	if [ ! -f "$(MPLABX_INSTALLER)" ] || [ "$$(stat -c%s "$(MPLABX_INSTALLER)" 2>/dev/null || echo 0)" -lt 100000000 ]; then \
		echo "missing (or too small): $(MPLABX_INSTALLER)"; \
		echo "  -> download the MPLAB X IDE v$(MPLABX_VERSION) Linux installer,"; \
		echo "     tar it up as a single .tar (matching docker/ci-toolchain/"; \
		echo "     Dockerfile's own extraction step), from"; \
		echo "     https://www.microchip.com/mplab/mplab-x-ide"; \
		echo "     and save it as $(MPLABX_INSTALLER)"; \
		ok=0; \
	fi; \
	if [ "$$ok" -eq 0 ]; then \
		echo ""; \
		echo "Neither file can be fetched automatically: Microchip's download"; \
		echo "CDN sits behind a bot-challenge (see docs/ci-plan.md), so this"; \
		echo "is a one-time manual step, not a bug in this Makefile."; \
		exit 1; \
	fi; \
	echo "vendor/ OK: both installers present."

# ─────────────────────────── image build / push ──────────────────────
image: check-vendor
	docker build -t $(LOCAL_IMAGE) docker/ci-toolchain

# Never invoked by any other target. Requires the operator to already be
# `docker login`-ed to ghcr.io with a PAT that has write:packages (this
# target does not embed, request, or manage credentials itself); GHCR_OWNER
# must be passed explicitly (see the variable's own comment above).
ci-image-push: image
	@if [ -z "$(GHCR_OWNER)" ]; then \
		echo "error: pass GHCR_OWNER=<your-github-username-or-org>" >&2; \
		echo "  e.g. make ci-image-push GHCR_OWNER=apojomovsky" >&2; \
		exit 1; \
	fi
	docker tag $(LOCAL_IMAGE) $(CI_IMAGE)
	docker push $(CI_IMAGE)
	@echo "Pushed $(CI_IMAGE). CI's toolchain-image job will pull this exact tag."

# ─────────────────────────── host-sim tests ──────────────────────────
# Every module with a top-level CMakeLists.txt, same discovery
# host-tests.yml's own `discover` job uses; MODULE= scopes to one.
ALL_MODULES := $(shell git ls-files -- '*/CMakeLists.txt' | sed 's#/CMakeLists.txt$$##' | sort)
TEST_MODULES := $(if $(MODULE),$(MODULE),$(ALL_MODULES))

test: image
	@fail=0; \
	for m in $(TEST_MODULES); do \
		echo "=== $$m ==="; \
		$(DOCKER_RUN) bash -c "cd $$m && cmake -B build >/dev/null && cmake --build build && ctest --test-dir build --output-on-failure" || fail=1; \
	done; \
	exit $$fail

# ─────────────────────────── real-target XC8 build ───────────────────
# MODULE is the module dir (e.g. pic16f193x-hal); most modules have TWO
# mcu/*-mplabx dirs, one per family (see pic8-tick and friends), so which
# one to build can't be inferred from MODULE alone. Disambiguate by the
# MCU value's own shape instead: classic PIC16 variants end in a letter
# after 3 digits (873A/874A/876A/877A), PIC18 variants start with 18F,
# Enhanced Mid-range (193X) variants are 16F19xx with no trailing letter.
# Same three families scripts/ci-discover-xc8-matrix.py already encodes.
xc8-build: image
	@if [ -z "$(MODULE)" ] || [ -z "$(MCU)" ]; then \
		echo "usage: make xc8-build MODULE=<dir> MCU=<mcu> (e.g. MODULE=pic16f193x-hal MCU=16F1937)" >&2; \
		exit 1; \
	fi
	@case "$(MCU)" in \
		18F*) family_glob='pic18fxx5x*-mplabx'; dfp=Microchip.PIC18Fxxxx_DFP ;; \
		16F19[0-9][0-9]) family_glob='pic16f193x*-mplabx'; dfp=Microchip.PIC12-16F1xxx_DFP ;; \
		16F*) family_glob='pic16f87xa*-mplabx'; dfp=Microchip.PIC16Fxxx_DFP ;; \
		*) echo "error: MCU=$(MCU) does not match any known family shape" >&2; exit 1 ;; \
	esac; \
	mcu_dir=$$(find "$(MODULE)/mcu" -maxdepth 1 -type d -iname "$$family_glob" | head -1); \
	if [ -z "$$mcu_dir" ]; then \
		echo "error: no $$family_glob dir found under $(MODULE)/mcu (does this module support MCU=$(MCU)'s family?)" >&2; \
		exit 1; \
	fi; \
	echo "=== $$mcu_dir MCU=$(MCU) (DFP=$$dfp) ==="; \
	$(DOCKER_RUN) bash -c "make -C $$mcu_dir clean && make -C $$mcu_dir MCU=$(MCU) DFP_DIR=\$$XC8_INSTALL_DIR/pic/packs/$$dfp/xc8"

# ─────────────────────────── mdb / MPLAB SIM gate ────────────────────
# Thin wrapper around scripts/sim-mdb-run.sh, the exact same script CI
# and scripts/sim-test-local.sh call, so there is one source of truth
# for the mdb command sequence, not a fourth copy of it here.
mdb-test: image
	@if [ -z "$(MODULE)" ] || [ -z "$(MCU)" ] || [ -z "$(DEVICE)" ] || [ -z "$(DFP)" ]; then \
		echo "usage: make mdb-test MODULE=<mcu/*-mplabx dir> MCU=<mcu> DEVICE=<device> DFP=<pack> [WAIT_MS=<ms>]" >&2; \
		echo "  e.g. make mdb-test MODULE=pic8-tick/mcu/pic16f87xa-tick-mplabx MCU=16F877A DEVICE=PIC16F877A DFP=Microchip.PIC16Fxxx_DFP" >&2; \
		exit 1; \
	fi
	$(DOCKER_RUN) scripts/sim-mdb-run.sh local $(MCU) $(DEVICE) $(MODULE) $(DFP) $(WAIT_MS)

# ─────────────────────────── dev shell ───────────────────────────────
# Same --user/passwd/HOME fix as DOCKER_RUN (see its comment); a plain
# `docker run -it` here instead of reusing $(DOCKER_RUN) since that
# variable doesn't carry -it and isn't worth complicating for one target.
shell: image
	mkdir -p $(HOME_MOUNT) && docker run --rm -it --user $$(id -u):$$(id -g) \
		-v /etc/passwd:/etc/passwd:ro -v /etc/group:/etc/group:ro \
		-v $(HOME_MOUNT):$(HOME) \
		-v $(CURDIR):/repo -w /repo $(LOCAL_IMAGE) bash
