"""Unit tests for scripts/sim/usb-cdc-echo-gate.py.

The gate drives real hardware, so what is tested here is its
decision logic: the echo round trip must return exactly the bytes
written, and a wrong or truncated answer must be distinguishable from
a correct one. The transport is a pseudo-terminal standing in for the
device, so the test exercises the gate's own read/write loop without
claiming anything about USB.
"""
import importlib.util
import os
import pathlib
import sys
import threading
import time
import unittest

SCRIPTS = pathlib.Path(__file__).resolve().parents[1]


def _load():
    """Import the gate script, which needs pyserial."""
    path = SCRIPTS / "sim" / "usb-cdc-echo-gate.py"
    spec = importlib.util.spec_from_file_location("usb_cdc_echo_gate", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


try:
    import serial  # noqa: F401
except ImportError:
    serial = None

if serial is None:
    gate = None
else:
    gate = _load()


@unittest.skipIf(serial is None, "pyserial is not installed")
class EchoRoundTripCase(unittest.TestCase):
    """The round trip over a pseudo-terminal."""

    def setUp(self):
        import pty
        self.master, slave = pty.openpty()
        self.port = serial.Serial(
            os.ttyname(slave), 115200, timeout=1.0)
        self.slave_name = os.ttyname(slave)

    def tearDown(self):
        self.port.close()
        os.close(self.master)

    def _echo_server(self, transform=None):
        """Echo what arrives back, optionally transformed."""
        def run():
            while True:
                try:
                    data = os.read(self.master, 1024)
                except OSError:
                    return
                if not data:
                    return
                os.write(self.master, transform(data) if transform else data)
        thread = threading.Thread(target=run, daemon=True)
        thread.start()
        return thread

    def test_exact_echo_comes_back(self):
        self._echo_server()
        payload = bytes(range(0x20, 0x80))
        got = gate.echo_round_trip(self.port, payload, 5.0)
        self.assertEqual(got, payload)

    def test_a_mangled_echo_is_distinguishable(self):
        """A device that returns the wrong bytes must not pass."""
        self._echo_server(transform=lambda d: bytes(b ^ 0xFF for b in d))
        payload = bytes(range(0x20, 0x80))
        got = gate.echo_round_trip(self.port, payload, 5.0)
        self.assertNotEqual(got, payload)

    def test_a_truncated_echo_is_distinguishable(self):
        """A device that drops bytes must not pass."""
        self._echo_server(transform=lambda d: d[:1])
        payload = bytes(range(0x20, 0x80))
        got = gate.echo_round_trip(self.port, payload, 1.0)
        self.assertNotEqual(got, payload)

    def test_a_silent_device_times_out_instead_of_hanging(self):
        """No device on the other end: the read gives up."""
        payload = bytes(range(0x20, 0x80))
        start = time.monotonic()
        got = gate.echo_round_trip(self.port, payload, 1.0)
        self.assertLess(time.monotonic() - start, 10.0)
        self.assertNotEqual(got, payload)

    def test_dtr_is_asserted(self):
        """The firmware's connected() waits for DTR, so it must be set."""
        self._echo_server()
        try:
            gate.echo_round_trip(self.port, b"x", 2.0)
        except OSError:
            # This port cannot carry modem-control lines at all, so
            # there is no DTR to assert.
            self.skipTest("this port does not support DTR")
        self.assertTrue(self.port.dtr)


if __name__ == "__main__":
    unittest.main()
