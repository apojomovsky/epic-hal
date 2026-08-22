#!/usr/bin/env bash
# Diff-scoped prose surface for the takeoff ritual's comment/doc review
# step (AGENTS.md): every added comment block and markdown hunk from
# BASE_REF...HEAD, in full, so the reviewer doesn't have to re-derive the
# diff. It flags block length and hardcoded counts as hints only;
# guessing "AI leftover" phrasing is a losing bet across the models this
# repo sees, so content judgment stays the reviewer's job, every block,
# flagged or not.
#
# Usage: bash scripts/prose-diff.sh
#   BASE_REF overrides the base branch (forks: BASE_REF=<fork>/master)

set -uo pipefail

BASE_REF="${BASE_REF:-origin/master}"

# AGENTS.md caps a comment block at ~8 lines; longer needs a real
# justification (a hand-trace, a race proof), which the hint asks for.
BLOCK_HINT_LINES=8

comment_blocks=0
doc_files=0

# Which comment syntax a file uses. Empty = not scanned for comments
# (markdown is handled separately below, everything else has no comment
# surface worth reviewing).
comment_style_for_file() {
    case "$1" in
        *.c|*.h)                                 printf '%s' 'c' ;;
        *.py|*.sh|*.toml|*.yml|*.yaml|*.cmake|*.mk) printf '%s' 'hash' ;;
        */Makefile|Makefile|*/Makefile.*)        printf '%s' 'hash' ;;
        */CMakeLists.txt|CMakeLists.txt)         printf '%s' 'hash' ;;
        */Dockerfile*|Dockerfile*)               printf '%s' 'hash' ;;
        *)                                       printf '' ;;
    esac
}

files=$(git diff --name-only --diff-filter=ACMR "$BASE_REF"...HEAD 2>/dev/null \
    | grep -vE '(^|/)(third_party|vendor|build[^/]*|\.worktrees)/' \
    | grep -v '^docs/superpowers/plans/' || true)

echo "== Comments added in $BASE_REF...HEAD =="
echo ""

for f in $files; do
    style=$(comment_style_for_file "$f")
    [ -z "$style" ] && continue
    [ -f "$f" ] || continue

    newline=0
    block_start=0
    block_content=""
    block_len=0
    in_c_block=0

    flush() {
        if [ "$block_len" -gt 0 ]; then
            comment_blocks=$((comment_blocks + 1))
            end=$((block_start + block_len - 1))
            tag=""
            [ "$block_len" -gt "$BLOCK_HINT_LINES" ] && \
                tag="  [hint: >$BLOCK_HINT_LINES lines, confirm it's justified]"
            echo "  $f:$block_start-$end ($block_len line(s))$tag"
            printf '%s' "$block_content" | sed 's/^/    /'
            echo ""
        fi
        block_start=0
        block_content=""
        block_len=0
        in_c_block=0
    }

    # Is this added line part of a comment? For C the answer is stateful:
    # a `*` continuation only counts inside an open /* block, otherwise
    # `*ptr = x;` would read as prose.
    is_comment_line() {
        local trimmed="$1"
        case "$style" in
            hash)
                case "$trimmed" in
                    '#!'*) return 1 ;;
                    '#'*)  return 0 ;;
                    *)     return 1 ;;
                esac
                ;;
            c)
                if [ "$in_c_block" = 1 ]; then
                    case "$trimmed" in
                        *'*/'*) in_c_block=0 ;;
                    esac
                    return 0
                fi
                case "$trimmed" in
                    '//'*) return 0 ;;
                    '/*'*)
                        case "$trimmed" in
                            *'*/'*) ;;
                            *) in_c_block=1 ;;
                        esac
                        return 0
                        ;;
                    *) return 1 ;;
                esac
                ;;
        esac
        return 1
    }

    while IFS= read -r line; do
        case "$line" in
            @@*)
                flush
                newline=$(printf '%s' "$line" | sed -nE 's/^@@ -[0-9]+(,[0-9]+)? \+([0-9]+)(,[0-9]+)? @@.*/\2/p')
                ;;
            +++*) ;;
            +*)
                content="${line#+}"
                trimmed="${content#"${content%%[![:space:]]*}"}"
                if is_comment_line "$trimmed"; then
                    [ "$block_len" -eq 0 ] && block_start=$newline
                    block_content="${block_content}${content}"$'\n'
                    block_len=$((block_len + 1))
                else
                    flush
                fi
                newline=$((newline + 1))
                ;;
            -*) ;;
            *) ;;
        esac
    done < <(git diff -U0 "$BASE_REF"...HEAD -- "$f" 2>/dev/null)
    flush
done

echo "== Markdown diffs added/modified in $BASE_REF...HEAD =="
echo ""

md_files=$(printf '%s\n' "$files" | grep -E '\.md$' || true)
for f in $md_files; do
    [ -f "$f" ] || continue
    doc_files=$((doc_files + 1))
    echo "  -- $f --"
    hunk=$(git diff "$BASE_REF"...HEAD -- "$f" 2>/dev/null)
    printf '%s\n' "$hunk" | sed 's/^/    /'
    # Volatile facts (AGENTS.md docs lifecycle): a count or a pasted tree
    # goes stale on the next merge; describe the mechanism instead.
    if printf '%s' "$hunk" | grep -qE '^\+.*[0-9]+ +(tests?|files?|modules?|families|devices?|examples?|lines? of code)\b' \
       || printf '%s' "$hunk" | grep -qE '^\+.*(├──|└──)'; then
        echo "    [hint: possible coupling to a volatile fact (count/tree); confirm it won't go stale]"
    fi
    # Datasheet PDFs are gitignored and must never be referenced by a
    # local path; link Microchip's hosted copy instead.
    if printf '%s' "$hunk" | grep -qE '^\+.*\]\([^)]*\.pdf\)' && \
       ! printf '%s' "$hunk" | grep -qE '^\+.*\]\(https?://[^)]*\.pdf\)'; then
        echo "    [hint: local .pdf link; datasheets are not committed, link Microchip's hosted copy]"
    fi
    echo ""
done

echo "SUMMARY: comment_blocks=$comment_blocks doc_files=$doc_files"
exit 0
