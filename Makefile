# Docker-first entry point for the whole repo: host-sim tests, real-target
# XC8 builds, the mdb (MPLAB SIM) gate, and a dev shell all run inside the
# docker/ci-toolchain/ image, so nobody needs XC8/MPLAB X/CMake installed
# locally. Not a top-level build (AGENTS.md): every target shells into
# per-module cmake/make invocations in the container.
#
# LOCAL_IMAGE is epic-hal-toolchain:local; the pushed CI tag is derived
# from the Dockerfile's ARGs (IMAGE_TAG below), so ci-image-push pushes
# exactly what CI resolves and pulls.

# ─────────────────────────── image identity ─────────────────────────
.PHONY: vendor-link check-vendor image ci-image-push test xc8-build epiccc-build epiccc-sim-sizes mdb-test mdb-epiccc mdb-hex sim-epiccc target-ci exec audit shell bootstrap doctor setup-hooks pre-pr-check
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

LOCAL_IMAGE     := epic-hal-toolchain:local
# GHCR_OWNER is not auto-derived from `git remote` here (unlike
# scripts/sim-test-local.sh, which only reads, never writes): pushing is
# a deliberate, infrequent, human-triggered action, so requiring an
# explicit override protects against silently pushing to the wrong
# owner's package if this repo is ever forked/cloned under another name.
GHCR_OWNER      ?=
CI_IMAGE        := ghcr.io/$(GHCR_OWNER)/epic-hal-ci:$(IMAGE_TAG)

# --user + passwd/group bind-mounts + a writable HOME_MOUNT (~/.cache):
# --user keeps repo artifacts host-owned; the mounts give mdb.sh's JVM
# a resolvable UID and writable home (else a literal `?` dir lands in
# the repo). HOME names that mount: the image default (/root) is
# root-owned, so $HOME-resolving tools (xc8 hexmate, signal 11 as
# non-root, epic-hal#280) crash where root succeeds. shell/exec/audit
# reuse this combo.
HOME_MOUNT := $(HOME)/.cache/epic-hal-toolchain-home
DOCKER_RUN := mkdir -p $(HOME_MOUNT) && docker run --rm --user $$(id -u):$$(id -g) \
	-e HOME=$(HOME) \
	-v /etc/passwd:/etc/passwd:ro -v /etc/group:/etc/group:ro \
	-v $(HOME_MOUNT):$(HOME) \
	-v $(CURDIR):/repo -w /repo $(LOCAL_IMAGE)

# ─────────────────────────── vendor installers ───────────────────────
VENDOR_DIR := docker/ci-toolchain/vendor
XC8_INSTALLER := $(VENDOR_DIR)/xc8-installer.run
MPLABX_INSTALLER := $(VENDOR_DIR)/mplabx-installer.tar

# The vendor installers are gitignored, so a fresh worktree under
# .worktrees/ starts without them and check-vendor fails there even
# though the main checkout has both and the image is already built.
# Hard-link them in (same filesystem, so it costs nothing); docker's
# build context needs real files, a symlink pointing out of the context
# is not followed.
MAIN_ROOT := $(shell cd "$$(git rev-parse --git-common-dir)/.." && pwd)

vendor-link:
	@[ "$(CURDIR)" = "$(MAIN_ROOT)" ] && exit 0; 	mkdir -p $(VENDOR_DIR); 	for f in xc8-installer.run mplabx-installer.tar; do 		src="$(MAIN_ROOT)/$(VENDOR_DIR)/$$f"; 		if [ ! -f "$(VENDOR_DIR)/$$f" ] && [ -f "$$src" ]; then 			ln "$$src" "$(VENDOR_DIR)/$$f" 2>/dev/null 				|| cp "$$src" "$(VENDOR_DIR)/$$f"; 			echo "vendor: linked $$f from the main checkout"; 		fi; 	done

check-vendor: vendor-link
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
# host-tests.yml's own `discover` job uses. MODULE names one module as
# a manifest id or a directory; anything else fails naming the valid
# ids, and an id with no host-sim build fails naming the host-testable ones.
ALL_MODULES := $(shell git ls-files -- '*/CMakeLists.txt' | sed 's#/CMakeLists.txt$$##' | sort)

