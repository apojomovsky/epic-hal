"""Unit tests for scripts/release_notes.py."""
import os
import pathlib
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

import release_notes  # noqa: E402

REPO = "https://github.com/apojomovsky/epic-hal"


class TestClassify(unittest.TestCase):
    """Parsing a Conventional Commit subject into release-notes parts."""

    def test_type_scope_and_pr(self):
        """A squash-merged subject yields heading, scope, text, and PR."""
        e = release_notes.classify("feat(init): scaffold Makefile builds (#50)")
        self.assertEqual(e["heading"], "Added")
        self.assertEqual(e["scope"], "init")
        self.assertEqual(e["text"], "scaffold Makefile builds")
        self.assertEqual(e["pr"], "50")
        self.assertFalse(e["breaking"])

    def test_scope_is_optional(self):
        """A subject without a scope still classifies."""
        e = release_notes.classify("fix: correct the divisor")
        self.assertEqual(e["heading"], "Fixed")
        self.assertIsNone(e["scope"])
        self.assertEqual(e["text"], "correct the divisor")
        self.assertIsNone(e["pr"])

    def test_bang_marks_breaking(self):
        """The Conventional Commits '!' marks a breaking change."""
        e = release_notes.classify("refactor(bundle)!: rename epicurus.mk (#52)")
        self.assertTrue(e["breaking"])
        self.assertEqual(e["scope"], "bundle")

    def test_refactor_and_perf_are_changed(self):
        """refactor and perf both land under Changed."""
        self.assertEqual(release_notes.classify("refactor(x): a")["heading"], "Changed")
        self.assertEqual(release_notes.classify("perf(x): a")["heading"], "Changed")

    def test_internal_types_have_no_heading(self):
        """ci/plan/style/test/chore are internal, not user-facing sections."""
        for t in ("ci", "plan", "style", "test", "chore"):
            self.assertIsNone(release_notes.classify(f"{t}(x): a")["heading"], t)

    def test_breaking_change_footer_in_body(self):
        """A BREAKING CHANGE: footer marks a break even without '!'."""
        e = release_notes.classify(
            "refactor(rebrand): rename Epicurus to Epic HAL (#52)",
            "Some prose.\n\nBREAKING CHANGE: EPICURUS_DIR is now EPIC_HAL_DIR.\n")
        self.assertTrue(e["breaking"])
        self.assertEqual(e["heading"], "Changed")

    def test_breaking_change_footer_hyphen_spelling(self):
        """The BREAKING-CHANGE spelling is accepted too."""
        e = release_notes.classify("fix(a): x", "BREAKING-CHANGE: gone.\n")
        self.assertTrue(e["breaking"])

    def test_body_without_footer_is_not_breaking(self):
        """Prose merely mentioning a break does not mark one."""
        e = release_notes.classify(
            "fix(a): x", "This is not a breaking change at all.\n")
        self.assertFalse(e["breaking"])

    def test_nonconforming_subject_is_kept(self):
        """A subject that is not a Conventional Commit is never dropped."""
        e = release_notes.classify("merge upstream and fix things")
        self.assertIsNone(e["heading"])
        self.assertEqual(e["text"], "merge upstream and fix things")

    def test_pr_suffix_only_stripped_from_the_end(self):
        """A '#n' inside the subject is not mistaken for the squash suffix."""
        e = release_notes.classify("fix(ci): handle issue #7 in the gate (#48)")
        self.assertEqual(e["text"], "handle issue #7 in the gate")
        self.assertEqual(e["pr"], "48")


