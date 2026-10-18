"""The gpsim Tier-2 gate fails on mismatches, never passes by skipping.

Covers lib/math/tools/gpsim_selftest.sh with stub gpsim binaries on PATH:
absent gpsim skips clean (exit 0), a stub emitting a wrong vector exits
nonzero, garbage output exits nonzero, a clean ALL PASS run exits zero,
and a present gpsim with no hex to load exits nonzero (fail-closed).
"""
import os
import pathlib
import shutil
import stat
import subprocess
import tempfile
import unittest

REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "lib" / "math" / "tools" / "gpsim_selftest.sh"

UTIL_NAMES = ("dirname", "find", "sort", "head", "mktemp", "rm",
              "cat", "grep", "sed", "timeout", "true")

CLEAN_LOG = (
    "epic-math target self-test\r\n"
    "PASS=90 FAIL=0\r\n"
    "RESULT: ALL PASS\r\n"
)
WRONG_VECTOR_LOG = (
    "epic-math target self-test\r\n"
    "FAIL mul_u8 got=000000F1 exp=000000F0\r\n"
    "PASS=89 FAIL=1\r\n"
    "RESULT: FAILURES PRESENT\r\n"
)


def _util_dirs():
    dirs = set()
    for name in UTIL_NAMES:
        found = shutil.which(name)
        if found is not None:
            dirs.add(str(pathlib.Path(found).parent))
    return sorted(dirs)


class TestGpsimSelftest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.bin = pathlib.Path(self.tmp.name) / "bin"
        self.bin.mkdir()
        self.hex = pathlib.Path(self.tmp.name) / "dummy.hex"
        self.hex.write_text(":020000040000FA\n:00000001FF\n")
        self.env_base = dict(os.environ)
        self.env_base["PATH"] = os.pathsep.join(
            [str(self.bin), *_util_dirs()])

    def tearDown(self):
        self.tmp.cleanup()

    def _write_gpsim(self, body):
        stub = self.bin / "gpsim"
        stub.write_text("#!/bin/sh\n" + body)
        stub.chmod(stub.stat().st_mode | stat.S_IXUSR)

    def _run(self, *args, with_gpsim=True, extra_env=None):
        env = dict(self.env_base)
        if not with_gpsim:
            env["PATH"] = os.pathsep.join(_util_dirs())
        env.pop("GPSIM_HEX", None)
        env.pop("GPSIM_CMDFILE", None)
        env.update(extra_env or {})
        self.last_path = env["PATH"]
        proc = subprocess.run(
            ["sh", str(SCRIPT), *args], env=env,
            capture_output=True, text=True, timeout=120)
        return proc

    def test_absent_gpsim_skips_clean(self):
        proc = self._run(with_gpsim=False)
        self.assertIsNone(shutil.which("gpsim", path=self.last_path))
        self.assertEqual(proc.returncode, 0)
        self.assertIn("SKIP", proc.stdout)

    def test_wrong_vector_fails(self):
        self._write_gpsim("printf '%s' '" + WRONG_VECTOR_LOG + "'\n")
        proc = self._run(str(self.hex))
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("FAIL", proc.stdout)

    def test_garbage_output_fails(self):
        self._write_gpsim("echo 'gpsim: confused'; exit 0\n")
        proc = self._run(str(self.hex))
        self.assertNotEqual(proc.returncode, 0)

    def test_clean_run_passes(self):
        self._write_gpsim("printf '%s' '" + CLEAN_LOG + "'\n")
        proc = self._run(str(self.hex))
        self.assertEqual(proc.returncode, 0)
        self.assertIn("PASS", proc.stdout)

    def test_present_gpsim_without_hex_fails(self):
        self._write_gpsim("printf '%s' '" + CLEAN_LOG + "'\n")
        missing = str(pathlib.Path(self.tmp.name) / "missing.hex")
        proc = self._run(extra_env={"GPSIM_HEX": missing})
        self.assertNotEqual(proc.returncode, 0)

    def test_present_gpsim_hex_fallback_empty_fails(self):
        for found in (REPO_ROOT / "lib" / "math").rglob("*.hex"):
            self.skipTest(f"fallback not empty: {found}")
        self._write_gpsim("printf '%s' '" + CLEAN_LOG + "'\n")
        proc = self._run()
        self.assertNotEqual(proc.returncode, 0)


if __name__ == "__main__":
    unittest.main()
