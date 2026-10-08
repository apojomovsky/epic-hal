#!/usr/bin/env bash
# Commit-msg checks: no attribution trailers, no em-dashes. Run from
# .git/hooks/commit-msg by the install-git-hooks.sh dispatcher.
#
# Trailers are rejected because git history is the human author's
# record; em-dashes violate the repo prose rule (AGENTS.md). Skip one
# commit with `git commit --no-verify`.

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
