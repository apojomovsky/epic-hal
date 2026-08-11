# Docker-first entry point for the whole repo: host-sim tests, real-target
# XC8 builds, the mdb (MPLAB SIM) gate, and a dev shell all run inside the
# docker/ci-toolchain/ image, so nobody needs XC8/MPLAB X/CMake installed
# locally. Not a top-level build (AGENTS.md): every target shells into
# per-module cmake/make invocations in the container.
#
# LOCAL_IMAGE is pic8-hal-toolchain:local; the pushed CI tag is derived
# from the Dockerfile's ARGs (IMAGE_TAG below), so ci-image-push pushes
# exactly what CI resolves and pulls.

.PHONY: check-vendor image ci-image-push test xc8-build mdb-test target-ci exec audit shell

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

# --user + passwd/group bind-mounts + a writable HOME_MOUNT (~/.cache):
# --user keeps build artifacts in the bind-mounted repo host-owned (root-
# owned files break host `rm -rf`); the mounts let mdb.sh's JVM resolve the
# UID and give its preferences a writable home, without which it wrote a
# literal `?` dir into the repo during testing. Every target that writes to
# the mounted repo needs this combo (shell/exec/audit reuse it).
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
		echo "missing (or too small, expected at least ~10 MB): $(XC8_INSTALLER)"; \
		echo "  -> download the XC8 v$(XC8_VERSION) Linux installer (.run) from"; \
		echo "     https://www.microchip.com/mplab/compilers"; \
		echo "     and save it as $(XC8_INSTALLER)"; \
		ok=0; \
	fi; \
	if [ ! -f "$(MPLABX_INSTALLER)" ] || [ "$$(stat -c%s "$(MPLABX_INSTALLER)" 2>/dev/null || echo 0)" -lt 100000000 ]; then \
		echo "missing (or too small, expected at least ~100 MB): $(MPLABX_INSTALLER)"; \
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
		echo "CDN sits behind a bot-challenge, so this"; \
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
# Real-target build. Resolution runs on the host (needs python3), the
# emitted sh script runs in the container (which has xc8-cc and no
# python3, see docker/ci-toolchain/Dockerfile). MODULE is a manifest
# module name, e.g. epic-serial, not a path: the mcu/*-mplabx dirs it
# used to name are gone.
xc8-build: image
	@test -n "$(MODULE)" || { echo "usage: make xc8-build MODULE=epic-serial MCU=16F877A" >&2; exit 1; }
	@test -n "$(MCU)" || { echo "usage: make xc8-build MODULE=epic-serial MCU=16F877A" >&2; exit 1; }
	python3 scripts/epic_build.py build --module $(MODULE) --mcu $(MCU) \
	  --dfp-dir "$$(python3 -c "import sys; sys.path.insert(0,'scripts'); import epicmanifest as e; m=e.load(e.default_path()); print('/opt/microchip/xc8/v$(XC8_VERSION)/pic/packs/'+m.family_of('$(MCU)').dfp+'/xc8')")"
	$(DOCKER_RUN) sh build/$(MCU)/build.sh

# ─────────────────────────── mdb / MPLAB SIM gate ────────────────────
# Thin wrapper around scripts/sim-mdb-run.sh, the exact same script CI
# and scripts/sim-test-local.sh call, so there is one source of truth
# for the mdb command sequence, not a fourth copy of it here. Resolution
# (epic_build.py build --variant sim, needs python3) runs on the host,
# before $(DOCKER_RUN); sim-mdb-run.sh itself only ever executes the
# pre-emitted script plus mdb.sh inside the container, which has no
# python3.
mdb-test: image
	@if [ -z "$(MODULE)" ] || [ -z "$(MCU)" ] || [ -z "$(DEVICE)" ]; then \
		echo "usage: make mdb-test MODULE=<manifest module> MCU=<mcu> DEVICE=<device> [WAIT_MS=<ms>] [MODE=uart|gpio] [EXTRA_MDB=<mdb commands>] [EEPROM_WRITES=<n>]" >&2; \
		echo "  MODE=uart (default) for PIC16F87XA/PIC18Fxxxx (UART capture);" >&2; \
		echo "  MODE=gpio for PIC16F193X (RA0 register readback)." >&2; \
		echo "  EXTRA_MDB: extra mdb commands inserted before quit, e.g." >&2; \
		echo "    EXTRA_MDB=\$'print INTCON\\nprint PIR1' for register-level debugging." >&2; \
		echo "  EEPROM_WRITES: halt+complete EEPROM-write cycles emitted before the final run" >&2; \
		echo "    (MPLAB SIM never completes a CPU-executed EEPROM write; only epic-settings' gate needs this)." >&2; \
		echo "  e.g. make mdb-test MODULE=epic-tick MCU=16F877A DEVICE=PIC16F877A" >&2; \
		exit 1; \
	fi
	python3 scripts/epic_build.py build --module $(MODULE) --mcu $(MCU) --variant sim \
	  --build-dir build-sim/$(MODULE) \
	  --dfp-dir "$$(python3 -c "import sys; sys.path.insert(0,'scripts'); import epicmanifest as e; m=e.load(e.default_path()); print('/opt/microchip/xc8/v$(XC8_VERSION)/pic/packs/'+m.family_of('$(MCU)').dfp+'/xc8')")"
	$(DOCKER_RUN) scripts/sim-mdb-run.sh local $(MCU) $(DEVICE) $(MODULE) $(or $(WAIT_MS),2000) $(or $(MODE),uart) "$(EXTRA_MDB)" $(or $(EEPROM_WRITES),$(if $(filter epic-settings,$(MODULE)),24,0))

