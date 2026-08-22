#!/usr/bin/env bash
# The commit-msg checks: no attribution trailers, no em-dashes. Installed
# into .git/hooks/commit-msg by scripts/install-git-hooks.sh.
#
# Trailers are rejected because git history is the human author's record:
# an agent-generated commit is still authored by the person who reviewed
# and landed it, and a Co-Authored-By line makes the release notes
# (scripts/release_notes.py reads these subjects) speak for someone who
# did not sign off. Em-dashes are this repo's documented prose rule
# (AGENTS.md); the same rule the pre-commit hook applies to added lines.
#
# Skip for one commit with `git commit --no-verify`.

set -uo pipefail

msg_file="$1"
fail=0

if grep -qiE '^(co-authored-by|coauthored-by|authored-by|claude-session|generated-with):' "$msg_file"; then
    echo "commit-msg: attribution trailer in the commit message (forbidden, AGENTS.md)."
    echo "commit-msg:   git history is the human author's record; drop the trailer."
    fail=1
fi

if grep -q '—' "$msg_file"; then
    echo "commit-msg: em-dash (U+2014) in the commit message (repo rule: no em-dashes)."
    echo "commit-msg:   use a comma, a colon, or a period and a new sentence."
    fail=1
fi

[ "$fail" -ne 0 ] && exit 1
exit 0
