"""The generated reference pages must match their sources, not drift.

docs/reference/modules.md, docs/reference/families.md and README.md
render from common/manifest/modules.toml and docs/README.src.md; these tests
regenerate in memory and diff against the checked-in pages, so a stale
page fails here as well as in the CI docs job. The include renderer
gets its own unit tests: nesting, cycles, and missing files.
"""
import dataclasses
import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "build"))

import docs_gen  # noqa: E402
import epicmanifest  # noqa: E402

REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]


class TestGeneratedPagesFresh(unittest.TestCase):
    def test_checked_in_pages_match_generate(self):
        outputs = docs_gen.generate(REPO_ROOT)
        for path, text in outputs.items():
            with self.subTest(page=path):
                self.assertEqual((REPO_ROOT / path).read_text(), text)


class TestUserPages(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = epicmanifest.load(epicmanifest.default_path())
        cls.outputs = docs_gen.generate(REPO_ROOT)

    def test_one_page_per_library_module_and_no_others(self):
        pages = {p for p in self.outputs if p.startswith("docs/user/modules/")}
        want = {f"docs/user/modules/{n.split('/', 1)[1]}.md"
                for n in self.manifest.modules if n.startswith("lib/")}
        self.assertEqual(pages, want)

    def test_index_links_every_page(self):
        index = self.outputs["docs/user/index.md"]
        for path in self.outputs:
            if path.startswith("docs/user/modules/"):
                self.assertIn(f"(modules/{path.rsplit('/', 1)[1]})", index)

    def test_pages_have_no_em_dash_or_version_prose(self):
        for path, text in self.outputs.items():
            if path.startswith("docs/user/"):
                with self.subTest(page=path):
                    self.assertNotIn("\u2014", text)

    def test_dependency_is_linked(self):
        page = self.outputs["docs/user/modules/console.md"]
        self.assertIn("[`lib/serial`](serial.md)", page)

    def test_orphan_pages_are_found(self):
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            (root / "docs/user/modules").mkdir(parents=True)
            (root / "docs/user/modules/gone.md").write_text("x")
            self.assertEqual(docs_gen.orphan_pages(root, {}),
                             ["docs/user/modules/gone.md"])


class TestModuleId(unittest.TestCase):
    def test_table_key_when_no_id_field(self):
        mod = epicmanifest.load(epicmanifest.default_path()).modules[
            "lib/serial"]
        self.assertEqual(docs_gen.module_id(mod), "lib/serial")

    def test_id_field_wins_when_present(self):
        @dataclasses.dataclass
        class Fake:
            name: str = "table-key"
            id: str = "from-path"
        self.assertEqual(docs_gen.module_id(Fake()), "from-path")


class TestCoreOf(unittest.TestCase):
    def test_known_arches(self):
        self.assertEqual(docs_gen.core_of("hal/pic12/16f5x"), "Baseline")
        self.assertEqual(
            docs_gen.core_of("hal/pic14/16f87xa"), "Mid-range (classic)")
        self.assertEqual(
            docs_gen.core_of("hal/pic14e/16f193x"), "Enhanced mid-range")
        self.assertEqual(docs_gen.core_of("hal/pic18/18fxx5x"), "PIC18")

    def test_unknown_arch_fails(self):
        with self.assertRaises(docs_gen.DocsGenError):
            docs_gen.core_of("hal/pic24/whatever")


class TestIncludes(unittest.TestCase):
    def test_include_expands_and_nests(self):
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            (root / "inner.md").write_text("inner line\n")
            (root / "outer.md").write_text(
                "before\n--8<-- \"inner.md\"\nafter\n")
            self.assertEqual(
                docs_gen.render_includes(
                    "top\n--8<-- \"outer.md\"\n", root),
                "top\nbefore\ninner line\nafter\n")

    def test_missing_include_fails(self):
        with self.assertRaises(docs_gen.DocsGenError):
            docs_gen.render_includes('--8<-- "docs/no-such-file.md"\n',
                                     REPO_ROOT)

    def test_include_cycle_fails(self):
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            (root / "a.md").write_text('--8<-- "b.md"\n')
            (root / "b.md").write_text('--8<-- "a.md"\n')
            with self.assertRaises(docs_gen.DocsGenError):
                docs_gen.render_includes('--8<-- "a.md"\n', root)

    def test_plain_lines_pass_through(self):
        text = "# Title\n\nSome prose.\n"
        self.assertEqual(
            docs_gen.render_includes(text, REPO_ROOT), text)


class TestReadmeSnippets(unittest.TestCase):
    def test_install_snippet_matches_installer_usage(self):
        snippet = (REPO_ROOT / "docs/snippets/install.sh").read_text()
        header = (REPO_ROOT / "install.sh").read_text()
        url = snippet.split("| sh")[0].split()[-1]
        self.assertIn(url, header)

    def test_blink_example_is_the_reference_project(self):
        src = (REPO_ROOT / "docs/README.src.md").read_text()
        ref = "examples/epic-hal-demo-pic16f87xa.X/main.c"
        self.assertIn(f'--8<-- "{ref}"', src)
        self.assertTrue((REPO_ROOT / ref).is_file())

    def test_readme_length(self):
        n = len((REPO_ROOT / "README.md").read_text().splitlines())
        self.assertTrue(80 <= n <= 120, n)


if __name__ == "__main__":
    unittest.main()
