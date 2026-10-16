"""No tooling carrier reads a family's `.dfp` attribute directly.

The device-pack name reaches the bundle's epic-hal.mk, the scaffolded
Makefile, the generated docs and the CI pack directories. Every one of
those goes through epicmanifest.dfp_name(fam), so a manifest rename
cannot split a consumer. This guard fails on any attribute read of
`.dfp`, including getattr(fam, "dfp"), in production Python under
scripts/ and in the non-Python carriers that embed manifest reads
(workflows, Makefile, shell). epicmanifest.py (the helper's home) and
scripts/tests/ (which assert the manifest field itself) are exempt."""
import ast
import pathlib
import re
import sys
import unittest

SCRIPTS = pathlib.Path(__file__).resolve().parents[1]
ROOT = SCRIPTS.parents[1]
EXEMPT = {SCRIPTS / "build" / "epicmanifest.py"}
DFP_ATTR = re.compile(r"\.dfp(?![A-Za-z0-9_])")
DFP_GETATTR = re.compile(r"""getattr\s*\(\s*[^,]+,\s*['"]dfp['"]""")


def direct_dfp_reads(path):
    """Line numbers of `<expr>.dfp` loads and `getattr(x, "dfp")` calls."""
    tree = ast.parse(path.read_text(), filename=str(path))
    hits = []
    for node in ast.walk(tree):
        if (isinstance(node, ast.Attribute) and node.attr == "dfp"
                and isinstance(node.ctx, ast.Load)):
            hits.append(node.lineno)
        elif (isinstance(node, ast.Call) and isinstance(node.func, ast.Name)
                and node.func.id == "getattr" and len(node.args) >= 2
                and isinstance(node.args[1], ast.Constant)
                and node.args[1].value == "dfp"):
            hits.append(node.lineno)
    return hits


def text_dfp_hits(path):
    """Line numbers of `.dfp` reads in a non-Python carrier file."""
    return [lineno for lineno, line in
            enumerate(path.read_text().splitlines(), start=1)
            if DFP_ATTR.search(line) or DFP_GETATTR.search(line)]


def carrier_files():
    """Workflows, Makefile and shell files embedding manifest reads."""
    files = [ROOT / "Makefile"]
    files += sorted((ROOT / ".github" / "workflows").glob("*.yml"))
    files += sorted((SCRIPTS).rglob("*.sh"))
    return [path for path in files if path.is_file()]


class TestNoDirectDfpReads(unittest.TestCase):
    def test_production_scripts_use_dfp_name(self):
        offenders = []
        for path in sorted(SCRIPTS.rglob("*.py")):
            if path in EXEMPT or "tests" in path.relative_to(SCRIPTS).parts:
                continue
            for line in direct_dfp_reads(path):
                offenders.append(f"{path.relative_to(ROOT)}:{line}")
        for path in carrier_files():
            for line in text_dfp_hits(path):
                offenders.append(f"{path.relative_to(ROOT)}:{line}")
        self.assertEqual(offenders, [],
                         "read the pack name through epicmanifest.dfp_name(fam)")

    def test_guard_catches_a_direct_read(self):
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            probe = pathlib.Path(tmp) / "probe.py"
            probe.write_text("name = fam.dfp\nother = fam.dfp_version\n")
            self.assertEqual(direct_dfp_reads(probe), [1])

    def test_guard_catches_getattr(self):
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            probe = pathlib.Path(tmp) / "probe.py"
            probe.write_text('name = getattr(fam, "dfp")\n')
            self.assertEqual(direct_dfp_reads(probe), [1])
            shell = pathlib.Path(tmp) / "probe.sh"
            shell.write_text("print(fam.dfp)\nname = getattr(fam, 'dfp')\n"
                             "other = fam.dfp_version\n")
            self.assertEqual(text_dfp_hits(shell), [1, 2])


if __name__ == "__main__":
    sys.exit(unittest.main())
