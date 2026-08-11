# CI Non-code Skip Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the two divergent "docs-only" CI classifiers with one fail-closed `is_non_code` classifier whose skip class is "cannot affect the object pipeline" (docs, assets, configs, dev-only tooling), so tooling-only PRs skip the host ctest matrix and the family jobs.

**Architecture:** One python module `scripts/ci_noncode_check.py` owns the allowlist (pure `is_non_code(changed_files)` + a `true`/`false` CLI over `git diff --name-only base...HEAD`). `ci-discover-affected-modules.py` imports the function (it already computes the same diff); `family-check.yml` calls the CLI. `ci-docs-only-check.sh` is deleted. Output key `docs_only` becomes `non_code`; gate polarity and every other behavior (lint always, master always full, module narrowing) unchanged.

**Tech Stack:** python3 (stdlib only), GitHub Actions YAML, git, unittest.

## Global Constraints

- No em-dash characters in docs, commit messages, or code comments; the word "em-dash" is fine.
- Conventional Commits (`feat`/`docs`/`plan`/`fix`/`refactor`/`style`), scope usually the module.
- The skip allowlist lives in exactly one place: `scripts/ci_noncode_check.py`. No other file re-implements or duplicates it.
- Fail closed: any changed file not on the allowlist is code-affecting (full CI); an empty diff counts as non-code (skip).
- `.github/workflows/**` is never on the allowlist; a workflow change always runs full CI.
- Naming: `docs_only` output keys and step ids become `non_code`; all gates keep their `!= 'true'` shape. Module narrowing, master-push behavior, and lint-always are unchanged.
- The classifier only gates `pull_request` runs (workflows already force `false` on non-PR runs).

---

### Task 1: The classifier and its unit tests

**Files:**
- Create: `scripts/ci_noncode_check.py`
- Create: `scripts/tests/test_ci_noncode.py`

**Interfaces:**
- Produces: `is_non_code(changed_files: list[str]) -> bool` (pure, the single source of truth) and CLI `python3 scripts/ci_noncode_check.py <base-ref>` printing `true`/`false` (diffs `<base>...HEAD`; empty diff prints `true`). Task 2 imports the function; Task 3 calls the CLI.
- Note: the spec named the file `ci-non-code-check.py`; hyphens are not importable, so the file is `ci_noncode_check.py` (underscores), the only deviation from the spec.

- [ ] **Step 1: Write `scripts/ci_noncode_check.py`**

