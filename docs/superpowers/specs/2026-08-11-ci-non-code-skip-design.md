# CI: skip unit tests and real-target gates on non-code changes

Status: **approved 2026-08-11, not started**.

## Problem

The repo already skips CI work on docs-only PRs, but the skip class is
"docs", not "cannot affect the object pipeline", and the definition
lives in two places in two languages with a "keep in sync" warning
between them (`scripts/ci-docs-only-check.sh` grep pattern and
`is_docs` inside `scripts/ci-discover-affected-modules.py`).

The gap shows up on tooling PRs: PR #32 changed `scripts/bootstrap.sh`
and the root `Makefile`, both of which CI never executes (the host job
installs its own apt packages; the workflows grep the Dockerfile's ARGs
directly and never read the Makefile), yet the docs-only classifiers
could not attribute them, so the PR ran the full 19-module host ctest
matrix plus all three family jobs (XC8 builds, mdb gates, audits,
bundle). Neither file can affect a single object file.

## Goal

Widen the skip class from "docs only" to "cannot affect the object
pipeline", with one fail-closed classifier as the single source of
truth. A PR that touches only non-code files skips the host ctest
matrix and the family jobs entirely; lint still always runs, and
master pushes still always run the full matrix.

## Decisions (user-approved)

1. **Skip class = docs + assets + configs + dev-only tooling.** The
   classifier prints "skip" only when every changed file is on the
   explicit allowlist below. Any file not on the list means full CI.
2. **One classifier, python, one place.** New
   `scripts/ci-non-code-check.py` exposes a pure function
   `is_non_code(changed_files) -> bool` plus a CLI that prints
   `true`/`false` for a base-ref diff. `ci-discover-affected-modules.py`
   imports the function (it already computes the same changed-file
   list); `family-check.yml` calls the CLI. `scripts/ci-docs-only-check.sh`
   is deleted; the two-definitions landmine is gone.
3. **Dev-only tooling carve-out:** `scripts/bootstrap.sh`,
   `scripts/install-git-hooks.sh`, and the root `Makefile` are on the
   skip list (CI never executes them). `scripts/pre-commit-checks.sh`
   stays code-affecting: the host job runs it.
4. **Workflow changes always run full CI.** `.github/workflows/**` is
   not on the skip list; an untested CI change is worse than a wasted
   run, and this very change ships one (its own PR runs the full
   matrix by design).
5. **Unknown files fail closed** to full CI; empty diff counts as
   skip (nothing to verify), matching today's posture.
6. **Naming:** the `docs_only` output keys and step ids become
   `non_code`; the host job's build-test gate and every family-check
   gate keep their `!= 'true'` shape. Master pushes and module
   narrowing semantics are unchanged.

## Skip allowlist (the entire definition, one place)

A changed file is non-code iff it matches any of:

- `*.md` (any markdown, anywhere)
- any `docs/` directory at any depth
- `LICENSE*`
- `.gitignore`, `.gitattributes`, `.clang-format`, `.editorconfig`
- image assets: `*.svg`, `*.png`, `*.jpg`, `*.jpeg`, `*.gif`, `*.ico`
- `scripts/bootstrap.sh`, `scripts/install-git-hooks.sh`
- the repo-root `Makefile` only, path exactly `Makefile` (nested
  Makefiles are real build inputs and stay code-affecting)

Everything else: any `.c`/`.h`, `CMakeLists.txt`, the manifest,
`epic_build.py`, `epicmanifest.py`, all other `scripts/`, the audit
scripts, `make_bundle.py`, `bundlegen.py`, the Dockerfile, any nested
Makefile (e.g. `examples/*.X/Makefile`), and `.github/workflows/**`
are code-affecting and run full CI.

## Components

- Create: `scripts/ci-non-code-check.py` (pure `is_non_code` + CLI).
- Modify: `scripts/ci-discover-affected-modules.py` (import the
  function, drop its inline `is_docs`, output key `non_code`).
- Modify: `.github/workflows/ci.yml` (host discover step and the
  build-test gate; header comment).
- Modify: `.github/workflows/family-check.yml` ("Determine docs-only"
  step calls the new CLI; `docs` step id and five `docs_only` gates
  become `non_code`).
- Delete: `scripts/ci-docs-only-check.sh`.
- Modify: `scripts/README.md` (the CI change-scoping section already
  names deleted workflows; refresh it to describe the new classifier
  and the real consumer set).
- Create: `scripts/tests/test_ci_noncode.py` (pure-function unit
  tests; the existing `scripts/tests/` pattern has `__init__.py` and
  `test_*.py` files, run the same way `test_epic_build.py` is).

## Verification

- Unit tests: `is_non_code` against representative lists: docs-only
  `true`; `scripts/bootstrap.sh`-only `true`; root `Makefile`-only
  `true`; any `.c`/`.h`/`CMakeLists.txt`/manifest/`epic_build.py`/
  workflow/Dockerfile/`pre-commit-checks.sh`/`sim-mdb-run.sh`/
  `examples/epicurus-demo-pic16f87xa.X/Makefile` `false`;
  mixed list with one code file `false`; unknown extension `false`;
  empty list `true`.
- Direct runs against real diffs in the repo: a docs-only diff,
  a bootstrap.sh-only diff, and a code diff, checking the printed
  verdict matches.
- `python3 scripts/ci-non-code-check.py` CLI agrees with the imported
  function on the same input.
- The change's own PR touches `.github/workflows/**`, so CI runs the
  full matrix on it; that is the workflow-file rule working.

## Out of scope

- Module narrowing semantics in `ci-discover-affected-modules.py`
  (unchanged beyond the import and the rename).
- Master-push behavior (always full) and lint-always behavior
  (unchanged).
- The pre-existing staleness in `AGENTS.md`'s CI paragraph (it still
  describes the pre-consolidation two-job layout); noted, not fixed
  here.
