"""compare-toolchains.sh ignores the menu/control/bridge fire-tick line, and only that."""
import pathlib
import subprocess
import tempfile
import unittest

HELPER = pathlib.Path(__file__).resolve().parents[1] / "compare-normalize.sh"
XC8_TICKS = "0005 001A 001A 001A 001A \n"
CC_TICKS = "0005 000A 000F 0014 0019 \n"


def run(snippet, *files):
    proc = subprocess.run(
        ["bash", "-c", f'. "{HELPER}"; {snippet}', "bash", *files],
        capture_output=True, text=True, check=True)
    return proc.stdout


class TestNormalize(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)

    def trace(self, name, text):
        path = pathlib.Path(self.tmp.name) / name
        path.write_text(text)
        return str(path)

    def normalized(self, module, text):
        f = self.trace("t.txt", text)
        return run('normalize_trace "$1" "$2"', module, f)

    def test_differing_fire_ticks_normalize_equal_for_the_menu_demo(self):
        head = "HB scr=0 br=7\n"
        self.assertEqual(
            self.normalized("epic-menu-demo", head + XC8_TICKS + "PASS\n"),
            self.normalized("epic-menu-demo", head + CC_TICKS + "PASS\n"))

    def test_differing_fire_ticks_normalize_equal_for_control_and_bridge(self):
        for module in ("epic-control-demo", "epic-bridge-demo"):
            with self.subTest(module=module):
                self.assertEqual(
                    self.normalized(module, XC8_TICKS + "PASS\n"),
                    self.normalized(module, CC_TICKS + "PASS\n"))

    def test_control_and_bridge_still_fail_on_any_other_line(self):
        for module in ("epic-control-demo", "epic-bridge-demo"):
            with self.subTest(module=module):
                self.assertNotEqual(
                    self.normalized(module, "HB t=53\n" + XC8_TICKS),
                    self.normalized(module, "HB t=54\n" + XC8_TICKS))

    def test_control_and_bridge_ticks_are_still_shown(self):
        for module in ("epic-control-demo", "epic-bridge-demo"):
            with self.subTest(module=module):
                a = self.trace("a.txt", "x\n" + XC8_TICKS)
                b = self.trace("b.txt", "x\n" + CC_TICKS)
                out = run(f'show_fire_ticks {module} "$1" "$2"', a, b)
                self.assertIn("xc8:     0005 001A 001A 001A 001A", out)
                self.assertIn("epic-cc: 0005 000A 000F 0014 0019", out)

    def test_any_other_line_still_differs(self):
        self.assertNotEqual(
            self.normalized("epic-menu-demo", "HB scr=0 br=7\n" + XC8_TICKS),
            self.normalized("epic-menu-demo", "HB scr=1 br=7\n" + XC8_TICKS))

    def test_other_modules_are_compared_byte_for_byte(self):
        self.assertNotEqual(
            self.normalized("epic-tick", XC8_TICKS),
            self.normalized("epic-tick", CC_TICKS))

    def test_only_the_exact_shape_is_ignored(self):
        for other in ("0005 001a 001A \n", "0005 001A 001A\n", "0005 GG1A \n"):
            self.assertNotEqual(
                self.normalized("epic-menu-demo", other),
                self.normalized("epic-menu-demo", CC_TICKS), other)

    def test_the_ticks_are_still_shown(self):
        a = self.trace("a.txt", "x\n" + XC8_TICKS)
        b = self.trace("b.txt", "x\n" + CC_TICKS)
        out = run('show_fire_ticks epic-menu-demo "$1" "$2"', a, b)
        self.assertIn("xc8:     0005 001A 001A 001A 001A", out)
        self.assertIn("epic-cc: 0005 000A 000F 0014 0019", out)

    def test_a_trace_without_the_line_is_not_hidden(self):
        with_line = "x\n" + CC_TICKS
        self.assertNotEqual(
            self.normalized("epic-menu-demo", with_line),
            self.normalized("epic-menu-demo", "x\n"))

    def test_a_missing_line_shows_as_empty(self):
        a = self.trace("a.txt", "x\n")
        out = run('show_fire_ticks epic-menu-demo "$1" "$1"', a)
        self.assertIn("xc8:", out)

    def test_nothing_is_shown_for_other_modules(self):
        a = self.trace("a.txt", XC8_TICKS)
        self.assertEqual(run('show_fire_ticks epic-tick "$1" "$1"', a), "")


if __name__ == "__main__":
    unittest.main()