```python
#!/usr/bin/env python3
"""Shared CI classifier: can this change affect the build?

Single source of truth for "can this PR's changed files affect the
object-creation pipeline?" Both CI consumers use it (design:
docs/superpowers/specs/2026-08-11-ci-non-code-skip-design.md):

  - ci.yml's host job reaches it through ci-discover-affected-modules.py,
    which imports is_non_code() and already computes the same
    changed-file list for module narrowing.
  - family-check.yml calls the CLI:
    python3 scripts/ci_noncode_check.py <base-ref>, printing "true"
    (skip the real-target steps) or "false" (run full CI).

Fail-closed on purpose: anything not on the explicit allowlist below
counts as code-affecting, so a misclassified file can only cause an
extra CI run, never let a real break merge unverified. Workflow files
(.github/workflows/**) are deliberately NOT on the list: a broken CI
change is worse than a wasted run. Every push to master still runs the
full matrix regardless (see ci.yml); this classifier only ever gates
pull_request runs.

The allowlist (the entire definition of "non-code", one place):
  *.md                                     markdown, anywhere
  any docs/ directory                      at any depth
  LICENSE*                                 license text
  .gitignore .gitattributes .clang-format .editorconfig
  *.svg *.png *.jpg *.jpeg *.gif *.ico     image assets
  scripts/bootstrap.sh                     dev-only, CI never runs it
  scripts/install-git-hooks.sh             dev-only, CI never runs it
  Makefile                                 repo-root dev entry only (CI
                                           greps the Dockerfile ARGs,
                                           never reads the Makefile);
                                           nested Makefiles (examples/
                                           *.X/Makefile, third_party/)
                                           are real build inputs and
                                           stay code-affecting
"""

import re
import subprocess
import sys

_NON_CODE_RE = re.compile(
    r"("
    r"\.md$"
    r"|(^|/)docs/"
    r"|^LICENSE"
    r"|\.gitignore$|\.gitattributes$|\.clang-format$|\.editorconfig$"
    r"|\.(svg|png|jpg|jpeg|gif|ico)$"
    r"|^scripts/bootstrap\.sh$"
    r"|^scripts/install-git-hooks\.sh$"
    r"|^Makefile$"
    r")"
)


def is_non_code(changed_files):
    """True iff every changed file is provably unable to affect the build."""
    return all(_NON_CODE_RE.search(p) for p in changed_files)


def main():
    base = sys.argv[1] if len(sys.argv) > 1 else ""
    if not base:
        sys.stderr.write("usage: ci_noncode_check.py <base-ref>\n")
        return 2
    changed = [
        line
        for line in subprocess.run(
            ["git", "diff", "--name-only", f"{base}...HEAD"],
            capture_output=True, text=True, check=True,
        ).stdout.splitlines()
        if line
    ]
    print("true" if is_non_code(changed) else "false")
    return 0


if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Step 2: Write `scripts/tests/test_ci_noncode.py`**

```python
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
```

- [ ] **Step 3: Run the tests**

Run: `python3 scripts/tests/test_ci_noncode.py`
Expected: all 16 tests pass (`OK`).

- [ ] **Step 4: Commit**

```bash
git add scripts/ci_noncode_check.py scripts/tests/test_ci_noncode.py
git commit -m "feat(ci): add shared non-code classifier"
```

- [ ] **Step 5: CLI smoke test after the commit (false path, own files are code)**

The diff must be taken after the commit, otherwise `origin/master...HEAD` still resolves to the docs-only spec/plan commits and the verdict would be `true`. After Step 4's commit, HEAD includes the classifier's own files, which are code.

Run: `python3 scripts/ci_noncode_check.py origin/master`
Expected: `false` (the classifier's own new files are code-affecting, correctly).
Agreement check: the CLI and the imported function must agree on the same diff:

Run: `python3 - <<'PY'
import subprocess, sys
sys.path.insert(0, "scripts")
import ci_noncode_check
changed = [l for l in subprocess.run(["git", "diff", "--name-only", "origin/master...HEAD"], capture_output=True, text=True, check=True).stdout.splitlines() if l]
print("function:", ci_noncode_check.is_non_code(changed))
PY`
Expected: `function: False` (same verdict as the CLI).

---

### Task 2: Host job routes through the shared classifier

**Files:**
- Modify: `scripts/ci-discover-affected-modules.py`
- Modify: `.github/workflows/ci.yml` (host job only)

**Interfaces:**
- Consumes: `is_non_code` from Task 1 (importable as `ci_noncode_check` because the workflow runs `python3 scripts/ci-discover-affected-modules.py`, so `scripts/` is sys.path[0]).
- Produces: host discover output key `non_code`; the build-test gate reads it. Task 3 does the family-check side; the two workflows are independently consistent (each emits and reads its own `non_code` key).

- [ ] **Step 1: Edit `scripts/ci-discover-affected-modules.py` docstring (first question)**

Old:

```python
  1. Is this change docs-only? If every changed file is a .md or lives under
     a docs/ directory, the build-test matrix is empty and the whole
     build-test job is skipped (a real cmake configure+build+ctest run
     cannot be affected by prose, and this repo has no doc-generation step
     that consumes source to produce docs, so there is no direction in
     which a docs change could break a build).
```

New:

```python
  1. Is this change non-code? If every changed file is on the shared
     non-code allowlist (see ci_noncode_check.py, the single source of
     truth), the build-test matrix is empty and the whole build-test job
     is skipped (a real cmake configure+build+ctest run cannot be
     affected by prose or by dev-only tooling).
```

- [ ] **Step 2: Edit the docstring's conservative paragraph and output line**

Old: the docstring's conservative bullet, the paragraph starting `Any changed file that is not (a) a .md file, ...` and ending `so it doesn't try.` The file's copy contains a literal em-dash after `every module affected"`; copy the real text from the file for the edit's old_string (it will contain that character) and make sure the replacement below carries none.

