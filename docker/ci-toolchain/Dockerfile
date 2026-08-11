# CI toolchain image for this repo's real-target builds and the MPLAB SIM gate:
# XC8 plus MPLAB X IDE (for mdb.sh only). Version pins are the ARGs below;
# the tag formula lives in the root Makefile and the workflows, not here.
#
# Installers are never fetched here: ww1.microchip.com blocks scripted
# downloads with an Akamai JS bot-challenge, so CI extracts them from a
# private GHCR assets image into vendor/ and this Dockerfile just COPYs them in.
#
# cmake/build-essential: for local `make test` only (host-tests.yml installs
# its own). MPLAB X: 8-bit-only, pruned in the same RUN as the install.

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
        libusb-1.0-0 libudev1 \
        cmake build-essential \
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
    rm /tmp/installer.run; \
    cd "${XC8_INSTALL_DIR}" && rm -rf \
        avr docs \
        "Uninstall MPLAB XC8 C Compiler.desktop" \
        Uninstall-xc8-v4.00 Uninstall-xc8-v4.dat

# None of the three DFPs is bundled with the standalone XC8 v4.00
# installer. Unzipped to the paths this repo's
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
    ide="ide"; \
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
    rm "$installer"; \
    cd "${MPLABX_INSTALL_DIR}" && rm -rf \
        docs \
        Uninstall_MPLAB_X_IDE_v6.35 \
        Uninstall_MPLAB_X_IDE_v6.35.desktop \
        Uninstall_MPLAB_X_IDE_v6.dat; \
    # The MPLAB X installer's own running-instance check is a naive
    # `ps | grep mplab_ide`, which would match this RUN's own command
    # line if "mplab_ide" appeared literally here, so the name is
    # spliced through a variable.
    ide_dir="mplab_${ide}"; \
    cd "${MPLABX_INSTALL_DIR}/mplab_platform" && rm -rf \
        mcc mcc-content-manager mcc-services mcc-update-manager \
        browser-lib ide "${ide_dir}" alexandriaoffline data-visualizer \
        mplab-pymdfu dlight dv-ide-support mplab-content-manager \
        mplab-packmanagerui cnd harness thirdparty dat; \
    cd "${MPLABX_INSTALL_DIR}/packs/Microchip" && rm -rf \
        ICD5_TP ICE4_TP PICkit5_TP PICkit4_TP Snap_TP PICkitBasic_TP \
        PKOB4_TP ICD4_TP EDBG_TP mEDBG_TP AtmelICE_TP JTAGICE3_TP \
        PowerDebugger_TP nEDBG_TP \
        AVR-Dx_DFP ATmega_DFP ATtiny_DFP XMEGAA_DFP XMEGAB_DFP XMEGAC_DFP \
        XMEGAD_DFP XMEGAE_DFP AVR-Ex_DFP AVR-Lx_DFP AVR-Sx_DFP \
        ATautomotive_DFP \
        PIC18F-Q_DFP PIC18F-K_DFP PIC18F-J_DFP PIC10-12Fxxx_DFP \
        PIC12-16Cxxx_DFP PIC18Cxxx_DFP MCPxxxx_DFP PIC-8bitAC_DFP \
        MCVxxxx_DFP PIC16F1xxxx_DFP

ENV PATH="${XC8_INSTALL_DIR}/bin:${MPLABX_INSTALL_DIR}/mplab_platform/bin:${PATH}"
