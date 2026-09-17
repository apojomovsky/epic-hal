#!/usr/bin/env bash
# The pre-commit checks: trailing whitespace/newline, em-dashes and
# Allman-brace style on added lines, cppcheck on staged .c files, stray
# root-level files. Installed into .git/hooks/pre-commit by
# scripts/install-git-hooks.sh; see scripts/README.md for what each check
# does and why. Operates on staged content, except the whitespace fixer,
# which edits the working-tree file in place and asks you to re-`git add`
# it. CI reuses this script against PRE_COMMIT_BASE_REF; unset = index.

set -u
fail=0

# Resolve this script's real location: the pre-commit hook is a symlink
# into .git/hooks, so $0/dirname would point into .git/hooks, not scripts/.
SCRIPT_DIR="$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]:-$0}")")" && pwd)"

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

# ---- 1a. unexpected files and directories at the repo root ----
#
# Two incident-driven rules:
# (a) 2026-08-10: 11 XC8 codegen-probe byproducts (root-level
#     *.d/*.p1/*.sdb) were swept into PR #12/#13 by a blanket
#     `git add -A`. Any staged XC8-byproduct extension (*.p1, *.sdb,
#     or a root-level *.d) fails, as does any staged root-level file
#     outside the file whitelist.
# (b) 2026-08-11: the 12 epic-combo-* test modules lived at the root
#     and were moved under tests/. The root is reserved for modules
#     (epic-*), the three HALs, and the known infra directories
#     (dir_whitelist below); a staged file whose top-level directory is
#     outside it fails the commit, so new test scaffolding or a stray
#     directory cannot pollute the root.
# Build output under build*/ is already gitignored, so it never
# reaches the staged set.

stray_files_check() {
    local file_whitelist='^\.gitignore$|^\.clang-format$|^pyproject\.toml$|^AGENTS\.md$|^CLAUDE\.md$|^DEVELOPMENT\.md$|^LICENSE$|^Makefile$|^README\.md$|^install\.sh$|^CHANGELOG\.md$|^cliff\.toml$'
    # Top-level directories: modules (epic-*), the HALs (pic*-hal), the
    # shared ISA cores (pic*-core, epic-hal#136 naming decision), and the
    # known infra directories. Anything else at the root is a stray
    # (probe output, a dropped directory, an unplanned module): the
    # 2026-08-11 cleanup moved the 12 epic-combo-* test modules under
    # tests/ precisely so the root stays readable. A new module is
    # added by extending this list deliberately, not by accident.
    local dir_whitelist='^epic-[a-z0-9-]+$|^pic16f87xa-hal$|^pic18fxx5x-hal$|^pic16f193x-hal$|^pic16f88x-hal$|^pic16f628a-hal$|^pic16f83_84-hal$|^pic16f818_819-hal$|^pic16f63x_67x_68x-hal$|^pic18f1320-hal$|^pic18f2520-hal$|^pic18f6520-hal$|^pic16f7x-hal$|^pic16f5x-hal$|^pic[0-9a-z]+-midrange-core$|^pic[0-9a-z]+-enhanced-core$|^pic-baseline-core$|^pic18-core$|^docs$|^scripts$|^docker$|^examples$|^tests$|^\.github$'
    local bad=0
    local f
    while IFS= read -r f; do
        case "$f" in
            */*) ;;
            *)  if ! printf '%s' "$f" | grep -qE "$file_whitelist"; then
                    echo "pre-commit: unexpected file at the repo root: $f"
                    echo "pre-commit:   remove it, move it under a directory, or add it to .gitignore"
                    bad=1
                fi
                ;;
        esac
        case "$f" in
            */*)
                top="${f%%/*}"
                if ! printf '%s' "$top" | grep -qE "$dir_whitelist"; then
                    echo "pre-commit: unexpected top-level directory: $top"
                    echo "pre-commit:   the repo root is reserved for modules, the HALs, and the known"
                    echo "pre-commit:   infra directories; put test scaffolding under tests/ instead"
                    bad=1
                fi
                ;;
        esac
        case "$f" in
            *.p1|*.sdb)
                echo "pre-commit: XC8 byproduct committed: $f"
                echo "pre-commit:   these are regenerable compiler output; remove them"
                bad=1
                ;;
            *.d)
                if [ "${f%/*}" = "$f" ]; then
                    echo "pre-commit: root-level compiler dependency file: $f"
                    echo "pre-commit:   regenerable build output; remove it"
                    bad=1
                fi
                ;;
        esac
    done < <(staged_files)
    [ "$bad" = 1 ] && fail=1
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