test: image
	@fail=0; \
	mods="$(ALL_MODULES)"; \
	if [ -n "$(MODULE)" ]; then \
		mods=$$(python3 scripts/resolve_module.py --dir "$(MODULE)") || exit 1; \
		if [ ! -f "$$mods/CMakeLists.txt" ]; then \
			echo "error: module '$(MODULE)' has no host-sim build; host-testable modules: $(ALL_MODULES)" >&2; \
			exit 1; \
		fi; \
	fi; \
	for m in $$mods; do \
		echo "=== $$m ==="; \
		$(DOCKER_RUN) bash -c "cd $$m && cmake -B build >/dev/null && cmake --build build && ctest --test-dir build --output-on-failure" || fail=1; \
	done; \
	exit $$fail

# ─────────────────────────── real-target XC8 build ───────────────────
# Real-target build. Resolution runs on the host (needs python3), the
# emitted sh script runs in the container (which has xc8-cc and no
# python3, see docker/ci-toolchain/Dockerfile). MODULE is a manifest
# id or directory, e.g. epic-serial or lib/serial; epic_build.py
# resolves either to the id.
xc8-build: image
	@test -n "$(MODULE)" || { echo "usage: make xc8-build MODULE=epic-serial MCU=16F877A" >&2; exit 1; }
	@test -n "$(MCU)" || { echo "usage: make xc8-build MODULE=epic-serial MCU=16F877A" >&2; exit 1; }
	python3 scripts/epic_build.py build --module $(MODULE) --mcu $(MCU) \
	  --dfp-dir "$$(python3 -c "import sys; sys.path.insert(0,'scripts'); import epicmanifest as e; m=e.load(e.default_path()); print('/opt/microchip/xc8/v$(XC8_VERSION)/pic/packs/'+m.family_of('$(MCU)').dfp+'/xc8')")"
	$(DOCKER_RUN) sh build/$(MCU)/build.sh

# ─────────────────────────── real-target epic-cc build ───────────────
# `build/epiccc` keeps the two toolchains from colliding. epic-cc and
# its clang front end ship only in the epic-cc dev image (the hal
# toolchain image has XC8 and mdb, no clang), so the emitted script
# runs there by default. EPIC_CC_IMAGE overrides the image; EPIC_CC_BIN
# is a path inside it (defaults to the dev image's own cargo-built
# binary at /tmp/cargo-target/release/epic-cc), a bind-mount of the
# host's ~/.cache/epic-cc/target. That binary must be one the image can
# load: it is Ubuntu 22.04 (glibc 2.35), so a host cargo build on a newer
# glibc breaks every epiccc-build with `version GLIBC_2.39 not found`.
# Build the driver in the image instead (DEVELOPMENT.md, "The shared
# driver binary and the image's glibc").
# VARIANT=sim builds the MPLAB SIM diagnostic firmware with epic-cc
# instead, for like-for-like measurement against the XC8 sim recipe.
# Sim hexes land in build/epiccc-sim (never shared with target), and
# the driver writes its flash/RAM JSON beside the hex (needs a driver
# past epic-cc#698). The target path stays flag-free: the CI gate pins
# an older driver.
#
# EPIC_CC_HOST=1 runs the emitted script directly on the host instead,
# which is the one mode a host-built driver suits: the CI epiccc-gate
# job prepares the driver and clang itself (see DEVELOPMENT.md "The
# epiccc gate pin") and has no dev image to run in.
# The default is the checkout's content-addressed tag (epic-cc#736
# builds epic-cc-dev:local-<hash>, never the bare tag), resolved
# through epic-cc's canonical script. Still overrideable via
# EPIC_CC_IMAGE. Empty when no epic-cc checkout is linked; guarded below.
EPIC_CC_IMAGE ?= $(shell cd $(CURDIR)/epic-cc 2>/dev/null && bash scripts/dev-image-tag.sh 2>/dev/null)
# The container sees the shared cache as /tmp/cargo-target, the host as
# EPIC_CC_SHARED_BIN. EPIC_CC_BIN_DEFAULT names the in-container path so
# the staleness guard can tell a caller's override from the default
# (epic-hal#240).
EPIC_CC_BIN_DEFAULT := /tmp/cargo-target/release/epic-cc
EPIC_CC_SHARED_BIN  := $(HOME)/.cache/epic-cc/target/release/epic-cc
EPIC_CC_BIN ?= $(EPIC_CC_BIN_DEFAULT)
EPIC_CC_RUN := mkdir -p $(HOME_MOUNT) $(HOME)/.cache/epic-cc/target && docker run --rm \
	--user $$(id -u):$$(id -g) \
	-e HOME=$(HOME) \
	-v /etc/passwd:/etc/passwd:ro -v /etc/group:/etc/group:ro \
	-v $(HOME_MOUNT):$(HOME) \
	-v $(HOME)/.cache/epic-cc/target:/tmp/cargo-target \
	-v $(CURDIR):/repo -w /repo
