#!/usr/bin/env bash
# Allman brace-style gate on added lines: an opening brace owns its line
# for function/control blocks (attached `){`, `else {`, `} else {` are
# violations). Scoped to added lines so pre-existing style never blocks
# an unrelated commit; initializers, compound literals, macro `do {`,
# strings and comments are exempt. Full logic + rationale:
# scripts/brace-style-check.py and scripts/README.md.
# Usage: brace-style-check.sh [base-ref]; no arg = staged index.
set -u
SCRIPT_DIR="$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")" && pwd)"
exec python3 "${SCRIPT_DIR}/brace-style-check.py" "$@"
