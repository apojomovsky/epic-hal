#!/usr/bin/env bash
# Takeoff ritual: run before opening a PR. Verifies the branch meets the
# repo's merge conventions and prints exactly what still has to be done.
# Exits 1 while blocking items are outstanding; advisory items warn only.
# AGENTS.md's "Takeoff ritual" section is the rationale for each step.
#
# Complements scripts/pre-commit-checks.sh rather than repeating it: the
# hook gates one commit's staged content, this gates the whole PR range.
#
# Usage: bash scripts/pre-pr-check.sh [--test] [--prose]
#   --test    also run the full host-sim suite (make test); slow, opt-in
#   --prose   attest that scripts/prose-diff.sh's output has been read and
#             judged against AGENTS.md's Expression Conventions
#   BASE_REF overrides the base branch (forks: BASE_REF=<fork>/master)

set -uo pipefail

BASE_REF="${BASE_REF:-origin/master}"
RUN_TESTS=0
RUN_PROSE=0
for arg in "$@"; do
    case "$arg" in
        --test)  RUN_TESTS=1 ;;
        --prose) RUN_PROSE=1 ;;
    esac
done

repo_root="$(git rev-parse --show-toplevel)"
cd "$repo_root" || exit 1

FAILS=0
WARNS=0

say()  { printf '%s\n' "$*"; }
ok()   { say "  [ok] $*"; }
warn() { say "  [warn] $*"; WARNS=$((WARNS + 1)); }
fail() { say "  [FAIL] $*"; FAILS=$((FAILS + 1)); }

branch=$(git branch --show-current)
say "== Takeoff ritual: ${branch:-detached HEAD} =="
say ""

# ---- 1. working tree must be clean ----
if [ -n "$(git status --porcelain)" ]; then
    fail "working tree has uncommitted changes (commit or stash first)"
else
    ok "working tree clean"
fi

# ---- 2. branch must not be behind the base ----
if git rev-parse --verify -q "$BASE_REF" >/dev/null 2>&1; then
    behind=$(git rev-list --count "HEAD..$BASE_REF")
    if [ "$behind" -gt 0 ]; then
        warn "branch is $behind commit(s) behind $BASE_REF; rebase before merging"
    else
        ok "up to date with $BASE_REF"
    fi
else
    warn "$BASE_REF not found; fetch first: git fetch origin master"
fi

# ---- 3. plan docs must not reach master ----
plans=$(git diff --name-only "$BASE_REF"...HEAD -- docs/superpowers/plans/ 2>/dev/null)
if [ -n "$plans" ]; then
    fail "plan file(s) in the PR's final diff (must not reach master):"
    for p in $plans; do
        say "    - $p"
    done
    say "    Fix:"
    say "      1. Distill the durable facts into the living docs: the"
    say "         module README/docs/, MANUAL.md for a register fact,"
    say "         DEVELOPMENT.md or docs/adding-a-device.md for a"
    say "         toolchain or debug gotcha."
    say "      2. git rm the plan in a final commit."
    say "    The plan stays in the PR's commit history (archaeology);"
    say "    master must not carry it."
else
    ok "no plan files in the PR diff"
fi

# ---- 4. commit hygiene: conventional, no trailers, no em-dash ----
n=$(git rev-list --count "$BASE_REF..HEAD" 2>/dev/null || echo 0)
if [ "$n" -eq 0 ]; then
    fail "no commits ahead of $BASE_REF; nothing to PR"
