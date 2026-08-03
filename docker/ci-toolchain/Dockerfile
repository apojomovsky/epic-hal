# CI toolchain image for docs/ci-plan.md Phase 1 (XC8) and Phase 2 (MPLAB X
# IDE, for `mdb`/MPLAB SIM). Tagged by the version pins below.
#
# Two consumers, one image (docs/docker-dev-plan.md):
#   - CI: `.github/workflows/xc8-build.yml`/`sim-tests.yml` `docker pull`
#     the version-pinned tag from GHCR; they do NOT build this image
#     themselves anymore (see that plan's "CI simplification").
#   - Local dev: the root `Makefile` (`make image`) builds this image
#     directly from `vendor/` (populated by hand, see that directory's
#     own notes and `make check-vendor`), and `make ci-image-push` can
#     push the result to the same private GHCR tag CI pulls, so a human
#     with `write:packages` access is how this image actually gets
#     updated; CI never builds or pushes it.
#
# Versions:
#   - XC8 v4.00: license-free (no PRO gate at all, so no --LicenseType flag
#     below, unlike older XC8), newer than what the rest of this repo's
#     Makefiles/docs currently name (v3.10). Deliberately bumped ahead of
#     the repo's documented pin rather than matching it: see "which XC8
#     version to pin" in docs/ci-plan.md's open questions for the tradeoff.
#   - Microchip.PIC16Fxxx_DFP.1.7.162, Microchip.PIC18Fxxxx_DFP.1.7.171,
#     Microchip.PIC12-16F1xxx_DFP.1.9.258 (pic16f193x-hal, Enhanced
#     Mid-range, added when that family's real-target build was verified
#     locally against this exact version): none of the three is bundled
#     with the standalone XC8 v4.00 installer (verified locally: `find
#     /opt/microchip/xc8/v4.00/pic` has no `packs/` directory at all).
#     This repo's mcu/*-mplabx/README.md files assumed PIC16Fxxx_DFP
#     ships bundled, that was true of the full MPLAB X IDE's pack
#     manager, not the bare compiler installer used here.
#   - MPLAB X IDE v6.35: installed only for `mplab_platform/bin/mdb.sh`
#     (headless, scriptable, distinct from the NetBeans GUI, see
#     docs/ci-plan.md's Phase 2 findings). Component selection installs
#     just IDE + 8-bit MCU device support (`--ipe 0 --16bitmcu 0
#     --32bitmcu 0`), skipping ~8GB of dsPIC/PIC24/PIC32/ARM support this
#     repo will never touch. This is the FULL, untrimmed 8-bit install
#     (~7.3GB): a multi-stage build that keeps only mdbcore/java/packs and
#     drops the NetBeans GUI/MCC/thirdparty tooling is real follow-up work,
#     deliberately deferred (see docs/ci-plan.md Phase 2), not attempted
#     here, `mdb.sh`'s actual runtime dependency set (a NetBeans-platform
#     module classpath, not just a few obvious directories) isn't obvious
#     enough to prune correctly without real trial-and-error verification.
#
# Neither the XC8 installer nor the MPLAB X IDE installer is fetched by
# this Dockerfile at all, on purpose. ww1.microchip.com sits behind an
# Akamai bot-challenge that returns a JavaScript challenge page (not the
# file) to any non-browser client, confirmed against both a local `docker
# build` and a real GitHub Actions run for XC8, and curl-impersonate
# (spoofs Chrome's TLS fingerprint) still didn't get past it, it's a JS
# challenge, not just TLS/header fingerprinting, so trying to automate
# past it was abandoned. The first fix for XC8 (re-hosting the installer
# as a public GitHub Release asset) turned out to itself be a licensing
# problem: Microchip's EULA (pulled from the installed product's own
# docs/MPLABIDELicense.htm) grants a *use* license, not a redistribution
# one, and `docker pull`ing the resulting image worked with zero
# authentication, i.e. we were redistributing it. See docs/ci-plan.md for
# the full account.
#
# Fix, applied to both installers: `.github/workflows/xc8-build.yml`'s
# `ci-assets`/`ci-assets-mplabx` jobs extract each installer from a
# PRIVATE `ghcr.io/.../pic8-hal-ci-assets` image (built by
# docker/ci-assets/Dockerfile, a plain file carrier, see its own header)
# into vendor/ in this Dockerfile's build context, purely local files on
# the runner's disk at that point, never a public URL. This Dockerfile
# just COPYs them in. DFPs are unaffected by any of this, fetched straight
# from packs.download.microchip.com (a different, S3/CloudFront-backed
# host, no bot-challenge, and Microchip's own DFP distribution terms are
# separate from the compiler EULA, confirmed working both locally and
# from a real GitHub Actions run).

