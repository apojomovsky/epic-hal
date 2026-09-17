"""The family-bearing lists must follow the manifest, not drift.

install.sh --list and the README's docgen blocks are generated from
epic-common/manifest/modules.toml; these tests diff them against that
manifest so a family landing alone cannot leave a stale list behind
(epic-hal#219's failure mode: hand-maintained surfaces, all behind the
manifest)."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

import bundlegen  # noqa: E402
import epicmanifest  # noqa: E402
import readme_tables  # noqa: E402

REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]


def load():
    return epicmanifest.load(epicmanifest.default_path())


def slugs(manifest):
    return sorted(f.hal_dir.removesuffix("-hal") for f in manifest.families.values())


class TestInstallList(unittest.TestCase):
    """install.sh --list derives its output from the parts.txt asset."""

    def test_names_exactly_the_manifest_families(self):
        manifest = load()
        with tempfile.TemporaryDirectory() as tmp:
            asset_dir = pathlib.Path(tmp)
            (asset_dir / "parts.txt").write_text(bundlegen.emit_parts_map(manifest))
            proc = subprocess.run(
                ["sh", str(REPO_ROOT / "install.sh"), "--list", "v0.0.0-test"],
                env={**os.environ, "EPIC_HAL_BASE_URL": asset_dir.as_uri()},
                capture_output=True, text=True, check=True)
        self.assertEqual(proc.stdout.splitlines(), slugs(manifest))

    def test_family_slug_is_recognized_any_case(self):
        """A mixed-case slug classifies as a family, not a part.

        The fixture carries no bundles, so a recognized slug proceeds
        to fetch epic-hal-<slug>-<version>.tar.gz and fails there; an
        unrecognized one would die earlier with "unknown part"."""
        manifest = load()
        with tempfile.TemporaryDirectory() as tmp:
            asset_dir = pathlib.Path(tmp)
            (asset_dir / "parts.txt").write_text(bundlegen.emit_parts_map(manifest))
            proc = subprocess.run(
                ["sh", str(REPO_ROOT / "install.sh"), "PIC16F87XA", "v0.0.0-test"],
                env={**os.environ, "EPIC_HAL_BASE_URL": asset_dir.as_uri()},
                capture_output=True, text=True)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("epic-hal-pic16f87xa-v0.0.0-test.tar.gz",
                      proc.stderr)
        self.assertNotIn("unknown part", proc.stderr)

    def test_unknown_slug_reports_the_real_families(self):
        manifest = load()
        with tempfile.TemporaryDirectory() as tmp:
            asset_dir = pathlib.Path(tmp)
            (asset_dir / "parts.txt").write_text(bundlegen.emit_parts_map(manifest))
            proc = subprocess.run(
                ["sh", str(REPO_ROOT / "install.sh"), "nosuchfamily", "v0.0.0-test"],
                env={**os.environ, "EPIC_HAL_BASE_URL": asset_dir.as_uri()},
                capture_output=True, text=True)
        self.assertEqual(proc.returncode, 2)
        for slug in slugs(manifest):
            self.assertIn(slug, proc.stderr)


class TestReadmeDocgenBlocks(unittest.TestCase):
    """The README's generated tables must match the manifest."""

    def test_blocks_are_current(self):
        _, stale = readme_tables.regenerate((REPO_ROOT / "README.md").read_text())
        self.assertEqual(stale, [])

    def test_missing_markers_fail_naming_the_block(self):
        with self.assertRaisesRegex(readme_tables.DocgenError, "docgen:packs"):
            readme_tables.regenerate("no markers here")


class TestNoHandEnumerations(unittest.TestCase):
    """Prose must not carry family counts or HAL link lists that rot."""

    def test_count_phrases_are_gone(self):
        for rel in ("README.md", "DEVELOPMENT.md", "AGENTS.md",
                    ".github/workflows/ci.yml"):
            text = (REPO_ROOT / rel).read_text()
            for phrase in ("three families", "three PIC families",
                           "all 3 families"):
                self.assertNotIn(phrase, text, f"{rel} still says {phrase!r}")

    def test_readme_does_not_hand_list_family_hals(self):
        text = (REPO_ROOT / "README.md").read_text()
        text = re.sub(
            r"<!-- docgen:\w+ begin.*?<!-- docgen:\w+ end -->",
            "", text, flags=re.DOTALL)
        self.assertIsNone(
            re.search(r"\]\(pic\w+-hal/\)", text),
            "README hand-enumerates family HAL links outside a docgen block")