VARIANT ?= target
EPICCC_BUILD_DIR := build/epiccc$(if $(filter-out target,$(VARIANT)),-$(VARIANT))
EPICCC_REPORT := $(if $(filter sim,$(VARIANT)),--report,)
# Optimization profile for the epic-cc driver (O0, O1, O2, Os). The Os
# default emits no flag: it is the driver's own default, and the
# target path stays flag-free so the CI gate's pinned driver
# (predating epic-cc#839) keeps building.
OPTLEVEL ?= Os
epiccc-build:
	@test -n "$(MODULE)" || { echo "usage: make epiccc-build MODULE=epic-serial MCU=16F877A [VARIANT=target|sim] [OPTLEVEL=O0|O1|O2|Os]" >&2; exit 1; }
	@test -n "$(MCU)" || { echo "usage: make epiccc-build MODULE=epic-serial MCU=16F877A [VARIANT=target|sim] [OPTLEVEL=O0|O1|O2|Os]" >&2; exit 1; }
	@if [ "$(EPIC_CC_HOST)" != "1" ] && [ -z "$(EPIC_CC_IMAGE)" ]; then \
		echo "epiccc-build: no EPIC_CC_IMAGE and no usable epic-cc checkout at $(CURDIR)/epic-cc" >&2; \
		echo "  (missing, or predates epic-cc#760 with no scripts/dev-image-tag.sh)." >&2; \
		echo "  link or clone a current one there, or set EPIC_CC_IMAGE explicitly." >&2; \
		exit 1; \
	fi
	@if [ "$(EPIC_CC_BIN)" = "$(EPIC_CC_BIN_DEFAULT)" ] && [ -e "$(CURDIR)/epic-cc/.git" ] \
			&& [ -z "$(EPIC_CC_ALLOW_STALE)" ] && [ -e "$(EPIC_CC_SHARED_BIN)" ]; then \
		checked=0; \
		head_sha=$$(git -C "$(CURDIR)/epic-cc" rev-parse --short HEAD 2>/dev/null); \
		bin_stamp=$$("$(EPIC_CC_SHARED_BIN)" --version 2>/dev/null); \
		case "$$bin_stamp" in \
			*+*) \
				bin_sha=$${bin_stamp##*+}; bin_sha=$${bin_sha%% *}; \
				case "$$bin_sha" in \
					''|*[!0-9a-f]*) ;; \
					*) \
						case "$$head_sha" in \
							''|*[!0-9a-f]*) ;; \
							*) \
								match=0; \
								case "$$head_sha" in "$$bin_sha"*) match=1;; esac; \
								case "$$bin_sha" in "$$head_sha"*) match=1;; esac; \
								if [ "$$match" = 0 ]; then \
									echo "epiccc-build: the cached driver does not match the epic-cc checkout." >&2; \
									echo "  binary: $(EPIC_CC_SHARED_BIN) (stamp $$bin_sha)" >&2; \
									echo "  epic-cc: $$head_sha ($(CURDIR)/epic-cc)" >&2; \
									echo "  A stale driver reruns older compiler bugs under new failure signatures (epic-hal#240)." >&2; \
									echo "  Rebuild it in the dev image, from the epic-cc checkout:" >&2; \
									echo "    cd epic-cc && make exec TARGET_CACHE=\$$HOME/.cache/epic-cc/target CMD='cargo build --release -p driver'" >&2; \
									echo "  Then set EPIC_CC_BIN to a driver of your own, or EPIC_CC_ALLOW_STALE=1 to use this one." >&2; \
									exit 1; \
								fi; \
								checked=1; \
								;; \
						esac \
						;; \
				esac \
				;; \
		esac; \
		if [ "$$checked" = 0 ]; then \
			head_ct=$$(git -C "$(CURDIR)/epic-cc" log -1 --format=%ct -- crates Cargo.toml Cargo.lock 2>/dev/null); \
			bin_ct=$$(stat -c %Y "$(EPIC_CC_SHARED_BIN)" 2>/dev/null || echo 0); \
			case "$$head_ct" in \
				''|*[!0-9]*) ;; \
				*) \
					if [ "$$head_ct" -gt "$$bin_ct" ]; then \
						head_log_sha=$$(git -C "$(CURDIR)/epic-cc" log -1 --format=%h -- crates Cargo.toml Cargo.lock 2>/dev/null); \
						echo "epiccc-build: the cached driver is older than the epic-cc compiler sources." >&2; \
						echo "  binary: $(EPIC_CC_SHARED_BIN) ($$(date -d @$$bin_ct '+%Y-%m-%d %H:%M'))" >&2; \
						echo "  epic-cc: $$head_log_sha ($$(date -d @$$head_ct '+%Y-%m-%d %H:%M'))" >&2; \
						echo "  The driver predates sha stamping (epic-cc#525); rebuild it to get an exact identity check." >&2; \
						echo "  A stale driver reruns older compiler bugs under new failure signatures (epic-hal#240)." >&2; \
						echo "  Rebuild it in the dev image, from the epic-cc checkout:" >&2; \
						echo "    cd epic-cc && make exec TARGET_CACHE=\$$HOME/.cache/epic-cc/target CMD='cargo build --release -p driver'" >&2; \
						echo "  If the sources are already built, cargo is a no-op and this keeps firing;" >&2; \
						echo "  then set EPIC_CC_BIN to a driver of your own, or EPIC_CC_ALLOW_STALE=1 to use this one." >&2; \
						exit 1; \
					fi \
					;; \
			esac; \
		fi; \
	fi
	python3 scripts/epic_build.py build --module $(MODULE) --mcu $(MCU) --variant $(VARIANT) --toolchain epic-cc --epic-cc $(EPIC_CC_BIN) --build-dir $(EPICCC_BUILD_DIR) $(EPICCC_REPORT) --opt-level $(OPTLEVEL)