# ---- 3. Allman brace style on added lines ----
#
# Repo brace style is Allman: an opening brace owns its line, for function
# bodies and control statements alike. Scoped like the em-dash rule (added
# lines only, PRE_COMMIT_BASE_REF respects) so pre-existing style in files
# an unrelated commit touches never blocks it; the standalone checker
# scripts/brace-style-check.sh holds the implementation and the rationale.

brace_style_check() {
    # Forward the CI base ref exactly like git_diff/emdash_check: in CI
    # the index equals HEAD (actions/checkout), so --cached is empty and
    # the gate would silently no-op without the base. Local hook: empty
    # arg -> checker scans the staged index.
    if ! bash "${SCRIPT_DIR}/brace-style-check.sh" "${PRE_COMMIT_BASE_REF:-}"; then
        fail=1
    fi
}

# ---- 4. cppcheck on staged .c files (real static-analysis findings only) ----
#
# unusedFunction and missingInclude/missingIncludeSystem are suppressed:
# cppcheck analyzes one translation unit at a time here, so it cannot see
# that a library's public functions are called from other files (tests,
# examples, other modules) and would otherwise flag every public API
# function in every module as "unused". missingInclude is expected (system
# headers and cross-module headers are not always on the include path this
# script builds); it does not stop cppcheck from analyzing the function
# bodies it can see.
#
# nullPointerRedundantCheck and the third-party syntaxError are suppressed
# because cppcheck 2.19+ (newer than CI's 2.13) reports them as false
# positives: the CHECK(ptr != NULL) macro pattern trips the redundant-null
# check, and the vendored m-stack mmc.h #error pattern that 2.13 classified
# as preprocessorErrorDirective (suppressed below) is reclassified as
# syntaxError, always inside third_party/ paths this repo does not own.
#
# -D'__at(x)=': XC8's __at(addr) placement attribute (direct in epic-math's
# pic16 backends, via EPIC_PLACE/..._PLACE macros in the target platform
# headers elsewhere) is not C99; cppcheck 2.19 fails on it, cppcheck 2.13
# did not. The hook analyzes code the way the host build compiles it, and
# the host build never places anything, so the empty define is the correct
# host semantics, not a lie to the analyzer.
#
# The include list stays sorted inside each group so a HAL's
# include/host precedes its include/target: header resolution then
# matches the host build (target headers carry the XC8 placement macros
# the host never sees), which keeps EPIC_PLACE-style macros from
# expanding to __at inside cppcheck.

# Families whose register map the non-family partitions analyze against.
# Deterministic picks, not preferences: analysis against any single
# family is equivalent for logic bugs, while the per-family truth stays
# with the builds. Constants, so a rename updates one place.
CPPCHECK_MIDRANGE_REF="pic16f628a-hal"
CPPCHECK_CONSUMER_REF="pic16f193x-hal"

