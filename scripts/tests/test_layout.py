"""The directory tree and modules.toml must describe the same modules.

The layout is fixed (common/, hal/<arch>/<family>, lib/<module>,
demos/<demo>): a directory the manifest does not know is a stray, and a
manifest entry with no directory is a stale path."""
import pathlib
import subprocess
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

import epicmanifest  # noqa: E402

REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
SHARED_CORES = {"hal/pic14/core"}


def manifest_dirs():
    m = epicmanifest.load(epicmanifest.default_path())
    return ({mod.dir for mod in m.modules.values()},
            {fam.hal_dir for fam in m.families.values()})


def tracked_dirs(pattern, depth):
    out = subprocess.run(["git", "ls-files", "--", pattern], cwd=REPO_ROOT,
                         capture_output=True, text=True, check=True).stdout
    return {"/".join(p.split("/")[:depth]) for p in out.splitlines()}


class TestLayoutMatchesManifest(unittest.TestCase):
    def test_every_manifest_directory_exists(self):
        modules, hals = manifest_dirs()
        missing = sorted(d for d in modules | hals if not (REPO_ROOT / d).is_dir())
        self.assertEqual(missing, [])

    def test_every_lib_directory_is_a_manifest_module(self):
        modules, _ = manifest_dirs()
        self.assertEqual(sorted(tracked_dirs("lib/*/*", 2) - modules), [])

    def test_every_demo_directory_is_a_manifest_module(self):
        modules, _ = manifest_dirs()
        self.assertEqual(sorted(tracked_dirs("demos/*/*", 2) - modules), [])

    def test_every_hal_directory_is_a_family_or_shared_core(self):
        _, hals = manifest_dirs()
        self.assertEqual(
            sorted(tracked_dirs("hal/*/*/*", 3) - hals - SHARED_CORES), [])

    def test_every_cmake_module_is_in_the_manifest(self):
        modules, hals = manifest_dirs()
        cmake = {p.rsplit("/CMakeLists.txt", 1)[0]
                 for p in subprocess.run(
                     ["git", "ls-files", "--", "*/CMakeLists.txt"],
                     cwd=REPO_ROOT, capture_output=True, text=True,
                     check=True).stdout.splitlines()}
        self.assertEqual(sorted(cmake - modules - hals), [])


if __name__ == "__main__":
    unittest.main()
