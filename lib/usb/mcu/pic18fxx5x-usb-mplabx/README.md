# lib/usb bring-up on PIC18F4550

The silicon bring-up project for `lib/usb`'s CDC-ACM device: the
`usb-cdc` example firmware, the gate that checks it, and what each
gate actually proves.

## What is here

- `pic18fxx5x-usb.X/`: the MPLAB X project. Open it in MPLAB X to
  program and debug a board (File > Open Project, pick the `.X`
  directory). Its file list, include dirs, macros and configuration
  words are generated from the manifest by
  `scripts/build/gen_usb_mplabx.py`, and `scripts/audit/mplabx-audit.py`
  checks the committed tree against the manifest, so the IDE project
  and the CI-gated build cannot drift apart.
- `../../tests/sim_usb.c`: the simulator gate firmware.
- `../../../scripts/sim/usb-bringup-gate.sh`: the gate runner.
- `../../../scripts/sim/usb-cdc-echo-gate.py`: the host-side hardware
  gate.

## Board requirements

- PIC18F4550 (also 18F2455/2550/4455, the same family).
- 20 MHz crystal on OSC1/OSC2, USB clock from the 96 MHz PLL
  (`PLLDIV = 5`, `CPUDIV = OSC1_PLL2`, `USBDIV = 2`): the SIE needs
  exactly 48 MHz for full speed (DS39632E Section 17.2.2).
- D+ on RC5, D- on RC4, the on-chip transceiver enabled
  (`UTRDIS = 0`). A board with an external transceiver sets
  `UTRDIS = 1` instead.
- A 3.3 V supply on VUSB, or `VREGEN = ON` to enable the on-chip
  regulator from VDD. The config words in this project carry
  `VREGEN = "OFF"`, so the board must supply VUSB externally
  (DS39632E Section 17.2.2, note 1: do not enable the regulator when
  an external regulator is connected to VUSB).
- The USB RAM layout is the same on every part of the family: each has
  2 KB of data memory in eight banks, and banks 4-7 are the USB RAM
  (DS39632E Table 1-1 and Section 17.3), so one `BUFFER_ADDR` fits
  all of them.

## Build and program

The manifest-driven build is the one CI gates, and it produces the
same image the IDE project builds:

```sh
make xc8-build MODULE=lib/usb MCU=18F4550
```

The hex lands at `build/18F4550-usb-cdc.hex`. Program it with any
PIC programmer, or open `pic18fxx5x-usb.X` in MPLAB X and use
*Programmer > Program*.

## The gate

```sh
make usb-gate MODULE=lib/usb MCU=18F4550
```

Two halves, because they prove different things.

### The simulator gate (always runs)

MPLAB SIM models no USB SIE: there is no bus, no host and no token
engine, so the simulator cannot enumerate anything. What it can do
is run the real firmware and read back the SIE state, which is where
enumeration starts. The gate firmware checks:

- the SIE is enabled (`UCON.USBEN`), not suspended, and packet
  transfers are not disabled;
- full speed is selected and the internal pull-up is on
  (`UCFG.FSEN`, `UCFG.UPUEN`): the pull-up is what makes the host
  see the device at all;
- the endpoint control registers: EP0 is a control endpoint with
  handshaking, EP1 and EP2 carry data with control transfers
  disabled on them;
- the buffer descriptor table at 400h: every descriptor points into
  USB RAM 500h-7FFh (DS39632E Section 17.4.3 says a BD address
  outside 400h-7FFh loses or overwrites data), and the OUT buffers
  are armed for the right byte counts.

That last check is the one that catches the defect this project
exists to prevent. The endpoint buffers are pinned into USB RAM by
`BUFFER_ADDR` in `lib/usb/src/usb_config.h`; without it, XC8
scatters them into the ordinary GPR banks, where the SIE cannot reach
them, and the device never enumerates. The gate fails loudly when
that happens.

### The hardware gate (needs a board)

With the board plugged into the machine running the gate:

- the device enumerates as a CDC-ACM port (VID 0xA0A0, PID 0x0004,
  M-Stack's placeholder pair, see `lib/usb/README.md`);
- the host opens the port and asserts DTR, which is what the
  firmware's `epic_usb_connected()` waits for;
- 64 bytes are written and must come back byte-exact over the bulk
  endpoints.

Without a board the hardware half is skipped and says so: the
enumeration evidence is then unproven, and the gate says exactly
that instead of implying it passed.

## What the gate does not prove

- The simulator half proves the SIE is programmed and the buffers
  are in USB RAM. It does not prove the device enumerates, that the
  descriptors are accepted by a host, that the CDC class requests
  work, or that data moves.
- The hardware half proves enumeration and one echo round trip. It
  does not exercise other hosts, other operating systems, hotplug,
  suspend/resume, or cable-length or signal-integrity margins.
- Neither half proves anything about the board's own wiring beyond
  what the enumeration itself shows: a board with D+ and D- swapped
  fails to enumerate, and a board with the wrong crystal fails to
  reach 48 MHz, but neither failure is diagnosed by the gate.
