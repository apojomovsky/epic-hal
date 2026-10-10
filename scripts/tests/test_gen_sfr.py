"""Behaviour of scripts/gen-sfr.py --check: a family whose EDC is absent is unverified, never a pass."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile
import unittest

REPO = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = REPO / "scripts" / "gen-sfr.py"
HEADER = REPO / "hal" / "pic14" / "16f88x" / "include" / "pic16f88x_sfr.h"
DEFINE = re.compile(r"#define\s+PIC_REG_(\w+)\s+0x([0-9A-Fa-f]+)U?")


def run_check(xc8_dir, *extra):
    env = {**os.environ, "XC8_INSTALL_DIR": str(xc8_dir)}
    return subprocess.run(
        [sys.executable, str(SCRIPT), "--family", "PIC16F88X", "--check", *extra],
        capture_output=True, text=True, env=env, cwd=REPO,
    )


def write_edc(path, drift_name=None):
    """Write an EDC whose SFR addresses equal the committed header, optionally with one address off by one."""
    defs = {m.group(1): int(m.group(2), 16) for m in DEFINE.finditer(HEADER.read_text())}
    if drift_name is not None:
        defs[drift_name] += 1
    body = "".join(f'<edc:SFRDef edc:cname="{n}" edc:_addr="0x{a:X}"/>' for n, a in defs.items())
    path.write_text(f'<?xml version="1.0"?><edc:PIC xmlns:edc="http://crownking/edc">{body}</edc:PIC>')
    return defs


class TestCheckWithoutEdc(unittest.TestCase):
    def test_missing_edc_fails_check(self):
        """An absent EDC must exit nonzero, otherwise drift goes undetected on machines without XC8."""
        with tempfile.TemporaryDirectory() as d:
            r = run_check(d)
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("drift unverified", r.stderr)

    def test_sweep_without_family_fails_check(self):
        """A bare --check sweep that verifies no family must not pass either."""
        with tempfile.TemporaryDirectory() as d:
            env = {**os.environ, "XC8_INSTALL_DIR": d}
            r = subprocess.run(
                [sys.executable, str(SCRIPT), "--check"],
                capture_output=True, text=True, env=env, cwd=REPO,
            )
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("drift unverified", r.stderr)


class TestCheckWithEdc(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = pathlib.Path(self._tmp.name)
        self.edc = self.tmp / "PIC16F887.PIC"

    def tearDown(self):
        self._tmp.cleanup()

    def test_matching_edc_passes(self):
        write_edc(self.edc)
        r = run_check(self.tmp, "--edc", str(self.edc))
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)

    def test_drifted_edc_fails(self):
        defs = write_edc(self.edc, drift_name="PORTA")
        self.assertIn("PORTA", defs)
        r = run_check(self.tmp, "--edc", str(self.edc))
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("drift detected", r.stderr)


if __name__ == "__main__":
    unittest.main(verbosity=0)
