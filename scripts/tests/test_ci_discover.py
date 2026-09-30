"""Unit tests for scripts/ci-discover-affected-modules.py, run against the
real tree: the change-detection reads CMakeLists.txt files and directory
names, so a layout move that it misreads shrinks the CI matrix silently."""
import importlib.util
import os
import pathlib
import sys
import unittest

SCRIPTS = pathlib.Path(__file__).resolve().parents[1]
REPO_ROOT = SCRIPTS.parent
sys.path.insert(0, str(SCRIPTS))


def load():
    spec = importlib.util.spec_from_file_location(
        "ci_discover", SCRIPTS / "ci-discover-affected-modules.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


class TestDiscover(unittest.TestCase):
    def setUp(self):
        self.old_cwd = os.getcwd()
        os.chdir(REPO_ROOT)
        self.addCleanup(os.chdir, self.old_cwd)
        self.d = load()
        self.modules = self.d.discover_modules()
        self.graph = self.d.build_dep_graph(self.modules)

    def test_declared_sibling_deps_are_found(self):
        self.assertEqual(
            self.graph["lib/modbus"], {"lib/serial", "lib/tick"})

    def test_a_dependent_is_affected_by_its_dependency(self):
        affected = self.d.transitive_closure({"lib/modbus"}, self.graph)
        self.assertTrue({"lib/serial", "lib/tick"} <= affected)

    def test_common_change_reaches_every_family_hal(self):
        hals = self.d.hal_modules(self.modules)
        self.assertTrue(hals)
        self.assertTrue(all(m.startswith("hal/") for m in hals))
        self.assertIn("hal/pic18/18fxx5x", hals)
        self.assertNotIn("lib/tick", hals)


if __name__ == "__main__":
    unittest.main()
