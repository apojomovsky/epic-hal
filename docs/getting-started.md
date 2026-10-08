# Getting started beyond the one-liner

The [README](../README.md) covers the fastest path: the installer script with
the epic-cc toolchain. This page covers every other way in: manual bundle
setup, MPLAB X, wiring by hand, and the XC8 device packs.

## 1. Download a bundle and run `epic-hal init`

Bundles live on the [Releases](https://github.com/apojomovsky/epic-hal/releases)
page; the README's bundle table lists which parts each one holds. The
`<version>` in a bundle name is the release tag (e.g. `v0.6.0`); the badge on
the README always shows the latest one.

Download and unpack one, then, with the CLI installed globally:

```sh
pipx install git+https://github.com/apojomovsky/epic-hal
epic-hal init --bundle /path/to/unpacked/bundle
```

Answer family, part, and modules. It writes `main.c`, a filled `Makefile`,
and a ready MPLAB X `.X` in your current directory for your exact part and
module subset. Open `myapp.X` in MPLAB X (or the MPLAB extension for VS
Code) and Build, or `make`.

## 2. Open the reference project in MPLAB X

Unpack the bundle, then open `examples/epic-hal-demo.X` (File > Open
Project). Pick your exact part under Project Properties, and Build. It
produces a `.hex` you can program with MPLAB IPE or any PICkit.

<details>
<summary>New to MPLAB X?</summary>

You need MPLAB X IDE and the MPLAB XC8 compiler, both free from
Microchip (the free XC8 tier is enough). The reference project is
pre-wired: sources, include paths, and configuration words are already
set. Selecting your part under Project Properties is the only manual
step.
</details>

Adding Epic HAL to an existing MPLAB X project instead? The bundle's
`MPLABX.md` walks through it.

## 3. Skip the IDE: a six-line Makefile

Just `epic-cc` and `make`, with no Microchip download on that path:

```make
EPIC_HAL_DIR := third_party/epic-hal
EPIC_HAL_MCU := 16F877A
EPIC_HAL_MODULES := serial tick
include $(EPIC_HAL_DIR)/epic-hal.mk

SRCS := main.c $(EPIC_HAL_SRCS)
CFLAGS += $(EPIC_HAL_CFLAGS)

app.hex: $(SRCS)
	epic-cc --device p16f877a $(CFLAGS) $^ -o $@
```

XC8 alternate: `xc8-cc $(CFLAGS) $^ -o $@ -ginhx32` with `TOOLCHAIN=xc8`
(plus its device pack, next section).

Run `make`, program the result. Dependencies resolve automatically
(`modbus` pulls in `serial` and `tick`), and asking for a module on a
part it does not fit fails immediately with the reason instead of a
wall of XC8 linker errors. Each bundle's `SUPPORT.md` has the full
per-part table.

## XC8 device packs

XC8 needs the device pack for your family; it ships with MPLAB X or
downloads separately from Microchip's pack CDN. The pack for every family
is in the README's pack table. The exact version a bundle's
`make TOOLCHAIN=xc8` build consumes is pinned in the bundle's
`epic-hal.mk` as `EPIC_HAL_DFP_VERSION`. Install the pack next to XC8:

```sh
mkdir -p /opt/microchip/xc8/v4.00/pic/packs
unzip ~/Downloads/Microchip.PIC16Fxxx_DFP.1.7.162.atpack \
  -d /opt/microchip/xc8/v4.00/pic/packs/Microchip.PIC16Fxxx_DFP
```

Or use MPLAB X's Tools > Packs manager, which does this for you.
