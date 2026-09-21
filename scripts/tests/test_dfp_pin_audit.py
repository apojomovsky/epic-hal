"""Unit tests for scripts/dfp-pin-audit.py."""
import dataclasses
import importlib.util
import pathlib
import shutil
import sys
import tempfile
import unittest

SCRIPTS = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS))


def _load():
    """Import dfp-pin-audit.py, whose filename is not a module name."""
    path = SCRIPTS / "dfp-pin-audit.py"
    spec = importlib.util.spec_from_file_location("dfp_pin_audit", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


audit = _load()


class TestDockerfileParsing(unittest.TestCase):
    """The pack-to-ARG pairs come from the download loop, not a copy."""

    def test_every_pack_maps_to_a_defined_arg(self):
        """A pair naming an undefined ARG would compare against None."""
        argv = audit.dockerfile_arg_versions()
        pairs = audit.dockerfile_pack_args()
        self.assertGreaterEqual(len(pairs), 3)
        for pack, arg in pairs.items():
            self.assertIn(arg, argv, f"{pack} names {arg}, which is undefined")

    def test_the_three_families_packs_are_present(self):
        """All three packs the manifest uses must be parsed."""
        pairs = audit.dockerfile_pack_args()
        for pack in ("Microchip.PIC16Fxxx_DFP", "Microchip.PIC18Fxxxx_DFP",
                     "Microchip.PIC12-16F1xxx_DFP"):
            self.assertIn(pack, pairs)


class TestSplitPackPath(unittest.TestCase):
    def test_reads_vendor_pack_and_version_from_a_dfp_dir(self):
        """The two path-valued pins share this shape."""
        self.assertEqual(
            audit.split_pack_path(
                "/opt/microchip/mplabx/v6.35/packs/Microchip/"
                "PIC16Fxxx_DFP/1.8.167"),
            ("Microchip", "PIC16Fxxx_DFP", "1.8.167"))

    def test_a_short_path_yields_nothing_rather_than_raising(self):
        """A malformed pin is reported by the caller, not a crash.

        A two-component value is the live case: the regex accepts any
        non-space run, so the parser must not index past the list.
        """
        for value in ("", "x", "a/b", "Microchip/PIC16Fxxx_DFP"):
            with self.subTest(value=value):
                self.assertEqual(audit.split_pack_path(value), ("", "", ""))


class TestBarePack(unittest.TestCase):
    """The project files drop the vendor prefix the manifest carries."""

    def test_strips_the_vendor_prefix(self):
        self.assertEqual(audit.bare_pack("Microchip.PIC16Fxxx_DFP"),
                         "PIC16Fxxx_DFP")

    def test_leaves_an_unprefixed_name_alone(self):
        self.assertEqual(audit.bare_pack("PIC16Fxxx_DFP"), "PIC16Fxxx_DFP")


class TestProjectPins(unittest.TestCase):
    """Pins are read from all three files, in the repo's own layout."""

    def test_a_reference_project_reports_all_three_sources(self):
        """Every shipped demo carries a pin in each of the three files."""
        m = audit.manifest_lib.load(audit.manifest_lib.default_path())
        checked = 0
        for name in sorted(m.families):
            project = audit.REPO / audit.bundlegen.reference_project_dir(m, name)
            if not project.is_dir():
                continue
            checked += 1
            found = {src for src, _p, _v, _x in audit.project_pins(project)}
            self.assertEqual(found, set(audit.PIN_SOURCES),
                             f"{project.name} pins: {sorted(found)}")
        self.assertGreaterEqual(checked, 10, "too few projects inspected")

    def test_the_pins_of_a_project_agree_with_its_family(self):
        """A project pinning another family's pack is the bug this audits."""
        m = audit.manifest_lib.load(audit.manifest_lib.default_path())
        for name in sorted(m.families):
            fam = m.families[name]
            project = audit.REPO / audit.bundlegen.reference_project_dir(m, name)
            if not project.is_dir():
                continue
            packs = {p for _s, p, _v, _x in audit.project_pins(project)}
            self.assertEqual(packs, {audit.bare_pack(fam.dfp)},
                             f"{project.name} vs {fam.dfp}")


class TestManifestAgreesWithTheImage(unittest.TestCase):
    """The invariant the audit exists for: no drift between the two."""

    def test_every_family_dfp_version_matches_its_arg(self):
        m = audit.manifest_lib.load(audit.manifest_lib.default_path())
        argv = audit.dockerfile_arg_versions()
        pairs = audit.dockerfile_pack_args()
        for name, fam in sorted(m.families.items()):
            if not fam.dfp_version:
                continue
            self.assertIn(fam.dfp, pairs, f"{name} names an unknown pack")
            self.assertEqual(fam.dfp_version, argv[pairs[fam.dfp]],
                             f"{name}: manifest vs Dockerfile ARG")


class TestCheckReportsDrift(unittest.TestCase):
    """The failure branches the audit exists for, driven through check().

    Each case mutates a copy of the real tree (or a synthetic manifest) so
    a branch that stopped failing would fail this suite.
    """

    def setUp(self):
        self.m = audit.manifest_lib.load(audit.manifest_lib.default_path())
        self.argv = audit.dockerfile_arg_versions()
        self.pack_arg = audit.dockerfile_pack_args()

    def _problems(self, **kw):
        return audit.check(self.m, audit.REPO, self.argv, self.pack_arg, **kw)

    def test_the_real_tree_is_clean(self):
        """The baseline: no drift anywhere today."""
        self.assertEqual(self._problems(), [])

    def test_a_stale_manifest_version_is_reported(self):
        """A manifest version the image does not carry must fail."""
        fam = self.m.families["PIC16F87XA"]
        stale = dataclasses.replace(fam, dfp_version="1.0.0")
        m = dataclasses.replace(self.m, families={
            **self.m.families, "PIC16F87XA": stale})
        problems = audit.check(m, audit.REPO, self.argv, self.pack_arg)
        self.assertTrue(any("manifest dfp_version" in p for p in problems),
                        problems)

    def test_a_family_naming_an_unknown_pack_is_reported(self):
        """A pack with no Dockerfile ARG cannot be checked, so it fails."""
        fam = self.m.families["PIC16F87XA"]
        bogus = dataclasses.replace(fam, dfp="Microchip.NOPE_DFP")
        m = dataclasses.replace(self.m, families={
            **self.m.families, "PIC16F87XA": bogus})
        problems = audit.check(m, audit.REPO, self.argv, self.pack_arg)
        self.assertTrue(any("not one of the Dockerfile's packs" in p
                            for p in problems), problems)

    def test_a_missing_reference_project_is_reported(self):
        """A family with no project is invisible to the audit otherwise."""
        problems = audit.check(self.m, pathlib.Path("/nonexistent"),
                               self.argv, self.pack_arg)
        self.assertTrue(any("no reference project at" in p for p in problems),
                        problems)
        self.assertEqual(len(problems), len(self.m.families))

    def test_family_filters_to_one_family(self):
        """The CI shards each check one family."""
        problems = audit.check(self.m, pathlib.Path("/nonexistent"),
                               self.argv, self.pack_arg, family="PIC16F87XA")
        self.assertEqual(len(problems), 1)
        self.assertIn("PIC16F87XA", problems[0])


class TestProjectPinsAgainstARealFixture(unittest.TestCase):
    """Drift injected into a temp copy of a real project is detected."""

    def setUp(self):
        self.m = audit.manifest_lib.load(audit.manifest_lib.default_path())
        self.src = audit.REPO / audit.bundlegen.reference_project_dir(
            self.m, "PIC16F87XA")
        self.tmp = pathlib.Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.tmp, ignore_errors=True)
        # check() resolves projects as <repo>/examples/<name>, so the
        # fixture mirrors that layout with just this one family present.
        (self.tmp / "examples").mkdir()
        self.project = self.tmp / "examples" / self.src.name
        shutil.copytree(self.src, self.project)

    def test_a_project_pin_carrying_a_comment_is_ignored(self):
        """A commented-out pin must not count as a real one."""
        genesis = self.project / "nbproject/Makefile-genesis.properties"
        genesis.write_text(
            genesis.read_text()
            + "#default.Pack.dfplocation=/opt/x/PIC16Fxxx_DFP/1.7.162\n")
        pins = audit.project_pins(self.project)
        self.assertEqual({p for _s, p, _v, _x in pins}, {"PIC16Fxxx_DFP"})
        self.assertEqual({x for _s, _p, _v, x in pins}, {"1.8.167"})

    def test_a_disagreeing_version_is_reported(self):
        """The three files must agree; one stale version is drift."""
        loc = self.project / "nbproject/Makefile-local-default.mk"
        loc.write_text(loc.read_text().replace("1.8.167", "1.7.162"))
        problems = audit.check(self.m, self.tmp, audit.dockerfile_arg_versions(),
                               audit.dockerfile_pack_args(),
                               family="PIC16F87XA")
        self.assertTrue(any("pins disagree" in p for p in problems), problems)

    def test_a_wrong_pack_is_reported(self):
        """Pinning another family's pack is the epic-hal#218 bug class."""
        xml = self.project / "nbproject/configurations.xml"
        xml.write_text(xml.read_text().replace(
            'name="PIC16Fxxx_DFP"', 'name="PIC12-16F1xxx_DFP"'))
        problems = audit.check(self.m, self.tmp, audit.dockerfile_arg_versions(),
                               audit.dockerfile_pack_args(),
                               family="PIC16F87XA")
        self.assertTrue(any("pins pack" in p for p in problems), problems)

    def test_a_deleted_pin_is_reported(self):
        """The pre-fix pic16f5x shape: the genesis pin never existed."""
        genesis = self.project / "nbproject/Makefile-genesis.properties"
        genesis.write_text("# nothing here\n")
        problems = audit.check(self.m, self.tmp, audit.dockerfile_arg_versions(),
                               audit.dockerfile_pack_args(),
                               family="PIC16F87XA")
        self.assertTrue(any("carries no DFP pin in" in p for p in problems),
                        problems)

    def test_a_wrong_vendor_is_reported(self):
        """A pin naming the right pack under another vendor is drift."""
        xml = self.project / "nbproject/configurations.xml"
        xml.write_text(xml.read_text().replace('vendor="Microchip"',
                                               'vendor="Other"'))
        problems = audit.check(self.m, self.tmp, audit.dockerfile_arg_versions(),
                               audit.dockerfile_pack_args(),
                               family="PIC16F87XA")
        self.assertTrue(any("pins vendor" in p for p in problems), problems)

    def test_a_pinless_project_is_reported(self):
        """A project whose nbproject carries no pin at all must fail.

        Reachable via a project whose files are all present but empty of
        pins, which is the sibling of the missing-file case.
        """
        for name in ("Makefile-genesis.properties",
                     "Makefile-local-default.mk"):
            (self.project / "nbproject" / name).write_text("# nothing\n")
        xml = self.project / "nbproject/configurations.xml"
        xml.write_text('<?xml version="1.0"?>\n<configurationDescriptor/>\n')
        problems = audit.check(self.m, self.tmp, audit.dockerfile_arg_versions(),
                               audit.dockerfile_pack_args(),
                               family="PIC16F87XA")
        self.assertTrue(any("no DFP pin found" in p for p in problems),
                        problems)

    def test_a_malformed_pin_is_reported_not_a_crash(self):
        """A two-component DFP_DIR must not raise out of the audit."""
        loc = self.project / "nbproject/Makefile-local-default.mk"
        loc.write_text("DFP_DIR=a/b\n")
        problems = audit.check(self.m, self.tmp, audit.dockerfile_arg_versions(),
                               audit.dockerfile_pack_args(),
                               family="PIC16F87XA")
        self.assertTrue(any("carries no DFP pin" in p for p in problems),
                        problems)

    def test_a_clean_copy_reports_nothing(self):
        """The injected-drift cases above must be the only source of noise."""
        other_present = self.m.families["PIC16F87XA"]
        m = dataclasses.replace(self.m, families={"PIC16F87XA": other_present})
        problems = audit.check(m, self.tmp, audit.dockerfile_arg_versions(),
                               audit.dockerfile_pack_args())
        self.assertEqual(problems, [])


if __name__ == "__main__":
    unittest.main(verbosity=0)
