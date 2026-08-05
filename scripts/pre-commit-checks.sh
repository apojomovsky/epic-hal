#!/usr/bin/env bash
# scripts/pre-commit-checks.sh, the actual pre-commit logic. Installed into
# .git/hooks/pre-commit by scripts/install-git-hooks.sh (see scripts/README.md
# for what each check does and why clang-format is not one of them yet).
#
# Every check here operates on staged content, not the working tree, except
# the newline/whitespace fixer, which edits the working tree file in place
# and then asks you to re-`git add` it (never silently changes what gets
# committed without you seeing it). If you staged only part of a file with
# `git add -p`, the fixer still touches the whole working-tree file; review
# `git diff` before re-adding in that case.
#
# CI reuses this same script (see .github/workflows/host-tests.yml) against
# an already-committed ref range instead of a staged index: a fresh CI
# checkout has nothing staged, everything is already committed. Set
# PRE_COMMIT_BASE_REF to the commit/ref to diff against (the PR base SHA, or
# the previous commit on a push); when unset, behavior is unchanged from the
# local staged-index hook.

set -u
fail=0

if [ -n "${PRE_COMMIT_BASE_REF:-}" ]; then
    diff_range=("$PRE_COMMIT_BASE_REF...HEAD")
    diff_cached=()
else
    diff_range=()
    diff_cached=(--cached)
fi

git_diff() {
    git diff "${diff_cached[@]}" "${diff_range[@]}" "$@"
}

staged_files() {
    git_diff --name-only --diff-filter=ACM
}

# ---- 1. trailing newline + trailing whitespace (auto-fix, then block) ----

newline_whitespace_check() {
    local touched=()
    local f
    while IFS= read -r f; do
        [ -f "$f" ] || continue
        # Skip binary files (git diff --numstat reports "-\t-\t..." for
        # binary).
        local numstat
        numstat="$(git_diff --numstat -- "$f")"
        case "$numstat" in
            -*) continue ;;
        esac
        local changed=0
        if [ -s "$f" ] && [ "$(tail -c1 "$f")" != "" ]; then
            printf '\n' >> "$f"
            changed=1
        fi
        if grep -qE '[[:blank:]]+$' "$f" 2>/dev/null; then
            sed -i -E 's/[[:blank:]]+$//' "$f"
            changed=1
        fi
        [ "$changed" = 1 ] && touched+=("$f")
    done < <(staged_files)

    if [ "${#touched[@]}" -gt 0 ]; then
        echo "pre-commit: fixed trailing newline/whitespace in:"
        printf '  %s\n' "${touched[@]}"
        echo "pre-commit: review with 'git diff', then 'git add' and commit again."
        fail=1
    fi
}

# ---- 2. no em-dashes in added lines (this repo's documented rule) ----
#
# Scoped to lines your commit actually adds, not the whole staged file, so
# pre-existing em-dashes elsewhere in a file you happen to touch don't block
# an unrelated commit (this repo has some from before the rule was adopted).

emdash_check() {
    local diff
    diff="$(git_diff -U0 --diff-filter=ACM -- \
        '*.c' '*.h' '*.md' 'CMakeLists.txt' '*/CMakeLists.txt' \
        'Makefile' '*/Makefile' '*/Makefile.*')"
    [ -z "$diff" ] && return 0

    local hits
    hits="$(awk '
        /^\+\+\+ / { file = substr($0, 7); next }
        /^@@/ {
            match($0, /\+[0-9]+/)
            newline = substr($0, RSTART + 1, RLENGTH - 1) + 0
            next
        }
        /^\+\+\+/ { next }
        /^\+/ {
            if (index($0, "—") > 0) {
                print file ":" newline ": " substr($0, 2)
            }
            newline++
            next
        }
    ' <<< "$diff")"

    if [ -n "$hits" ]; then
        echo "pre-commit: em-dash found in added lines (repo rule: no em-dashes,"
        echo "use a comma, colon, or period instead):"
        echo "$hits" | sed 's/^/  /'
        fail=1
    fi
}

# ---- 3. cppcheck on staged .c files (real static-analysis findings only) ----
#
# unusedFunction and missingInclude/missingIncludeSystem are suppressed:
# cppcheck analyzes one translation unit at a time here, so it cannot see
# that a library's public functions are called from other files (tests,
# examples, other modules) and would otherwise flag every public API
# function in every module as "unused". missingInclude is expected (system
# headers and cross-module headers are not always on the include path this
# script builds); it does not stop cppcheck from analyzing the function
# bodies it can see.

cppcheck_check() {
    command -v cppcheck >/dev/null 2>&1 || {
        echo "pre-commit: cppcheck not installed, skipping static analysis."
        return 0
    }

    local c_files=()
    local f
    while IFS= read -r f; do
        [[ "$f" == *.c ]] && [ -f "$f" ] && c_files+=("$f")
    done < <(staged_files)
    [ "${#c_files[@]}" -eq 0 ] && return 0

    local includes=()
    while IFS= read -r d; do
        includes+=(-I "$d")
    done < <(find . -type d \( -name include -o -path '*/include/host' -o -path '*/include/target' \) -not -path '*/build/*' 2>/dev/null)

    # --suppress=preprocessorErrorDirective: a #error in #ifndef <build-define>
    # is a common vendored pattern (e.g. m-stack's mmc.h requires the
    # integrator to define MMC_SPI_TRANSFER) and is a false positive here,
    # because this hook passes only -I dirs, no -D build defines (the real
    # CMake/Make build defines them; host-sim + xc8 are the source of truth).
    # Real "unsupported platform" #errors are caught by the actual build.
    if ! cppcheck --enable=warning,performance,portability --std=c99 --error-exitcode=1 \
        --suppress=missingInclude --suppress=missingIncludeSystem \
        --suppress=unmatchedSuppression \
        --suppress=preprocessorErrorDirective \
        --quiet "${includes[@]}" "${c_files[@]}"; then
        echo "pre-commit: cppcheck found issues in the files above."
        fail=1
    fi
}

newline_whitespace_check
emdash_check
cppcheck_check

if [ "$fail" -ne 0 ]; then
    echo "pre-commit: blocked, see messages above."
    exit 1
fi
exit 0
