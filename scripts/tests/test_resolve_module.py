"""Unit tests for scripts/resolve_module.py."""
import pathlib
import subprocess
import sys
import unittest

SCRIPT = pathlib.Path(__file__).resolve().parents[1] / "resolve_module.py"


def run(*args):
    return subprocess.run([sys.executable, str(SCRIPT), *args],
                          capture_output=True, text=True)


class TestResolveModuleCli(unittest.TestCase):
    def test_dir_from_an_id(self):
        proc = run("--dir", "epic-tick")
        self.assertEqual((proc.returncode, proc.stdout.strip()), (0, "lib/tick"))

    def test_id_from_a_dir(self):
        proc = run("--id", "lib/lcd")
        self.assertEqual((proc.returncode, proc.stdout.strip()), (0, "epic-lcd"))

    def test_unknown_value_fails_naming_valid_ids(self):
        proc = run("--id", "epic-nope")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("unknown module 'epic-nope'", proc.stderr)
        self.assertIn("epic-tick", proc.stderr)


if __name__ == "__main__":
    unittest.main()