New:

```python
  - Any changed file that is not (a) on the shared non-code allowlist,
    (b) inside a discovered module's own directory, or (c) inside
    epic-common/ falls back to "not non-code, every module affected": a
    workflow, CI script, or Docker change is exactly the kind of change
    this script cannot reason about safely, so it doesn't try.
```

Old (last docstring line):

```python
Prints one JSON object to stdout: {"docs_only": bool, "modules": [...]}.
```

New:

```python
Prints one JSON object to stdout: {"non_code": bool, "modules": [...]}.
```

- [ ] **Step 3: Add the import**

Old:

```python
import json
import re
import subprocess
import sys
```

New:

```python
import json
import re
import subprocess
import sys

import ci_noncode_check
```

- [ ] **Step 4: Rewrite `main()`'s docs classification**

Old:

```python
    if not changed:
        # Nothing changed in range (e.g. an empty/merge commit): treat as
        # docs-only rather than guessing, the safest "nothing to verify"
        # signal available.
        print(json.dumps({"docs_only": True, "modules": []}))
        return

    def is_docs(path):
        return path.endswith(".md") or "/docs/" in path or path.startswith("docs/")

    if all(is_docs(p) for p in changed):
        print(json.dumps({"docs_only": True, "modules": []}), file=sys.stdout)
        print(f"docs-only change ({len(changed)} file(s)), skipping build-test", file=sys.stderr)
        return
```

New:

```python
    if not changed:
        # Nothing changed in range (e.g. an empty/merge commit): treat as
        # non-code rather than guessing, the safest "nothing to verify"
        # signal available.
        print(json.dumps({"non_code": True, "modules": []}))
        return

    if ci_noncode_check.is_non_code(changed):
        print(json.dumps({"non_code": True, "modules": []}), file=sys.stdout)
        print(f"non-code change ({len(changed)} file(s)), skipping build-test", file=sys.stderr)
        return
```

- [ ] **Step 5: Update the attribution loop and the two remaining prints**

Old (loop body):

```python
    for p in changed:
        if is_docs(p):
            continue
```

New:

```python
    for p in changed:
        if ci_noncode_check.is_non_code([p]):
            continue
```

Old (fallback print):

```python
        print(json.dumps({"docs_only": False, "modules": modules}))
```

New:

```python
        print(json.dumps({"non_code": False, "modules": modules}))
```

Old (final print):

```python
    print(json.dumps({"docs_only": False, "modules": affected_sorted}))
```

New:

```python
    print(json.dumps({"non_code": False, "modules": affected_sorted}))
```

- [ ] **Step 6: Edit `.github/workflows/ci.yml` host job**

Comment above the host job:

Old: `  # docs-only PRs skip the build+test loop but still run lint (a`
New: `  # non-code PRs skip the build+test loop but still run lint (a`

Discover-step comment:

Old: `      # ... narrowed list. Docs-only PRs get an`
New: `      # ... narrowed list. Non-code PRs get an`

(Adjust the surrounding words if the line breaks differ; the two lines above are the only "Docs-only" mentions in the host job.)

Discover-step output line:

Old: `            echo "docs_only=$(echo "$result" | jq -r '.docs_only')" >> "$GITHUB_OUTPUT"`
New: `            echo "non_code=$(echo "$result" | jq -r '.non_code')" >> "$GITHUB_OUTPUT"`

Build-test gate:

Old: `      if: steps.discover.outputs.docs_only != 'true'`
New: `      if: steps.discover.outputs.non_code != 'true'`

Also the "Determine base ref" step comment says `the docs-only/module`; change to `the non-code/module`.

- [ ] **Step 7: Verify against real diffs**

