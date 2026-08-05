#!/usr/bin/env bash
# Print "true" to stdout if every file changed between $1 and HEAD is a
# .md file or lives under a docs/ directory (anywhere in the tree, not
# just the root docs/), "false" otherwise.
#
# Used by xc8-build.yml and sim-tests.yml to skip their expensive
# Docker-based jobs (image pull, real XC8 cross-compiles, mdb runs) on a
# pure documentation change; host-tests.yml has the same "docs_only"
# concept but computes it itself inside scripts/
# ci-discover-affected-modules.py, since that script needs the identical
# changed-file list anyway to also decide which modules were touched.
# Keep both definitions of "is this file docs" in sync if either changes
# (grep for `is_docs` there, the pattern here).
#
# Usage: ci-docs-only-check.sh <base-ref>
#   base-ref  the ref to diff against (a PR's base SHA); this script does
#             not do host-tests.yml's push-to-master / zero-SHA fallback
#             resolution itself, callers pass an already-resolved ref.

set -euo pipefail

base="$1"

changed="$(git diff --name-only "${base}...HEAD")"

if [ -z "$changed" ]; then
    # Nothing changed in range: treat as docs-only, the same
    # nothing-to-verify posture ci-discover-affected-modules.py takes.
    echo "true"
    exit 0
fi

if echo "$changed" | grep -qvE '(\.md$|(^|/)docs/)'; then
    echo "false"
else
    echo "true"
fi