class TestRender(unittest.TestCase):
    """Rendering a set of subjects into the What-changed section."""

    def test_groups_by_section_in_order(self):
        """Sections appear in Added, Fixed, Changed, Documentation order."""
        out = release_notes.render(
            ["docs(readme): retitle", "fix(install): report prereqs (#49)",
             "feat(init): clean default build (#51)"],
            "v0.3.4", "v0.3.7", REPO)
        self.assertLess(out.index("### Added"), out.index("### Fixed"))
        self.assertLess(out.index("### Fixed"), out.index("### Documentation"))

    def test_bullet_has_bold_scope_and_pr_link(self):
        """A bullet bolds the scope and links the PR number."""
        out = release_notes.render(
            ["feat(init): clean default build (#51)"], "v0.3.6", "v0.3.7", REPO)
        self.assertIn(
            f"- **init**: clean default build ([#51]({REPO}/pull/51))", out)

    def test_pr_unlinked_without_repo_url(self):
        """Without a repo URL the PR number stays as plain text."""
        out = release_notes.render(
            ["feat(init): clean default build (#51)"], "v0.3.6", "v0.3.7")
        self.assertIn("- **init**: clean default build (#51)", out)

    def test_breaking_section_comes_first_and_excludes_from_others(self):
        """A breaking change is listed once, under Breaking."""
        out = release_notes.render(
            ["feat(a)!: drop the old var", "fix(b): a fix"],
            "v0.3.6", "v0.4.0", REPO)
        self.assertLess(out.index("### Breaking"), out.index("### Fixed"))
        self.assertEqual(out.count("drop the old var"), 1)
        self.assertNotIn("### Added", out)

    def test_internal_changes_are_collapsed_not_dropped(self):
        """Internal commits survive inside a collapsed details block."""
        out = release_notes.render(
            ["ci(gate): retune the matrix (#40)"], "v0.3.6", "v0.3.7", REPO)
        self.assertIn("<details><summary>Internal changes</summary>", out)
        self.assertIn("retune the matrix", out)
        self.assertIn("</details>", out)

    def test_empty_range_states_no_source_changes(self):
        """A re-release from an unchanged tree says so instead of rendering blank."""
        out = release_notes.render([], "v0.3.2", "v0.3.3", REPO)
        self.assertIn("No source changes since v0.3.2", out)
        self.assertNotIn("### Added", out)

    def test_compare_link_present_and_omitted_on_first_release(self):
        """The compare link needs a predecessor; a first release has none."""
        out = release_notes.render(["feat(a): x"], "v0.3.6", "v0.3.7", REPO)
        self.assertIn(f"Full changelog: {REPO}/compare/v0.3.6...v0.3.7", out)

        first = release_notes.render(["feat(a): x"], None, "v0.1.0", REPO)
        self.assertNotIn("Full changelog", first)

    def test_section_omitted_when_it_has_no_commits(self):
        """Headings never render empty."""
        out = release_notes.render(["fix(a): x"], "v0.3.6", "v0.3.7", REPO)
        self.assertNotIn("### Added", out)
        self.assertNotIn("### Documentation", out)

    def test_accepts_subject_body_pairs(self):
        """render takes (subject, body) pairs as commits() returns them."""
        out = release_notes.render(
            [("refactor(x): rename things (#52)",
              "BREAKING CHANGE: the old variable is gone.\n")],
            "v0.3.7", "v0.4.0", REPO)
        self.assertIn("### Breaking", out)
        self.assertIn("rename things", out)
        self.assertNotIn("### Changed", out)

    def test_output_ends_with_newline(self):
        """The section is safe to concatenate with the static block."""
        out = release_notes.render(["fix(a): x"], "v0.3.6", "v0.3.7", REPO)
        self.assertTrue(out.endswith("\n"))


class TestAgainstRealGit(unittest.TestCase):
    """commits() and previous_tag() driven against an actual repository.

    These cover what the pure-parsing tests cannot: that the git invocation
    itself is well-formed. A NUL field separator parsed fine in-process but
    could not be passed to git as an argument.
    """

    def setUp(self):
        """Build a throwaway repo with two tags and a breaking-change body."""
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.repo = pathlib.Path(self.tmp.name)
        self.cwd = os.getcwd()
        self.addCleanup(os.chdir, self.cwd)
        os.chdir(self.repo)

        self._run("git", "init", "-q", "-b", "main")
        self._run("git", "config", "user.email", "t@example.com")
        self._run("git", "config", "user.name", "T")
        self._commit("feat(a): first thing", tag="v0.1.0")
        self._commit("fix(b): second thing (#7)")
        self._commit(
            "refactor(c): third thing (#8)",
            body="Prose here.\n\nBREAKING CHANGE: the old name is gone.\n",
            tag="v0.2.0")

    def _run(self, *args):
        """Run a command in the throwaway repo."""
        subprocess.run(args, check=True, capture_output=True)

    def _commit(self, subject, body=None, tag=None):
        """Add one commit, optionally with a body and a tag."""
        (self.repo / "f.txt").write_text(subject)
        self._run("git", "add", "f.txt")
        msg = subject if body is None else f"{subject}\n\n{body}"
        self._run("git", "commit", "-q", "-m", msg)
        if tag:
            self._run("git", "tag", tag)

    def test_previous_tag_resolves_by_version(self):
        """previous_tag finds the preceding version tag."""
        self.assertEqual(release_notes.previous_tag("v0.2.0"), "v0.1.0")
        self.assertIsNone(release_notes.previous_tag("v0.1.0"))

    def test_commits_returns_subject_body_pairs(self):
        """commits() shells out successfully and splits subject from body."""
        got = release_notes.commits("v0.1.0", "v0.2.0")
        self.assertEqual(len(got), 2)
        subjects = [s for s, _ in got]
        self.assertIn("fix(b): second thing (#7)", subjects)
        self.assertIn("refactor(c): third thing (#8)", subjects)

    def test_end_to_end_render_marks_the_breaking_commit(self):
        """A body-declared break survives the git round trip into Breaking."""
        out = release_notes.render(
            release_notes.commits("v0.1.0", "v0.2.0"), "v0.1.0", "v0.2.0", REPO)
        self.assertIn("### Breaking", out)
        self.assertIn("third thing", out)
        self.assertIn("### Fixed", out)

    def test_empty_range_against_real_git(self):
        """A tag with no new commits renders the no-changes line."""
        self._run("git", "tag", "v0.3.0")
        out = release_notes.render(
            release_notes.commits("v0.2.0", "v0.3.0"), "v0.2.0", "v0.3.0", REPO)
        self.assertIn("No source changes since v0.2.0", out)


if __name__ == "__main__":
    unittest.main(verbosity=0)
