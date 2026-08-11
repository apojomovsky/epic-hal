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