# ─────────────────────────── dev shell ───────────────────────────────
# Same --user/passwd/HOME fix as DOCKER_RUN (see its comment); a plain
# `docker run -it` here instead of reusing $(DOCKER_RUN) since that
# variable doesn't carry -it and isn't worth complicating for one target.
shell: image
	mkdir -p $(HOME_MOUNT) && docker run --rm -it --user $$(id -u):$$(id -g) \
		-v /etc/passwd:/etc/passwd:ro -v /etc/group:/etc/group:ro \
		-v $(HOME_MOUNT):$(HOME) \
		-v $(CURDIR):/repo -w /repo $(LOCAL_IMAGE) bash

# ─────────────────────── one-off container command ──────────────────
# Escape hatch for any ad-hoc command inside the toolchain container,
# with the same --user/passwd/HOME mount plumbing as every other target
# (probes, clean rebuilds, custom mdb sessions). The CMD is handed to
# `bash -c`, so avoid double quotes inside it:
#   make exec CMD='bash scripts/sim-mdb-run.sh pic16f87xa 16F877A PIC16F877A epic-tick 60000 gpio'
exec: image
	@test -n "$(CMD)" || { echo "usage: make exec CMD='bash scripts/... args'" >&2; exit 1; }
	$(DOCKER_RUN) bash -c "$(CMD)"

# ─────────────────────────────── audits ─────────────────────────────
# The static device-data audits (the CI target job's SFR-map +
# config-key step, reproduced locally): HAL SFR maps against the DFP
# proc headers, every manifest example's config keys/values against
# the compiler's config database, and every matrix .hex rebuilt twice
# into separate dirs and sha256-compared (layout drift as a reviewable
# diff, not a flaky gate). Host-side python3, shells into the toolchain
# container for the DFP headers and xc8-cc.
audit: image
	python3 scripts/sfr-map-audit.py
	python3 scripts/config-key-audit.py
	python3 scripts/statics-audit.py
	python3 scripts/hex-identity-audit.py

# ──────────────────── local replica of CI's target job ──────────────
# One command to reproduce the whole "target" CI job locally: emit the
# real-target matrix, the sim variants, and the bundles (host, needs
# python3), then run the build loop, the mdb gate loop, and the bundle
# gate in the toolchain container, using the exact scripts CI runs, in
# the same order. Summaries land in ci-summary-*.md (gitignored).
# The bundle gate extracts to /isolated, which is container-internal and
# not bind-mounted, so that one step runs as root exactly like CI (a
# --user container cannot create /isolated); everything else uses
# DOCKER_RUN so build artifacts stay host-owned.
target-ci: image
	python3 scripts/ci-local-emit.py
	$(DOCKER_RUN) bash scripts/ci-target-build.sh matrix.txt ci-summary-build.md
	$(DOCKER_RUN) bash scripts/ci-target-sim.sh ci-summary-sim.md
	docker run --rm -v $(CURDIR):/repo -w /repo $(LOCAL_IMAGE) \
		bash scripts/ci-target-bundle.sh bundles ci-summary-bundle.md
	@cat ci-summary-build.md ci-summary-sim.md ci-summary-bundle.md
	$(DOCKER_RUN) bash scripts/ci-target-build.sh matrix.txt ci-summary-build.md
	$(DOCKER_RUN) bash scripts/ci-target-sim.sh ci-summary-sim.md
	docker run --rm -v $(CURDIR):/repo -w /repo $(LOCAL_IMAGE) \
		bash scripts/ci-target-bundle.sh bundles ci-summary-bundle.md
	@cat ci-summary-build.md ci-summary-sim.md ci-summary-bundle.md
