"""Unit tests for scripts/mplabx-audit.py."""
import importlib.util
import pathlib
import sys
import tempfile
import types
import unittest

SCRIPTS = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS))


def _load():
    """Import mplabx-audit.py, whose filename is not a valid module name."""
    path = SCRIPTS / "mplabx-audit.py"
    spec = importlib.util.spec_from_file_location("mplabx_audit", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


mplabx_audit = _load()


def family():
    return types.SimpleNamespace(
        name="PIC16F87XA",
        hal_sources=["hal/pic14/16f87xa/src/core/pic16_irq_table.c"],
        conditional_sources=[
            types.SimpleNamespace(
                path="hal/pic14/16f87xa/src/peripherals/pic16f87xa_psp.c",
                variants=["16F877A"],
            ),
        ],
        harness_src="common/src/core/epic_harness_target.c",
    )


CONFIG = """<?xml version="1.0" encoding="UTF-8"?>
<configurationDescriptor version="65">
  <logicalFolder name="root" displayName="root" projectFiles="true">
    <logicalFolder name="SourceFiles" displayName="Source Files" projectFiles="true">
{items}    </logicalFolder>
  </logicalFolder>
  <sourceRootList>
{roots}  </sourceRootList>
  <confs><conf><targetDevice>PIC16F877A</targetDevice></conf></confs>
</configurationDescriptor>
"""

PROJECT = """<?xml version="1.0" encoding="UTF-8"?>
<project>
  <sourceRootList>
{roots}  </sourceRootList>
</project>
"""

MAKEFILE = "SOURCEFILES={sources}\n"


class AuditCase(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        # Mirror the real tree (examples/demo.X): ../../hal then stays
        # inside the temp dir instead of escaping to /tmp/hal.
        self.proj = pathlib.Path(self.tmp.name) / "repo" / "examples" / "demo.X"
        (self.proj / "nbproject").mkdir(parents=True)

    def tearDown(self):
        self.tmp.cleanup()

    def write_project(self, items, roots, sources, includes=()):
        items_xml = "".join(f"      <itemPath>{p}</itemPath>\n" for p in items)
        roots_xml = "".join(f"    <Elem>{p}</Elem>\n" for p in roots)
        croots_xml = "".join(
            f"      <sourceRootElem>{p}</sourceRootElem>\n" for p in roots)
        (self.proj / "nbproject" / "configurations.xml").write_text(
            CONFIG.format(items=items_xml, roots=roots_xml))
        (self.proj / "nbproject" / "project.xml").write_text(
            PROJECT.format(roots=croots_xml))
        inc = "".join(f' -I"{p}"' for p in includes)
        (self.proj / "nbproject" / "Makefile-default.mk").write_text(
            MAKEFILE.format(sources=" ".join(sources)) + inc + "\n")

    def touch(self, *paths):
        for path in paths:
            target = self.proj / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.touch()

    def good_sets(self):
        items = ["../../hal/pic14/16f87xa/src/core/pic16_irq_table.c",
                 "main.c"]
        roots = ["../../hal/pic14/16f87xa/src/core"]
        return items, roots, list(items)

    def test_clean_project_passes(self):
        items, roots, sources = self.good_sets()
        self.touch(*items, "nbproject/Makefile")
        self.write_project(items, roots, sources)
        self.assertEqual(mplabx_audit.check_project(self.proj, family()), [])

    def test_missing_item_is_named(self):
        items, roots, sources = self.good_sets()
        self.touch("main.c")
        self.write_project(items, roots, sources)
        errors = mplabx_audit.check_project(self.proj, family())
        self.assertTrue(any("pic16_irq_table.c" in e for e in errors))

    def test_missing_source_root_is_named(self):
        items, roots, sources = self.good_sets()
        self.touch(*items)
        self.write_project(items, roots + ["../../hal/gone/src"], sources)
        errors = mplabx_audit.check_project(self.proj, family())
        self.assertTrue(any("gone" in e for e in errors))

    def test_config_makefile_divergence_fails(self):
        items, roots, sources = self.good_sets()
        self.touch(*items, "extra.c")
        self.write_project(items, roots, sources + ["extra.c"])
        errors = mplabx_audit.check_project(self.proj, family())
        self.assertTrue(any("disagree" in e for e in errors))

    def test_hal_file_outside_the_manifest_fails(self):
        items = ["../../hal/pic14/16f87xa/src/core/pic16_rogue.c", "main.c"]
        roots = ["../../hal/pic14/16f87xa/src/core"]
        self.touch(*items)
        self.write_project(items, roots, list(items))
        errors = mplabx_audit.check_project(self.proj, family())
        self.assertTrue(any("rogue" in e and "manifest" in e for e in errors))

    def test_conditional_for_this_part_passes(self):
        items = ["../../hal/pic14/16f87xa/src/peripherals/pic16f87xa_psp.c",
                 "main.c"]
        self.touch(*items)
        self.write_project(items, [], list(items))
        self.assertEqual(mplabx_audit.check_project(self.proj, family()), [])

    def test_conditional_for_another_part_fails(self):
        fam = family()
        fam.conditional_sources[0].variants = ["16F874A"]
        items = ["../../hal/pic14/16f87xa/src/peripherals/pic16f87xa_psp.c",
                 "main.c"]
        self.touch(*items)
        self.write_project(items, [], list(items))
        errors = mplabx_audit.check_project(self.proj, fam)
        self.assertTrue(any("psp" in e and "manifest" in e for e in errors))


    def test_lib_file_needs_existence_only(self):
        items = ["../../lib/whatever/src/thing.c", "main.c"]
        self.touch(*items)
        self.write_project(items, [], list(items))
        self.assertEqual(mplabx_audit.check_project(self.proj, family()), [])


if __name__ == "__main__":
    unittest.main()