else
    say "checking $n commit(s):"
    while IFS= read -r -d '' hash; do
        IFS= read -r -d '' subject || break
        IFS= read -r -d '' body || break
        # git log separates commits with a newline, which lands at the
        # front of the next NUL-delimited hash field.
        hash=${hash#$'\n'}
        short=${hash:0:7}
        if [[ "$subject" == Merge\ * ]]; then
            ok "$short merge commit (git-generated, skipped)"
            continue
        fi
        if [[ "$subject" =~ ^(feat|fix|docs|plan|refactor|style|chore|build|ci|test|perf|revert)(\([a-z0-9._-]+\))?!?: ]]; then
            ok "$short conventional"
        else
            fail "$short not conventional: $subject"
        fi
        [ ${#subject} -gt 72 ] && warn "$short subject ${#subject} chars (keep <= 72)"
        if printf '%s\n%s' "$subject" "$body" | grep -qiE '^(co-authored-by|coauthored-by|authored-by|claude-session|generated-with):'; then
            fail "$short carries an attribution trailer (forbidden, AGENTS.md)"
        fi
        if printf '%s\n%s' "$subject" "$body" | grep -q '—'; then
            fail "$short contains an em-dash in the commit message (use a comma, colon, or period)"
        fi
    done < <(git log "$BASE_REF..HEAD" --format='%H%x00%s%x00%b%x00')
fi

# ---- 5. whitespace errors in the PR diff ----
if out=$(git diff "$BASE_REF"...HEAD --check 2>&1); then
    ok "no whitespace errors"
else
    printf '%s\n' "$out" | head -10
    fail "whitespace errors in the diff (above); fix and re-stage"
fi

# ---- 6. em-dashes and the residue of a mechanical sweep ----
# Only ADDED lines are scanned: context lines may carry pre-existing
# em-dashes from lines the PR merely touches. AGENTS.md, the hook, and
# this script are excluded by design: they document the rule and need
# the character in their own enforcement patterns.
prose_paths=(. ':(exclude)docs/superpowers/plans/'
             ':(exclude)AGENTS.md' ':(exclude)CLAUDE.md'
             ':(exclude)scripts/pre-pr-check.sh'
             ':(exclude)scripts/prose-diff.sh'
             ':(exclude)scripts/pre-commit-checks.sh'
             ':(exclude)scripts/commit-msg-checks.sh'
             ':(exclude)*/third_party/*' ':(exclude)*/vendor/*')

emdashes=$(git diff "$BASE_REF"...HEAD -- "${prose_paths[@]}" 2>/dev/null \
    | grep -nE '^\+[^+].*—' | head -10)
if [ -n "$emdashes" ]; then
    printf '    %s\n' "$emdashes"
    fail "em-dashes (—) in the PR's diff (forbidden in prose; use a comma,"
    fail "      colon, or period. Exception: ascii-art/diagrams)."
else
    ok "no em-dashes in the diff"
fi

# Space-before-comma: the residue a mechanical em-dash sweep leaves
# behind (` — ` replaced by ` , `, a comma splice). The scan above
# cannot catch it, because the sweep deleted every em-dash it swept.
# Advisory, not blocking: picking a comma, a colon, or a period needs
# logic a script does not have. Also catches stray whitespace before
# commas in code.
commas=$(git diff "$BASE_REF"...HEAD -- "${prose_paths[@]}" 2>/dev/null \
    | grep -nE '^\+[^+].* ,' | head -10)
if [ -n "$commas" ]; then
    printf '    %s\n' "$commas"
    warn "space-before-comma (' ,') in the PR's diff: mechanical em-dash"
    warn "      sweeps leave this (a comma splice, not prose). Replace"
    warn "      with a comma, a colon, or a period, chosen with logic."
else
    ok "no space-before-comma residue"
fi

# ---- 7. Doxygen docstring compliance on the PR's C files ----
# The hard gate of the ritual (AGENTS.md: every first-party function
# carries a doc block). Scoped to the C files this PR touches, not the
# tree, so it never charges a PR for a pre-existing violation.
# tests/ and examples/ are @brief-only by convention.
doc_full=()
doc_brief=()
while IFS= read -r f; do
    [ -f "$f" ] || continue
    case "$f" in
        *//third_party/*|*/third_party/*|*/vendor/*|build*/*) continue ;;
    esac
    case "$f" in
        tests/*|*/tests/*|*/test/*|examples/*|*/examples/*) doc_brief+=("$f") ;;
        *) doc_full+=("$f") ;;
    esac
done < <(git diff --name-only --diff-filter=ACMR "$BASE_REF"...HEAD -- '*.c' '*.h' 2>/dev/null)

if [ "${#doc_full[@]}" -eq 0 ] && [ "${#doc_brief[@]}" -eq 0 ]; then
    ok "no C files changed; docstring check not needed"
else
    doc_out=""
    doc_bad=0
    if [ "${#doc_full[@]}" -gt 0 ]; then
        doc_out=$(python3 scripts/doxygen_doc_check.py "${doc_full[@]}" 2>&1) || doc_bad=1
    fi
    if [ "${#doc_brief[@]}" -gt 0 ]; then
        brief_out=$(python3 scripts/doxygen_doc_check.py --brief-only "${doc_brief[@]}" 2>&1) || doc_bad=1
        doc_out="$doc_out${doc_out:+$'\n'}$brief_out"
    fi
    if [ "$doc_bad" -eq 0 ]; then
        ok "docstrings compliant (${#doc_full[@]} file(s), ${#doc_brief[@]} brief-only)"
    else
        printf '%s\n' "$doc_out" | head -40
        fail "docstring violations (above); fix and re-run"
        fail "      (python3 scripts/doxygen_doc_check.py <files>)"
    fi
fi

# ---- 8. comment and doc prose review ----
# scripts/prose-diff.sh only flags objective signals (block length,
# hardcoded counts); it cannot judge content, so it never fails on its
# own. --prose attests the output was read and acted on, the same trust
# model as --test below.
prose_out=$(bash "$repo_root/scripts/prose-diff.sh" 2>/dev/null)
prose_summary=$(printf '%s\n' "$prose_out" | grep '^SUMMARY:' || true)
prose_blocks=$(printf '%s' "$prose_summary" | sed -nE 's/.*comment_blocks=([0-9]+).*/\1/p')
prose_docs=$(printf '%s' "$prose_summary" | sed -nE 's/.*doc_files=([0-9]+).*/\1/p')
prose_blocks=${prose_blocks:-0}
prose_docs=${prose_docs:-0}
if [ "$prose_blocks" -eq 0 ] && [ "$prose_docs" -eq 0 ]; then
    ok "no added comments or markdown changes to review"
elif [ "$RUN_PROSE" -eq 1 ]; then
    ok "prose reviewed and attested ($prose_blocks comment block(s), $prose_docs doc file(s))"
else
    warn "$prose_blocks comment block(s), $prose_docs doc file(s) changed; read"
    warn "      scripts/prose-diff.sh output, judge each against AGENTS.md's"
    warn "      Expression Conventions, then re-run with --prose (make"
    warn "      pre-pr-check PROSE=1)"
fi

# ---- 9. hooks installed ----
hooks="$(git rev-parse --git-common-dir)/hooks"
missing=()
[ -x "$hooks/pre-commit" ] || missing+=(pre-commit)
[ -x "$hooks/commit-msg" ] || missing+=(commit-msg)
if [ "${#missing[@]}" -gt 0 ]; then
    warn "git hook(s) not installed (${missing[*]}); run: make setup-hooks"
else
    ok "git hooks installed"
fi

# ---- 10. suite (opt-in via --test) ----
if [ "$RUN_TESTS" -eq 1 ]; then
    say ""
    say "== running the host-sim suite (make test) =="
    if make test; then
        ok "host-sim suite passed"
    else
        fail "host-sim suite failed"
    fi
else
    warn "suite not run; re-run with --test (make pre-pr-check TEST=1) before merging"
fi

say ""
if [ "$FAILS" -gt 0 ]; then
    say "== $FAILS blocking item(s), $WARNS advisory; fix and re-run =="
    exit 1
fi
say "== ritual clean ($WARNS advisory); ready to open the PR =="
exit 0
