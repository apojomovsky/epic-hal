"""Behaviour of scripts/gen-sfr.py when a family's EDC is absent: both modes fail, never a silent pass."""
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


def run_write(xc8_dir, *extra):
    env = {**os.environ, "XC8_INSTALL_DIR": str(xc8_dir)}
    return subprocess.run(
        [sys.executable, str(SCRIPT), "--family", "PIC16F88X", *extra],
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


class TestWriteWithoutEdc(unittest.TestCase):
    def test_missing_edc_fails_write(self):
        """An absent EDC must exit nonzero, otherwise a regen that wrote nothing looks successful."""
        with tempfile.TemporaryDirectory() as d:
            r = run_write(d)
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("not regenerated", r.stderr)

    def test_sweep_without_family_fails_write(self):
        """A bare write sweep that regenerates no family must not pass either."""
        with tempfile.TemporaryDirectory() as d:
            env = {**os.environ, "XC8_INSTALL_DIR": d}
            r = subprocess.run(
                [sys.executable, str(SCRIPT)],
                capture_output=True, text=True, env=env, cwd=REPO,
            )
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("not regenerated", r.stderr)

    def test_ungenerated_family_still_skips_write(self):
        """PIC18F1320 has no generator support yet; write mode must keep skipping it with exit 0."""
        with tempfile.TemporaryDirectory() as d:
            env = {**os.environ, "XC8_INSTALL_DIR": d}
            r = subprocess.run(
                [sys.executable, str(SCRIPT), "--family", "PIC18F1320"],
                capture_output=True, text=True, env=env, cwd=REPO,
            )
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertIn("no generator support", r.stdout)


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

    def test_matching_edc_write_passes_untouched(self):
        write_edc(self.edc)
        before = HEADER.read_bytes()
        r = run_write(self.tmp, "--edc", str(self.edc))
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual(HEADER.read_bytes(), before)


    def test_drifted_edc_fails(self):
        defs = write_edc(self.edc, drift_name="PORTA")
        self.assertIn("PORTA", defs)
        r = run_check(self.tmp, "--edc", str(self.edc))
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("drift detected", r.stderr)


if __name__ == "__main__":
    unittest.main(verbosity=0)