# Print the include dirs (one per line) for one staged top-level group.
# A family group sees only its own headers plus the shared layer, so a
# basename shared across families (pic14_midrange.h, epic_hal.h, ...)
# cannot resolve to another die. The shared midrange drivers (pic14_*.h)
# and epic-common headers have unique basenames repo-wide, so adding
# them never shadows a family header. Caller keeps only the directories
# that exist.
group_includes() {
    local group="$1"
    case "$group" in
        pic*-hal|pic14-midrange-core)
            find "$group" -type d \( -name include -o -path '*/include/host' -o -path '*/include/target' \) -not -path '*/build/*' 2>/dev/null | sort
            printf '%s\n' "epic-common/include" "pic14-midrange-core/include"
            ;;
        *)
            # consumer modules, tests, examples: module headers plus one
            # family's surface for epic_hal.h, chosen deterministically.
            # Headers a file pulls in only under a per-family #if guard
            # resolve nowhere without a family define (suppressed, as
            # before for genuinely missing headers); the analyzable
            # surface is the family-agnostic logic, whose per-family
            # truth stays with the builds.
            find epic-* tests -type d \( -name include -o -path '*/include/host' -o -path '*/include/target' \) -not -path '*/build/*' 2>/dev/null | sort
            printf '%s\n' "epic-common/include"
            find "$CPPCHECK_CONSUMER_REF" -type d \( -name include -o -path '*/include/host' -o -path '*/include/target' \) -not -path '*/build/*' 2>/dev/null | sort
            ;;
    esac
    if [ "$group" = "pic14-midrange-core" ]; then
        # midrange-core's own drivers include the family-blind
        # pic14_midrange.h, which only families provide: analyze them
        # against one family's map, deterministically.
        find "$CPPCHECK_MIDRANGE_REF" -type d \( -name include -o -path '*/include/host' -o -path '*/include/target' \) -not -path '*/build/*' 2>/dev/null | sort
    fi
}

cppcheck_check() {
    command -v cppcheck >/dev/null 2>&1 || {
        echo "pre-commit: cppcheck not installed, skipping static analysis."
        return 0
    }

    # Group the staged .c files by top-level directory: one invocation
    # per group with that group's own include set, instead of every
    # file against every family's headers at once.
    local groups
    groups="$(while IFS= read -r f; do
        [[ "$f" == *.c ]] && [ -f "$f" ] || continue
        printf '%s\n' "${f%%/*}"
    done < <(staged_files) | sort -u)"
    [ -z "$groups" ] && return 0

    local group
    while IFS= read -r group; do
        [ -z "$group" ] && continue
        local c_files=()
        local f
        while IFS= read -r f; do
            [[ "$f" == *.c ]] && [ -f "$f" ] && [ "${f%%/*}" = "$group" ] || continue
            c_files+=("$f")
        done < <(staged_files)
        [ "${#c_files[@]}" -eq 0 ] && continue
        local includes=()
        local d
        # sort -u drops the dirs group_includes emits twice (its find
        # hits overlap the explicit shared dirs); the order stays
        # sorted, so host still precedes target.
        while IFS= read -r d; do
            [ -d "$d" ] && includes+=(-I "$d")
        done < <(group_includes "$group" | sort -u)
        [ "${#includes[@]}" -eq 0 ] && continue

    # --suppress=preprocessorErrorDirective: a #error in #ifndef <build-define>
    # is a common vendored pattern (e.g. m-stack's mmc.h requires the
    # integrator to define MMC_SPI_TRANSFER) and is a false positive here,
    # because this hook passes only -I dirs, no -D build defines (the real
    # CMake/Make build defines them; host-sim + xc8 are the source of truth).
    # Real "unsupported platform" #errors are caught by the actual build.
        local out
        if ! out="$(cppcheck --enable=warning,performance,portability --std=c99 --error-exitcode=1 \
            --suppress=missingInclude --suppress=missingIncludeSystem \
            --suppress=unmatchedSuppression \
            --suppress=preprocessorErrorDirective \
            --suppress=nullPointerRedundantCheck \
            --suppress=syntaxError:*/third_party/* \
            -D'__at(x)=' \
            --quiet "${includes[@]}" "${c_files[@]}" 2>&1)"; then
            [ -n "$out" ] && printf '%s\n' "$out"
            case "$out" in
                *"Failed to load library configuration"*|*"installation is broken"*|*"cannot open shared object file"*)
                    echo "pre-commit: cppcheck cannot run (broken install; no findings reported)."
                    echo "pre-commit:   fix the environment (make bootstrap installs cppcheck), then commit again."
                    ;;
                *)
                    echo "pre-commit: cppcheck found issues in the files above."
                    ;;
            esac
            fail=1
        fi
    done <<<"$groups"
}

newline_whitespace_check
emdash_check
brace_style_check
cppcheck_check
stray_files_check

if [ "$fail" -ne 0 ]; then
    echo "pre-commit: blocked, see messages above."
    exit 1
fi
exit 0
