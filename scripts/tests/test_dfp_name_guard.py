"""No tooling script reads a family's `.dfp` attribute directly.

The device-pack name reaches the bundle's epic-hal.mk, the scaffolded
Makefile, the generated docs and the CI pack directories. Every one of
those goes through epicmanifest.dfp_name(fam), so a manifest rename
cannot split a consumer. This guard walks the production Python under
scripts/ and fails on any attribute read of `.dfp`. epicmanifest.py
(the helper's home) and scripts/tests/ (which assert the manifest
field itself) are exempt."""
import ast
import pathlib
import sys
import unittest

SCRIPTS = pathlib.Path(__file__).resolve().parents[1]
EXEMPT = {SCRIPTS / "build" / "epicmanifest.py"}


def direct_dfp_reads(path):
    """Line numbers of `<expr>.dfp` loads in one Python file."""
    tree = ast.parse(path.read_text(), filename=str(path))
    return [node.lineno for node in ast.walk(tree)
            if isinstance(node, ast.Attribute)
            and node.attr == "dfp"
            and isinstance(node.ctx, ast.Load)]


class TestNoDirectDfpReads(unittest.TestCase):
    def test_production_scripts_use_dfp_name(self):
        offenders = []
        for path in sorted(SCRIPTS.rglob("*.py")):
            if path in EXEMPT or "tests" in path.relative_to(SCRIPTS).parts:
                continue
            for line in direct_dfp_reads(path):
                offenders.append(f"{path.relative_to(SCRIPTS.parents[1])}:{line}")
        self.assertEqual(offenders, [],
                         "read the pack name through epicmanifest.dfp_name(fam)")

    def test_guard_catches_a_direct_read(self):
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            probe = pathlib.Path(tmp) / "probe.py"
            probe.write_text("name = fam.dfp\nother = fam.dfp_version\n")
            self.assertEqual(direct_dfp_reads(probe), [1])


if __name__ == "__main__":
    sys.exit(unittest.main())