ifeq ($(EPIC_CC_HOST),1)
	sh $(EPICCC_BUILD_DIR)/$(MCU)/build.sh
else
	$(EPIC_CC_RUN) -e PIC8_CLANG_UNWRAPPED=/opt/clang/bin/clang \
		-e PIC8_CLANG_RESOURCE_DIR=/opt/clang/lib/clang/20 \
		$(EPIC_CC_IMAGE) sh $(EPICCC_BUILD_DIR)/$(MCU)/build.sh
endif
# ─────────────────── epic-cc sim size table ──────────────────────────
# One-shot like-for-like measurement for the RAM audit and the
# benchmark matrix: every audit demo's sim variant through the epic-cc
# path above, then a flash/RAM table read off the drivers' JSON
# reports. Encoder fits since the phase-4 extraction (epic-hal#327)
# on a driver past epic-cc#935, so every row must print and any
# failure exits nonzero.
EPICCC_SIM_SPECS := epic-menu-demo:18F4550 epic-control-demo:18F4550 epic-pid:18F4550 epic-bridge-demo:18F4550 epic-encoder:16F877A
epiccc-sim-sizes:
	@fail=0; \
	for spec in $(EPICCC_SIM_SPECS); do \
		m=$${spec%%:*}; c=$${spec##*:}; \
		$(MAKE) --no-print-directory epiccc-build MODULE=$$m MCU=$$c VARIANT=sim || fail=1; \
	done; \
	python3 scripts/epiccc_sim_sizes.py $(EPICCC_SIM_SPECS) || fail=1; \
	exit $$fail

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
		echo "usage: make mdb-test MODULE=<module id or dir> MCU=<mcu> DEVICE=<device> [WAIT_MS=<ms>] [MODE=uart|gpio] [EXTRA_MDB=<mdb commands>] [EEPROM_WRITES=<n>]" >&2; \
		echo "  MODE=uart (default) for PIC16F87XA/PIC18Fxxxx (UART capture);" >&2; \
		echo "  MODE=gpio for PIC16F193X/PIC16F1508 (RA0 register readback)." >&2; \
		echo "  EXTRA_MDB: extra mdb commands inserted before quit, e.g." >&2; \
		echo "    EXTRA_MDB=\$'print INTCON\\nprint PIR1' for register-level debugging." >&2; \
		echo "  EEPROM_WRITES: halt+complete EEPROM-write cycles emitted before the final run" >&2; \
		echo "    (MPLAB SIM never completes a CPU-executed EEPROM write; only epic-settings' gate needs this)." >&2; \
		echo "  e.g. make mdb-test MODULE=epic-tick MCU=16F877A DEVICE=PIC16F877A" >&2; \
		exit 1; \
	fi
	MID=$$(python3 scripts/resolve_module.py --id "$(MODULE)") || exit 1; \
	python3 scripts/epic_build.py build --module $$MID --mcu $(MCU) --variant sim \
	  --build-dir build-sim/$$MID \
	  --dfp-dir "$$(python3 -c "import sys; sys.path.insert(0,'scripts'); import epicmanifest as e; m=e.load(e.default_path()); print('/opt/microchip/xc8/v$(XC8_VERSION)/pic/packs/'+m.family_of('$(MCU)').dfp+'/xc8')")"
	MID=$$(python3 scripts/resolve_module.py --id "$(MODULE)") || exit 1; \
	$(DOCKER_RUN) env \
	  $(if $(filter toggle,$(or $(MODE),uart)),TOGGLE_REG=$(or $(REG),PORTB) TOGGLE_BIT=$(or $(BIT),0) TOGGLE_SAMPLES=$(or $(SAMPLES),12) TOGGLE_STEPI=$(or $(STEPI),200000),) \
	  scripts/sim-mdb-run.sh local $(MCU) $(DEVICE) $$MID $(or $(WAIT_MS),2000) $(or $(MODE),uart) "$(EXTRA_MDB)" $(or $(EEPROM_WRITES),$(if $(filter epic-settings lib/settings,$(patsubst %/,%,$(MODULE))),24,0))

mdb-epiccc: image
	@if [ -z "$(MODULE)" ] || [ -z "$(MCU)" ] || [ -z "$(DEVICE)" ]; then \
		echo "usage: make mdb-epiccc MODULE=<module id or dir> MCU=<mcu> DEVICE=<device> [REG=PORTB] [BIT=0] [SAMPLES=12] [STEPI=200000]" >&2; \
		echo "  Runs an ALREADY BUILT epic-cc hex under MPLAB SIM and requires REG bit BIT to" >&2; \
		echo "  change across SAMPLES samples of STEPI instructions each. Deterministic:" >&2; \
		echo "  stepi, not wall-clock wait, so the sequence is identical run to run." >&2; \
		echo "  Build the hex first where epic-cc lives (its compiler and clang are not in" >&2; \
		echo "  this image):" >&2; \
		echo "    python3 scripts/epic_build.py build --module <m> --mcu <mcu> \\" >&2; \
		echo "      --toolchain epic-cc --epic-cc <path> --build-dir build-sim/<m>" >&2; \
		echo "    then run the emitted build-sim/<m>/<mcu>/build.sh there" >&2; \
		echo "  e.g. make mdb-epiccc MODULE=pic16f88x-hal MCU=16F887 DEVICE=PIC16F887" >&2; \
		exit 1; \
	fi
	MID=$$(python3 scripts/resolve_module.py --id "$(MODULE)") || exit 1; \
	$(DOCKER_RUN) env SIM_MDB_SKIP_BUILD=1 \
	  TOGGLE_REG=$(or $(REG),PORTB) TOGGLE_BIT=$(or $(BIT),0) \
	  TOGGLE_SAMPLES=$(or $(SAMPLES),12) TOGGLE_STEPI=$(or $(STEPI),200000) \
	  scripts/sim-mdb-run.sh local $(MCU) $(DEVICE) $$MID 0 toggle
# ─────────────────── mdb run on an arbitrary hex ─────────────────────
# Program an existing hex under MPLAB SIM and run EXTRA_MDB (register
# reads) after the first wait. Unlike mdb-test there is no HARNESS=sim
# rebuild: this is the gate for hexes another toolchain produced, e.g.
# epiccc-build's output, where the acceptance is a register read, not a
# UART marker. Thin wrapper around scripts/mdb-hex-run.sh, the same
# "one source of truth" shape sim-mdb-run.sh gives the harness gates:
# the CI epiccc-gate job runs that script directly. HEX is
# repo-relative, DEVICE is the MPLAB part name, and EXTRA_MDB uses
# backslash-n escapes for newlines (single shell value, expanded inside
# the container), e.g.
#   EXTRA_MDB='print PORTB\nprint TMR0'
mdb-hex: image
	@if [ -z "$(HEX)" ] || [ -z "$(DEVICE)" ]; then \
		echo "usage: make mdb-hex HEX=<hex> DEVICE=<device> [EXTRA_MDB=<mdb commands>] [WAIT_MS=<ms>]" >&2; \
		echo "  EXTRA_MDB: mdb commands after the first wait, \\n-escaped, e.g." >&2; \
		echo "    EXTRA_MDB='print PORTB\\nprint TMR0'" >&2; \
		echo "  e.g. make mdb-hex HEX=build/epiccc/16F887-blink.hex DEVICE=PIC16F887" >&2; \
		exit 1; \
	fi
	$(DOCKER_RUN) scripts/mdb-hex-run.sh /repo/$(HEX) $(DEVICE) $(or $(WAIT_MS),2000) "$(EXTRA_MDB)"

# ─────── epic-cc sim gate (HAL-4) ───────
# The public gate: run an epic-cc hex under its own ISA simulator
# (crates/sim, via scripts/sim-runner), replacing the mdb half of
# the epiccc-gate job (epic-hal#60): no XC8, no mdb, no private
# image. Needs an epic-cc checkout at <repo>/epic-cc (locally
# `ln -s ~/projects/epic-cc epic-cc`). WATCH requires a toggle;
# IRQ_EVERY injects the flag through its enable bit when GIE allows
# for a timer crates/sim lacks.
sim-epiccc:
	@if [ ! -e "$(CURDIR)/epic-cc/crates/sim" ]; then \
		echo "sim-epiccc: no epic-cc checkout at $(CURDIR)/epic-cc." >&2; \
		echo "  link or clone one there, e.g.: ln -s ~/projects/epic-cc epic-cc" >&2; \
		exit 1; \
	fi
	@test -n "$(DEVICE)" || { echo "usage: make sim-epiccc HEX=build/epiccc/16F887-tick.hex DEVICE=16F887 [WATCH=PORTB:0] [SAMPLES=24] [STEPS=500000] [IRQ_EVERY=5000 IRQ_FLAG=PIR1:1 IRQ_ENABLE=PIE1:1]" >&2; exit 1; }
	@test -n "$(EPIC_CC_IMAGE)" || { echo "sim-epiccc: checkout at $(CURDIR)/epic-cc predates epic-cc#760 (no scripts/dev-image-tag.sh); update it or set EPIC_CC_IMAGE." >&2; exit 1; }
	@test -f "$(CURDIR)/$(HEX)" || { echo "sim-epiccc: no hex at $(HEX)" >&2; exit 1; }
	mkdir -p $(HOME_MOUNT) && docker run --rm --user $$(id -u):$$(id -g) \
		-e HOME=$(HOME) \
		-v /etc/passwd:/etc/passwd:ro -v /etc/group:/etc/group:ro \
		-v $(HOME_MOUNT):$(HOME) \
		-v $(realpath $(CURDIR)/epic-cc):$(HOME)/projects/epic-cc \
		-e CARGO_HOME=$(HOME)/.cargo-sim-runner -e CARGO_TARGET_DIR=$(HOME)/.target-sim-runner \
		-v $(CURDIR):/repo \
		-w /repo/scripts/sim-runner $(EPIC_CC_IMAGE) \
		cargo run --release --locked -- \
		--hex /repo/$(HEX) --device $(DEVICE) --watch $(or $(WATCH),PORTB:0) \
		--samples $(or $(SAMPLES),24) --steps $(or $(STEPS),500000) \
		$(if $(IRQ_EVERY),--irq-every $(IRQ_EVERY) --irq-flag $(IRQ_FLAG) --irq-enable $(IRQ_ENABLE),)

# ─────────────────────────── dev shell ───────────────────────────────
# Same --user/passwd/HOME fix as DOCKER_RUN (see its comment); a plain
# `docker run -it` here instead of reusing $(DOCKER_RUN) since that
# variable doesn't carry -it and isn't worth complicating for one target.
shell: image
	mkdir -p $(HOME_MOUNT) && docker run --rm -it --user $$(id -u):$$(id -g) \
		-e HOME=$(HOME) \
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
# container for the DFP headers and xc8-cc. The dfp-pin audit is pure
# text (Dockerfile, manifest, reference projects), so it needs neither.
audit: image
	python3 scripts/sfr-map-audit.py
	python3 scripts/config-key-audit.py
	python3 scripts/statics-audit.py
	python3 scripts/hex-identity-audit.py
	python3 scripts/dfp-pin-audit.py
	python3 scripts/epiccc-slice-audit.py
	python3 scripts/mplabx-audit.py

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

# ───────────────────── local-only developer rituals ─────────────────
# Host-side bash/python3, no container: these gate the branch, not the
# build, so they must run before `make image` is even possible.

bootstrap:
	@bash scripts/bootstrap.sh

doctor:
	@bash scripts/bootstrap.sh --check-only

setup-hooks:
	@bash scripts/install-git-hooks.sh

pre-pr-check: ## Takeoff ritual before opening a PR; TEST=1 runs the suite
	@bash scripts/pre-pr-check.sh $(if $(TEST),--test,)
