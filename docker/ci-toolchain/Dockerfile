# CI toolchain image for docs/ci-plan.md Phase 1: XC8 only (no MPLAB X IDE /
# mdb yet, that's Phase 2). Built and pushed to GHCR by the `toolchain-image`
# job in .github/workflows/xc8-build.yml, tagged by the version pin below, so
# a tag that already exists on GHCR is reused instead of rebuilt.
#
# Versions:
#   - XC8 v4.00: license-free (no PRO gate at all, so no --LicenseType flag
#     below, unlike older XC8), newer than what the rest of this repo's
#     Makefiles/docs currently name (v3.10). Deliberately bumped ahead of
#     the repo's documented pin rather than matching it: see "which XC8
#     version to pin" in docs/ci-plan.md's open questions for the tradeoff.
#   - Microchip.PIC16Fxxx_DFP.1.7.162, Microchip.PIC18Fxxxx_DFP.1.7.171:
#     neither is bundled with the standalone XC8 v4.00 installer (verified
#     locally: `find /opt/microchip/xc8/v4.00/pic` has no `packs/`
#     directory at all). This repo's mcu/*-mplabx/README.md files assumed
#     PIC16Fxxx_DFP ships bundled, that was true of the full MPLAB X IDE's
#     pack manager, not the bare compiler installer used here.
#
# The XC8 installer is fetched from this repo's own GitHub Release, not
# Microchip's CDN: ww1.microchip.com sits behind an Akamai bot-challenge
# that returns a JavaScript challenge page (not the file) to any
# non-browser client, confirmed against both a local `docker build` and a
# real GitHub Actions run of an earlier version of this Dockerfile, and
# curl-impersonate (which spoofs Chrome's TLS fingerprint) still didn't get
# past it, it's a JS challenge, not just TLS/header fingerprinting. Trying
# to automate past a vendor's explicit anti-bot protection in CI is also
# just the wrong instinct, so this doesn't attempt to. Instead: a human
# (with a real browser, which clears the challenge trivially) downloaded
# the installer once and it's re-hosted as a release asset in this repo,
# release tag `ci-toolchain-assets`. DFPs are unaffected, fetched straight
# from packs.download.microchip.com (a different, S3/CloudFront-backed
# host, not Akamai-protected, confirmed working both locally and from a
# real GitHub Actions run).

FROM debian:12-slim

ARG XC8_VERSION=4.00
ARG PIC16FXXX_DFP_VERSION=1.7.162
ARG PIC18FXXXX_DFP_VERSION=1.7.171
ARG XC8_INSTALLER_URL=https://github.com/apojomovsky/pic8-hal/releases/download/ci-toolchain-assets/xc8-v4.00-full-install-linux-x64-installer.run

ENV DEBIAN_FRONTEND=noninteractive
ENV XC8_INSTALL_DIR=/opt/microchip/xc8/v${XC8_VERSION}

RUN apt-get update -yq \
    && apt-get install -yq --no-install-recommends \
        ca-certificates curl unzip make \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN set -eu; \
    curl -fsSL -o /tmp/installer.run "${XC8_INSTALLER_URL}"; \
    size=$(stat -c%s /tmp/installer.run); \
    if [ "$size" -lt 10000000 ]; then \
        echo "error: downloaded XC8 installer is only ${size} bytes," \
             "expected a multi-megabyte .run file: ${XC8_INSTALLER_URL}" >&2; \
        exit 1; \
    fi; \
    chmod +x /tmp/installer.run; \
    /tmp/installer.run --mode unattended --unattendedmodeui none \
        --prefix "${XC8_INSTALL_DIR}"; \
    rm /tmp/installer.run

# Neither DFP is bundled with the standalone XC8 v4.00 installer (see
# header comment). Unzipped to the paths this repo's mcu/*-mplabx/Makefile
# DFP_DIR override targets (xc8-build.yml passes DFP_DIR explicitly, these
# paths don't match the Makefiles' own v3.10-pinned defaults).
RUN set -eu; \
    for pack_ver in \
        "Microchip.PIC16Fxxx_DFP:${PIC16FXXX_DFP_VERSION}" \
        "Microchip.PIC18Fxxxx_DFP:${PIC18FXXXX_DFP_VERSION}"; \
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

ENV PATH="${XC8_INSTALL_DIR}/bin:${PATH}"
