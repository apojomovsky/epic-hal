#!/usr/bin/env python3
"""Silicon bring-up gate for lib/usb's CDC-ACM firmware on a real board.

REQUIRES REAL HARDWARE. Finds the device M-Stack's placeholder VID/PID
identifies (0xA0A0:0x0004 unless --vid/--pid say otherwise), waits for the
host to enumerate it as a CDC-ACM port, asserts DTR (the firmware's
connected() gate), writes a known byte string and requires it echoed back
byte-exact over the bulk endpoints. That is the enumeration-plus-echo
evidence no simulator can supply here: MPLAB SIM models no USB SIE, so the
mdb gate in tests/sim_usb.c stops at the SIE register and buffer-descriptor
image.

The board must already be running the usb-cdc example (flash it with
`make xc8-build MODULE=lib/usb MCU=18F4550` and any PIC programmer, or the
MPLAB X project under lib/usb/mcu/pic18fxx5x-usb-mplabx/). This script
never programs anything; it only observes the running firmware.

Needs pyserial (pip install pyserial). Run it by hand, with the board on a
USB port of the machine it runs on; it is not part of any headless CI job.
Exit codes: 0 the echo round trip passed, 1 the device was found but a
check failed, 2 no matching device on the bus.
"""

from __future__ import annotations

import argparse
import sys
import time

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    # No pyserial means this host cannot drive the board. That is a
    # skip, not a failure: exit 2, the same code the gate uses for "no
    # board on this machine".
    print("usb gate: pyserial is not installed (pip install pyserial); "
          "the hardware gate cannot run on this host", file=sys.stderr)
    sys.exit(2)

DEFAULT_VID = 0xA0A0
DEFAULT_PID = 0x0004
DEFAULT_TIMEOUT = 30.0


class DtrError(Exception):
    """The port could not assert DTR, so the device will not echo."""

# Deterministic, non-repeating payload: the echo comparison is byte-exact,
# so a mangled ring index or a dropped packet fails here.
PAYLOAD = bytes(range(0x20, 0x20 + 64))


def find_port(vid: int, pid: int, deadline: float) -> serial.Serial | None:
    """Open the CDC port of the given VID/PID, waiting for enumeration.

    pyserial reports the USB VID/PID of each port, so the device is
    identified from the port list rather than assumed to be the only
    ttyACM node on the machine.
    """
    while time.monotonic() < deadline:
        for info in list_ports.comports():
            if info.vid == vid and info.pid == pid:
                return serial.Serial(info.device, 115200, timeout=1.0)
        time.sleep(0.25)
    return None


def echo_round_trip(port, payload: bytes, timeout: float) -> bytes:
    """Write payload and collect the bytes the device sends back.

    DTR is asserted first: the firmware's connected() waits for it, so
    without it the device never echoes. A port that cannot carry
    modem-control lines raises here, and that is a failure in its own
    right: reporting it as a byte mismatch would point at the echo
    instead of at the port.

    The read side has its own deadline: the time spent waiting for
    enumeration does not come out of the time allowed for the echo to
    come back.
    """
    try:
        port.dtr = True
    except (OSError, IOError) as exc:
        raise DtrError("cannot assert DTR on %s (%s); the firmware only "
                       "echoes once the host opens the port"
                       % (port.name, exc)) from exc
    time.sleep(0.5)
    port.reset_input_buffer()
    port.write(payload)
    port.flush()

    deadline = time.monotonic() + timeout
    got = bytearray()
    while len(got) < len(payload) and time.monotonic() < deadline:
        chunk = port.read(len(payload) - len(got))
        if chunk:
            got.extend(chunk)
    return bytes(got)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--vid", type=lambda s: int(s, 0), default=DEFAULT_VID,
                        help="USB vendor id (default 0x%04X, M-Stack's "
                             "placeholder)" % DEFAULT_VID)
    parser.add_argument("--pid", type=lambda s: int(s, 0), default=DEFAULT_PID,
                        help="USB product id (default 0x%04X)" % DEFAULT_PID)
    parser.add_argument("--timeout", type=float, default=DEFAULT_TIMEOUT,
                        help="seconds to wait for enumeration (default %d)"
                             % DEFAULT_TIMEOUT)
    args = parser.parse_args()

    deadline = time.monotonic() + args.timeout
    port = find_port(args.vid, args.pid, deadline)
    if port is None:
        print("usb gate: no %04X:%04X CDC port appeared within %.0f s; "
              "no board, or it is not running the usb-cdc example"
              % (args.vid, args.pid, args.timeout), file=sys.stderr)
        return 2

    try:
        try:
            got = echo_round_trip(port, PAYLOAD, args.timeout)
        except DtrError as exc:
            print("usb gate: FAIL, %s" % exc, file=sys.stderr)
            return 1
        print("usb gate: enumerated %04X:%04X on %s"
              % (args.vid, args.pid, port.port))
        if got != PAYLOAD:
            print("usb gate: FAIL, echo mismatch: sent %d bytes, got %d"
                  % (len(PAYLOAD), len(got)), file=sys.stderr)
            if got:
                print("  first difference at byte %d"
                      % next(i for i in range(min(len(got), len(PAYLOAD)))
                             if got[i] != PAYLOAD[i]), file=sys.stderr)
            return 1
        print("usb gate: PASS, %d bytes echoed byte-exact" % len(PAYLOAD))
        return 0
    finally:
        port.close()


if __name__ == "__main__":
    sys.exit(main())