Run: `git diff --name-only origin/master...feat/bootstrap-docker-toolchain | python3 -c 'import sys; sys.path.insert(0, "scripts"); import ci_noncode_check; files=[l.strip() for l in sys.stdin if l.strip()]; print(ci_noncode_check.is_non_code(files))'`
Expected: `True` (PR #32's 7 files are all on the allowlist).

Run: `git diff --name-only fcd895e^1...fcd895e | python3 -c 'import sys; sys.path.insert(0, "scripts"); import ci_noncode_check; files=[l.strip() for l in sys.stdin if l.strip()]; print(ci_noncode_check.is_non_code(files))'`
Expected: `False` (the mcp23x17 merge added code).

Run: `python3 -m py_compile scripts/ci_noncode_check.py scripts/ci-discover-affected-modules.py`
Expected: no output, exit 0.

- [ ] **Step 8: Commit**

```bash
git add scripts/ci-discover-affected-modules.py .github/workflows/ci.yml
git commit -m "feat(ci): host job skips tests on non-code changes"
```

---

### Task 3: Family checks route through the shared classifier

**Files:**
- Modify: `.github/workflows/family-check.yml`
- Modify: `.github/workflows/ci.yml` (family-job comment only)

**Interfaces:**
- Consumes: the CLI from Task 1 (`python3 scripts/ci_noncode_check.py <base>`).
- Produces: family-check emits `non_code` and gates its five real-target steps on it; identical to the old `docs_only` behavior with the widened definition.

- [ ] **Step 1: Edit `family-check.yml` "Determine docs-only" step**

Old:

```yaml
      - name: Determine docs-only
        id: docs
        run: |
          if [ "${{ github.event_name }}" = "pull_request" ]; then
            result="$(bash scripts/ci-docs-only-check.sh '${{ github.event.pull_request.base.sha }}')"
          else
            result="false"
          fi
          echo "docs_only=$result" >> "$GITHUB_OUTPUT"
```

New:

```yaml
      - name: Determine non-code
        id: noncode
        run: |
          if [ "${{ github.event_name }}" = "pull_request" ]; then
            result="$(python3 scripts/ci_noncode_check.py '${{ github.event.pull_request.base.sha }}')"
          else
            result="false"
          fi
          echo "non_code=$result" >> "$GITHUB_OUTPUT"
```

- [ ] **Step 2: Update the five gates**

Replace every occurrence of `if: steps.docs.outputs.docs_only != 'true'` with `if: steps.noncode.outputs.non_code != 'true'` (seven occurrences, one per gated step: image pull, device-data audits, hex-rebuild identity, emit, real XC8 builds, MPLAB SIM runs, isolated bundle build; verify with a grep after editing that no `docs_only` gate remains).

- [ ] **Step 3: Edit `ci.yml`'s family-job comment**

Old: `  # family-check.yml: the calls run concurrently on separate runners, so the PR wait is the\n  # largest family's time, not the sum. Sharded 2026-08-11 (was one\n  # ~23 min target job; the per-family jobs land ~8-10 min wall). A\n  # docs-only PR skips everything past the pull in every family.`
New: same with `docs-only` -> `non-code`.

- [ ] **Step 4: Verify no stale references remain**

Run: `grep -rn "docs_only\|ci-docs-only-check\|is_docs" .github/ scripts/ci-discover-affected-modules.py scripts/ci_noncode_check.py`
Expected: no matches (the docstring refresh in Task 2 removed the last `is_docs`).

Run: `python3 -m py_compile scripts/ci_noncode_check.py`
Expected: no output, exit 0.

- [ ] **Step 5: Commit**

```bash
git add .github/workflows/family-check.yml .github/workflows/ci.yml
git commit -m "feat(ci): family checks skip on non-code changes"
```

---

### Task 4: Delete the old classifier, refresh scripts/README.md

**Files:**
- Delete: `scripts/ci-docs-only-check.sh`
- Modify: `scripts/README.md` (the "CI change-scoping" section)

**Interfaces:**
- Consumes: Tasks 1-3 (no caller of `ci-docs-only-check.sh` remains).
- Produces: one classifier story in the docs; no stale workflow names.

- [ ] **Step 1: Delete the script**

Run: `git rm scripts/ci-docs-only-check.sh`
Expected: staged deletion.

- [ ] **Step 2: Replace the "CI change-scoping" section in `scripts/README.md`**

Old (the whole section, from `## CI change-scoping (docs-only skip, affected-module narrowing)` through the end of its two bullets):

New:

```markdown
## CI change-scoping (non-code skip, affected-module narrowing)

Two classifiers keep cheap PRs cheap, both fail-closed (anything they
cannot attribute means full CI, so a wrong call can only cause an extra
run, never let a real break merge unverified):

- `ci_noncode_check.py <base-ref>`: the single source of truth for "can
  this change affect the build?". Prints `true` only when every changed
  file is on its explicit non-code allowlist (markdown, any `docs/`
  directory, `LICENSE*`, repo config files, image assets, and the
  dev-only `scripts/bootstrap.sh` / `scripts/install-git-hooks.sh` /
  repo-root `Makefile`). Workflow files and everything else are code by
  default. `family-check.yml` calls it to skip all real-target steps on
  a non-code PR; `ci-discover-affected-modules.py` imports
  `is_non_code()` for the host job.
- `ci-discover-affected-modules.py [base-ref]`: used by `ci.yml`'s
  `host` job. Answers two questions: is the change non-code (imports
  `ci_noncode_check`), and which modules were actually touched,
  directly or through a sibling dependency (read straight from each
  module's own `CMakeLists.txt`, no separately-maintained dependency
  graph to drift). Conservative on both axes, falls back to the full,
  unfiltered module list the moment it sees a changed file it can't
  attribute to a known module or `epic-common/`. Only ever applied to
  `pull_request` runs; every push to `master` always gets the full
  matrix regardless of what changed.
```

- [ ] **Step 3: Verify**

Run: `grep -rn "ci-docs-only-check\|host-tests.yml" scripts/README.md`
Expected: no matches (the README's stale workflow names are gone from this section; if any other README section still names a deleted workflow, leave it, it is out of scope).

- [ ] **Step 4: Commit**

```bash
git add scripts/README.md
git commit -m "refactor(ci): remove ci-docs-only-check.sh, refresh CI change-scoping docs"
```

---

### Task 5: Final verification and status flips

**Files:**
- Modify: `docs/superpowers/specs/2026-08-11-ci-non-code-skip-design.md` (Status line)
- Modify: `docs/superpowers/plans/2026-08-11-ci-non-code-skip.md` (Status line)

**Interfaces:**
- Consumes: the finished Tasks 1-4 branch state.
- Produces: evidence the shipped contract holds. The spec and plan files are dropped from the PR before it opens (user preference, same as PR #32), so these flips are process records on the branch, not PR content.

- [ ] **Step 1: Full test suite for the change**

Run: `python3 scripts/tests/test_ci_noncode.py`
Expected: 16 tests pass.

- [ ] **Step 2: Re-run the real-diff checks**

Run the two `git diff --name-only ... | python3 -c ...` commands from Task 2 Step 7 again, plus `python3 scripts/ci_noncode_check.py origin/master` (expect `false`).
Expected: `True`, `False`, `false` respectively.

- [ ] **Step 3: Whole-branch review prep**

Run: `git status --short` (clean) and `git log --oneline origin/master..HEAD` (6 commits: spec, plan, and the four task commits in order).
Run: `grep -rn "docs_only" .github/ scripts/ | grep -v Binary`, expected: no matches.

- [ ] **Step 4: Flip the spec and plan Status lines**

Spec: `Status: **approved 2026-08-11, not started**.` -> `Status: **implemented 2026-08-11**.`
Plan: add `Status: **implemented 2026-08-11**.` under the header line.

- [ ] **Step 5: Commit**

```bash
git add docs/superpowers/specs/2026-08-11-ci-non-code-skip-design.md docs/superpowers/plans/2026-08-11-ci-non-code-skip.md
git commit -m "plan(ci): mark non-code skip plan implemented"
```

---

### Post-plan (PR-time, not a task)

Before opening the PR: `git rm docs/superpowers/specs/2026-08-11-ci-non-code-skip-design.md docs/superpowers/plans/2026-08-11-ci-non-code-skip.md` and commit, so the PR carries only the feature files (the user's standing preference). The branch keeps the full history for the record. Push and open the PR; it changes `.github/workflows/**`, so it runs full CI by design.