FROM debian:12-slim

ARG XC8_VERSION=4.00
ARG PIC16FXXX_DFP_VERSION=1.7.162
ARG PIC18FXXXX_DFP_VERSION=1.7.171
ARG PIC12_16F1XXX_DFP_VERSION=1.9.258
ARG MPLABX_VERSION=6.35

ENV DEBIAN_FRONTEND=noninteractive
ENV XC8_INSTALL_DIR=/opt/microchip/xc8/v${XC8_VERSION}
ENV MPLABX_INSTALL_DIR=/opt/microchip/mplabx/v${MPLABX_VERSION}
ENV HOME=/root
ENV USER=root

RUN apt-get update -yq \
    && apt-get install -yq --no-install-recommends \
        ca-certificates curl unzip make tar \
        libgtk-3-0 libxext6 libxrender1 libxtst6 libxi6 libusb-1.0-0 libudev1 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

COPY vendor/xc8-installer.run /tmp/installer.run
RUN set -eu; \
    size=$(stat -c%s /tmp/installer.run); \
    if [ "$size" -lt 10000000 ]; then \
        echo "error: /tmp/installer.run is only ${size} bytes," \
             "expected a multi-megabyte .run file; check the ci-assets" \
             "job actually extracted it into the build context" >&2; \
        exit 1; \
    fi; \
    chmod +x /tmp/installer.run; \
    /tmp/installer.run --mode unattended --unattendedmodeui none \
        --prefix "${XC8_INSTALL_DIR}"; \
    rm /tmp/installer.run

# None of the three DFPs is bundled with the standalone XC8 v4.00
# installer (see header comment). Unzipped to the paths this repo's
# mcu/*-mplabx/Makefile DFP_DIR override targets (xc8-build.yml passes
# DFP_DIR explicitly, these paths don't match the Makefiles' own
# v3.10-pinned defaults).
RUN set -eu; \
    for pack_ver in \
        "Microchip.PIC16Fxxx_DFP:${PIC16FXXX_DFP_VERSION}" \
        "Microchip.PIC18Fxxxx_DFP:${PIC18FXXXX_DFP_VERSION}" \
        "Microchip.PIC12-16F1xxx_DFP:${PIC12_16F1XXX_DFP_VERSION}"; \
    do \
        pack="${pack_ver%%:*}"; \
        ver="${pack_ver##*:}"; \
        dest="${XC8_INSTALL_DIR}/pic/packs/${pack}"; \
        mkdir -p "$dest"; \
        curl -fsSL -o "/tmp/${pack}.atpack" \
            "https://packs.download.microchip.com/${pack}.${ver}.atpack"; \
        unzip -q "/tmp/${pack}.atpack" -d "$dest"; \
        rm "/tmp/${pack}.atpack"; \
    done

# MPLAB X IDE, for mdb.sh only (see header comment). --ide 1 --ipe 0
# --8bitmcu 1 --16bitmcu 0 --32bitmcu 0 --othermcu 0: only what this repo
# needs. --collectInfo 0 --collectMyMicrochipInfo 0: no telemetry/opt-in
# prompts in an unattended CI image. HOME/USER above are required, the
# installer's root check fails without them even running as uid 0.
COPY vendor/mplabx-installer.tar /tmp/mplabx.tar
RUN set -eu; \
    size=$(stat -c%s /tmp/mplabx.tar); \
    if [ "$size" -lt 100000000 ]; then \
        echo "error: /tmp/mplabx.tar is only ${size} bytes," \
             "expected a multi-hundred-megabyte .tar file; check the" \
             "ci-assets-mplabx job actually extracted it into the build" \
             "context" >&2; \
        exit 1; \
    fi; \
    cd /tmp && tar -xf mplabx.tar && rm mplabx.tar; \
    installer="/tmp/MPLABX-v${MPLABX_VERSION}-linux-installer.sh"; \
    chmod +x "$installer"; \
    "$installer" --nox11 -- \
        --mode unattended --unattendedmodeui none \
        --ide 1 --ipe 0 --8bitmcu 1 --16bitmcu 0 --32bitmcu 0 --othermcu 0 \
        --collectInfo 0 --collectMyMicrochipInfo 0 \
        --installdir "${MPLABX_INSTALL_DIR}"; \
    rm "$installer"

ENV PATH="${XC8_INSTALL_DIR}/bin:${MPLABX_INSTALL_DIR}/mplab_platform/bin:${PATH}"
