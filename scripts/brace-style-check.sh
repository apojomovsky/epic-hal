#!/usr/bin/env bash
# Allman brace-style gate on added lines: an opening brace owns its line
# for function/control blocks (attached `){`, `else {`, `} else {` are
# violations). Scoped to added lines so pre-existing style never blocks
# an unrelated commit; initializers, macro `do {` and strings are exempt.
# Full rationale: scripts/README.md. Exits 1 with file:line hits.
set -u

base="${1:-${PRE_COMMIT_BASE_REF:-}}"
if [ -n "$base" ]; then
    diff="$(git diff -U0 "${base}...HEAD" --diff-filter=ACM -- '*.c' '*.h')"
else
    diff="$(git diff -U0 --cached --diff-filter=ACM -- '*.c' '*.h')"
fi
[ -z "$diff" ] && exit 0

hits="$(awk '
    /^\+\+\+ / { file = substr($0, 7); next }
    /^@@/ {
        match($0, /\+[0-9]+/)
        newline = substr($0, RSTART + 1, RLENGTH - 1) + 0
        next
    }
    /^\+/ {
        if (index($0, "{") > 0) {
            line = substr($0, 2)
            # Skip comment lines and braces inside quoted strings.
            if (line ~ /^\/\// || line ~ /^ \*/) { newline++; next }
            before = line
            sub(/{.*/, "", before)
            nq = gsub(/"/, "", before)
            if (nq % 2 == 0) {
                # Trim trailing whitespace, then classify the opener.
                sub(/[ \t]+$/, "", before)
                # Block openers: function/control (ends with ")"), else
                # (ends with "else"), or a naked switch/case brace. The
                # multi-statement macro idiom `do {` is exempt (it is the
                # accepted form even in Allman code; do-loops share the
                # exemption). Initializers (`= {`, `( {`, `, {`, `[ {`,
                # `struct {`, `enum {`) are NOT blocks.
                if (before ~ /\)$/ || before ~ /else$/ || before == "}") {
                    if (before !~ /^do$| [ \t]*do$/) {
                        print file ":" newline ": " line
                    }
                }
            }
        }
        newline++
        next
    }
' <<< "$diff")"

if [ -n "$hits" ]; then
    echo "brace-style: opening brace must be on its own line (Allman) in added lines:"
    echo "$hits" | sed 's/^/  /'
    exit 1
fi
exit 0
