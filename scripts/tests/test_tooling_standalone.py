"""Standalone and discovery runs of the tooling tests must agree.

Three scripts/tests/test_*.py files once ran fewer tests standalone
than discovery found: a missing or mid-file unittest.main guard exits
0 while silently skipping the classes defined after it (#323, #325).
The guards are fixed; this file fails the PR that reintroduces the
skew, by checking every scripts/tests/test_*.py ends with the guard
and that each file's standalone count equals its discovery count."""
import ast
import pathlib
import re
import subprocess
import sys
import unittest

REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
TESTS_DIR = REPO_ROOT / "scripts" / "tests"
SELF = pathlib.Path(__file__).name
RAN_RE = re.compile(r"^Ran (\d+) test", re.MULTILINE)


def tooling_tests():
    return sorted(TESTS_DIR.glob("test_*.py"))


def _is_main_guard(node):
    if not isinstance(node, ast.If):
        return False
    test = node.test
    if not (isinstance(test, ast.Compare) and len(test.ops) == 1
            and isinstance(test.ops[0], ast.Eq)):
        return False
    left = test.left
    (right,) = test.comparators
    if not (isinstance(left, ast.Name) and left.id == "__name__"
            and isinstance(right, ast.Constant)
            and right.value == "__main__"):
        return False
    return any(isinstance(call, ast.Call)
               and isinstance(call.func, ast.Attribute)
               and call.func.attr == "main"
               and isinstance(call.func.value, ast.Name)
               and call.func.value.id == "unittest"
               for call in ast.walk(node))


def _ran_count(output):
    match = RAN_RE.search(output)
    return int(match.group(1)) if match else None


def _counted_run(args):
    proc = subprocess.run(
        [sys.executable, *args], cwd=REPO_ROOT,
        capture_output=True, text=True, timeout=300)
    return proc.returncode, _ran_count(proc.stdout + proc.stderr)


class TestStandaloneParity(unittest.TestCase):
    def test_guard_is_the_last_statement(self):
        for path in tooling_tests():
            with self.subTest(path.name):
                tree = ast.parse(path.read_text())
                self.assertTrue(
                    tree.body and _is_main_guard(tree.body[-1]),
                    f"{path.name}: the unittest.main guard must be "
                    "the last statement, with no TestCase after it")

    def test_standalone_count_matches_discovery(self):
        for path in tooling_tests():
            # This file exempts itself: checking its own standalone
            # count would run this check, which would run this file,
            # without end. A skew here can only come from a moved
            # guard, which the test above already catches.
            if path.name == SELF:
                continue
            with self.subTest(path.name):
                code, alone = _counted_run([str(path)])
                self.assertEqual(
                    code, 0, f"{path.name}: standalone run failed")
                self.assertIsNotNone(
                    alone, f"{path.name}: no test count in output")
                code, found = _counted_run([
                    "-m", "unittest", "discover", "-s", "scripts/tests",
                    "-t", "scripts", "-p", path.name])
                self.assertEqual(
                    code, 0, f"{path.name}: discovery run failed")
                self.assertIsNotNone(
                    found, f"{path.name}: no test count in output")
                self.assertEqual(
                    alone, found,
                    f"{path.name}: standalone ran {alone}, "
                    f"discovery found {found}")


if __name__ == "__main__":
    unittest.main()
