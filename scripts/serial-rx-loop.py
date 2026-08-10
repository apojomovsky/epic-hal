#!/usr/bin/env python3
"""serial-rx-loop.py: drive the RX-loopback harness over a real UART.

REQUIRES REAL HARDWARE: this script is the silicon leg of the
RX-loopback harness (quality roadmap task 8; the board bring-up is
task 1). It talks to the firmware in
tests/epic-combo-rx-loopback/tests/combo_rx_loopback.c running on a
real PIC through its UART pins. MPLAB SIM cannot inject RX, so no
simulator can satisfy this script: only silicon can.

Protocol (documented in the firmware's file header):
  boot:    the firmware emits "RXLOOP UP\\r\\n" once after USART init.
  line:    a byte run terminated by '\\n'. The payload is everything
           before the '\\n'; only '\\n' terminates, a lone '\\r' is a
           plain payload byte (a CR-only "terminator" does not end a
           line).
  echo:    "OK:<payload>\\r\\n" when the payload is non-empty, ends
           with '\\r' (CRLF terminator) and never exceeded 32 bytes;
           "ERR:<payload>\\r\\n" otherwise (bare LF, missing CR, or
           over-long). An over-long line echoes its first 32 buffered
           bytes; the rest are discarded until the '\\n'.
  framing: after every frame the line state resets, so lines can be
           sent back to back.

Usage:
  python3 scripts/serial-rx-loop.py /dev/ttyUSB0 [baud] [--inter-byte-ms 5]

The script opens the port, waits for the boot banner, then sends each
framing vector byte by byte (paced to keep the firmware's shallow
receive path from overrunning) and compares the echo byte-exact.
Exits 0 when every vector passes.

pyserial is required (pip install pyserial).
"""

import argparse
import sys
import time

try:
    import serial
except ImportError:
    sys.exit("error: pyserial is required (pip install pyserial); "
             "this script drives real hardware through a serial port")

BOOT_BANNER = b"RXLOOP UP\r\n"

# (input line, expected echo) pairs, byte-exact against the firmware's
# documented protocol.
VECTORS = [
    (b"hello\r\n", b"OK:hello\r\n"),
    (b"x\r\n", b"OK:x\r\n"),
    (b"\r\n", b"OK:\r\n"),                       # empty line
    (b"q\rw\r\n", b"OK:q\rw\r\n"),               # lone CR is payload
    (b"abc\n", b"ERR:abc\r\n"),                  # bad terminator
    (b"\n", b"ERR:\r\n"),                        # bare LF
    (b"a\rb\n", b"ERR:a\rb\r\n"),                # CR-only terminator
    # 40-byte line: overflow, first 32 bytes echoed
    (b"0123456789abcdef0123456789abcdef01234567\n",
     b"ERR:0123456789abcdef0123456789abcdef\r\n"),
]


def wait_for_banner(ser, timeout_s=5.0):
    """Drain bytes until the boot banner is seen (or timeout)."""
    buf = bytearray()
    ser.timeout = 0.2
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        chunk = ser.read(64)
        if chunk:
            buf.extend(chunk)
            idx = buf.find(BOOT_BANNER)
            if idx >= 0:
                del buf[:idx + len(BOOT_BANNER)]
                return True
            if len(buf) > len(BOOT_BANNER):
                del buf[:len(buf) - len(BOOT_BANNER)]
    return False


def read_exact(ser, n, timeout_s=2.0):
    """Read exactly n bytes, or as many as arrive before the timeout."""
    ser.timeout = timeout_s
    got = bytearray()
    deadline = time.monotonic() + timeout_s
    while len(got) < n and time.monotonic() < deadline:
        chunk = ser.read(n - len(got))
        if chunk:
            got.extend(chunk)
    return bytes(got)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("port", help="serial device, e.g. /dev/ttyUSB0")
    ap.add_argument("baud", type=int, nargs="?", default=9600,
                    help="baud rate (default 9600, the firmware's)")
    ap.add_argument("--inter-byte-ms", type=float, default=5.0,
                    help="delay between transmitted bytes (default 5 ms)")
    args = ap.parse_args()

    with serial.Serial(args.port, args.baud, timeout=1.0,
                       write_timeout=2.0) as ser:
        ser.reset_input_buffer()
        if not wait_for_banner(ser):
            print("FAIL: boot banner not seen (is the firmware running?)")
            return 1
        print("boot banner: OK")

        fails = 0
        for i, (vector, expected) in enumerate(VECTORS):
            ser.reset_input_buffer()
            for b in vector:
                ser.write(bytes((b,)))
                if args.inter_byte_ms > 0.0:
                    time.sleep(args.inter_byte_ms / 1000.0)
            got = read_exact(ser, len(expected))
            if got == expected:
                print("vector %d: OK" % i)
            else:
                print("vector %d: FAIL (sent %r, got %r, want %r)"
                      % (i, vector, got, expected))
                fails += 1

    if fails:
        print("%d vector(s) failed" % fails)
        return 1
    print("all vectors passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
