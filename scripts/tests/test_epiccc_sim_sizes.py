"""Unit tests for scripts/epiccc_sim_sizes.py."""
import contextlib
import io
import json
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

import epiccc_sim_sizes  # noqa: E402

REPORT = {
    "version": 1,
    "device": "pic18f4550",
    "core": "pic18",
    "flash_words": {"used": 9634, "total": 16384},
    "ram_bytes": {"used": 781, "total": 2048},
}


class TestSimSizesTable(unittest.TestCase):
    """The table prints one flash/RAM row per expected demo off the
    drivers' JSON reports, FAILs a demo with no readable report, and
    exits nonzero unless every FAIL is tolerated."""

    def run_main(self, build_dir, args):
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = epiccc_sim_sizes.main(
                ["--build-dir", str(build_dir)] + args)
        return code, out.getvalue()

    def write_report(self, build_dir, name="18F4550-menu-demo-sim.json",
                     payload=REPORT):
        path = pathlib.Path(build_dir) / name
        path.write_text(json.dumps(payload))
        return path

    def test_present_report_prints_flash_and_ram(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.write_report(tmp)
            code, out = self.run_main(
                tmp, ["epic-menu-demo:18F4550"])
        self.assertEqual(code, 0)
        self.assertIn("epic-menu-demo 18F4550", out)
        self.assertIn("9634 / 16384", out)
        self.assertIn("781 / 2048", out)

    def test_missing_report_fails_untolerated(self):
        with tempfile.TemporaryDirectory() as tmp:
            code, out = self.run_main(
                tmp, ["epic-menu-demo:18F4550"])
        self.assertEqual(code, 1)
        self.assertIn("FAIL", out)

    def test_missing_report_passes_when_tolerated(self):
        with tempfile.TemporaryDirectory() as tmp:
            code, out = self.run_main(
                tmp, ["--tolerate", "epic-encoder:16F877A",
                      "epic-encoder:16F877A"])
        self.assertEqual(code, 0)
        self.assertIn("FAIL", out)

    def test_malformed_report_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp) / "18F4550-menu-demo-sim.json"
            path.write_text("{ not json")
            code, out = self.run_main(
                tmp, ["epic-menu-demo:18F4550"])
        self.assertEqual(code, 1)
        self.assertIn("FAIL", out)

    def test_bad_spec_exits_2(self):
        with tempfile.TemporaryDirectory() as tmp, \
                self.assertRaises(SystemExit) as ctx:
            self.run_main(tmp, ["no-colon-here"])
        self.assertEqual(ctx.exception.code, 2)


if __name__ == "__main__":
    unittest.main()
