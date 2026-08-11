"""Unit tests for scripts/ci_noncode_check.py."""
import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

import ci_noncode_check  # noqa: E402


class TestIsNonCode(unittest.TestCase):
    def check(self, files):
        return ci_noncode_check.is_non_code(files)

    def test_docs_only(self):
        self.assertTrue(self.check(["README.md", "docs/ci-plan.md", "AGENTS.md"]))

    def test_docs_dir_at_any_depth(self):
        self.assertTrue(self.check(["a/b/docs/guide.md", "docs/x.md"]))

    def test_license_and_configs(self):
        self.assertTrue(self.check([
            "LICENSE", "LICENSE.txt", ".gitignore",
            ".gitattributes", ".clang-format", ".editorconfig",
        ]))

    def test_image_assets(self):
        self.assertTrue(self.check(["docs/assets/logo.svg", "img/icon.png"]))

    def test_dev_tooling(self):
        self.assertTrue(self.check(["scripts/bootstrap.sh"]))
        self.assertTrue(self.check(["scripts/install-git-hooks.sh"]))
        self.assertTrue(self.check(["Makefile"]))

    def test_nested_makefile_is_code(self):
        self.assertFalse(self.check(["examples/epicurus-demo-pic16f87xa.X/Makefile"]))

    def test_c_source_is_code(self):
        self.assertFalse(self.check(["epic-tick/src/epic_tick.c"]))

    def test_header_is_code(self):
        self.assertFalse(self.check(["pic16f87xa-hal/include/target/epic_hal.h"]))

    def test_cmakelists_is_code(self):
        self.assertFalse(self.check(["epic-tick/CMakeLists.txt"]))

    def test_manifest_is_code(self):
        self.assertFalse(self.check(["epic-common/manifest/modules.toml"]))

    def test_ci_scripts_are_code(self):
        self.assertFalse(self.check(["scripts/epic_build.py"]))
        self.assertFalse(self.check(["scripts/sim-mdb-run.sh"]))
        self.assertFalse(self.check(["scripts/pre-commit-checks.sh"]))

    def test_workflow_is_code(self):
        self.assertFalse(self.check([".github/workflows/ci.yml"]))

    def test_dockerfile_is_code(self):
        self.assertFalse(self.check(["docker/ci-toolchain/Dockerfile"]))

    def test_unknown_extension_fails_closed(self):
        self.assertFalse(self.check(["data/foo.bin"]))

    def test_mixed_with_one_code_file(self):
        self.assertFalse(self.check(["README.md", "epic-tick/src/epic_tick.c"]))

    def test_empty_list_is_skip(self):
        self.assertTrue(self.check([]))


if __name__ == "__main__":
    unittest.main()
