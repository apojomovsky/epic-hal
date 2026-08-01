# CI toolchain image for docs/ci-plan.md Phase 1: XC8 only (no MPLAB X IDE /
# mdb yet, that's Phase 2). Built and pushed to GHCR by the `toolchain-image`
# job in .github/workflows/xc8-build.yml, tagged by the version pin below, so
# a tag that already exists on GHCR is reused instead of rebuilt.
#
# Versions pinned to match what the rest of this repo's Makefiles and docs
# already assume (docs/multi-family-plan.md, mcu/*-mplabx/Makefile), not the
# newest available, so CI exercises the same toolchain a contributor would
# actually have installed locally:
#   - XC8 v3.10 (the version named throughout this repo's Makefiles/docs;
#     still license-gated for PRO optimizations, unlike v4.00+, hence
#     --LicenseType FreeMode below)
#   - Microchip.PIC18Fxxxx_DFP.1.7.171 (the exact version
#     docs/multi-family-plan.md recorded getting a real PIC18 .hex working
#     with; PIC16Fxxx_DFP is bundled with the XC8 v3.10 install itself, see
#     pic18fxx5x-hal/mcu/pic18fxx5x-mplabx/README.md, so it is not fetched
#     separately here)
#
# NOTE: the exact Linux installer filename below could not be confirmed from
# the development sandbox this Dockerfile was written in (Microchip's
# download CDN returns "Access Denied" from that network for reasons unclear,
# possibly IP-reputation-based bot protection unrelated to GitHub Actions'
# network). This is the filename pattern used by community MPLAB X/XC8
# Dockerfiles (e.g. github.com/xanderhendriks/docker-mplabx). Treat the first
# real run of xc8-build.yml as the actual verification, per this repo's own
# convention of probing uncertain toolchain behavior instead of assuming it.

FROM debian:12-slim

ARG XC8_VERSION=3.10
ARG PIC18FXXXX_DFP_VERSION=1.7.171

ENV DEBIAN_FRONTEND=noninteractive
ENV XC8_INSTALL_DIR=/opt/microchip/xc8/v${XC8_VERSION}

RUN apt-get update -yq \
    && apt-get install -yq --no-install-recommends \
        ca-certificates curl unzip make \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# A generic browser User-Agent, some vendor download CDNs reject requests
# that don't look like a browser.
ARG CURL_UA="Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0 Safari/537.36"

RUN set -eu; \
    installer="xc8-v${XC8_VERSION}-full-install-linux-installer.run"; \
    url="https://ww1.microchip.com/downloads/en/DeviceDoc/${installer}"; \
    curl -fsSL -A "$CURL_UA" -o "/tmp/${installer}" "$url"; \
    size=$(stat -c%s "/tmp/${installer}"); \
    if [ "$size" -lt 10000000 ]; then \
        echo "error: downloaded XC8 installer is only ${size} bytes," \
             "expected a multi-megabyte .run file; the download URL or" \
             "filename pattern is probably wrong: $url" >&2; \
        exit 1; \
    fi; \
    chmod +x "/tmp/${installer}"; \
    "/tmp/${installer}" --mode unattended --unattendedmodeui none \
        --netservername localhost --LicenseType FreeMode \
        --prefix "/opt/microchip/xc8/v${XC8_VERSION}"; \
    rm "/tmp/${installer}"

# PIC18Fxxxx DFP: not bundled with XC8, unlike PIC16Fxxx DFP (see header
# comment). Unzipped to the exact path this repo's Makefiles already default
# DFP_DIR to (pic18fxx5x-hal/mcu/pic18fxx5x-mplabx/Makefile).
RUN set -eu; \
    pack="Microchip.PIC18Fxxxx_DFP.${PIC18FXXXX_DFP_VERSION}"; \
    dest="${XC8_INSTALL_DIR}/pic/packs/Microchip.PIC18Fxxxx_DFP"; \
    mkdir -p "$dest"; \
    curl -fsSL -A "$CURL_UA" -o "/tmp/${pack}.atpack" \
        "https://packs.download.microchip.com/${pack}.atpack"; \
    unzip -q "/tmp/${pack}.atpack" -d "$dest"; \
    rm "/tmp/${pack}.atpack"

ENV PATH="${XC8_INSTALL_DIR}/bin:${PATH}"
