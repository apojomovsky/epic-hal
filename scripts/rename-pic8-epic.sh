#!/usr/bin/env bash
# Driver for the pic8_ / pic8- -> epic_ / epic- rename
# (docs/pic8-epic-rename-plan.md). Three phases:
#   A. content substitution (scripts/rename-pic8-epic.awk) on every
#      tracked file, except the 4 self-files that encode old names as
#      data/logic.
#   B. file `git mv`: every tracked file whose basename contains pic8_
#      (41 files, incl. the 3 pic8_hal.h) -> epic_ basename, plus the 6
#      docs/pic8-<module>-plan.md -> docs/epic-<module>-plan.md.
#   C. dir `git mv`: the 17 pic8-<name>/ module dirs -> epic-<name>/.
# Apply order is A -> B -> C (content first, then file moves, then dir
# moves, so the pre-computed file list at old paths stays valid).
#
# Usage: scripts/rename-pic8-epic.sh [--dry-run|--apply]
#   --dry-run (default): print per-file content diffs plus the planned
#     git mv commands. Writes nothing.
#   --apply: write transformed content in place (temp + mv, preserving
#     mode), then run the git mv commands.
#
# Must be run from the repo root (relies on `git ls-files` and the
# module list's paths being repo-relative).

set -euo pipefail

mode="dry-run"
if [ "${1:-}" = "--apply" ]; then
  mode="apply"
elif [ -n "${1:-}" ] && [ "${1:-}" != "--dry-run" ]; then
  echo "usage: $0 [--dry-run|--apply]" >&2
  exit 1
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
awk_script="$script_dir/rename-pic8-epic.awk"
mods="$script_dir/pic8-epic-modules.txt"

if [ ! -f "$awk_script" ] || [ ! -f "$mods" ]; then
  echo "error: expected both $awk_script and $mods to exist" >&2
  exit 1
fi

# Files excluded from the content pass: they deliberately encode old
# names as data/logic and must not self-edit.
exclude_re='^(scripts/rename-pic8-epic\.sh|scripts/rename-pic8-epic\.awk|scripts/pic8-epic-modules\.txt|docs/pic8-epic-rename-plan\.md)$'

# Load the 17 module names into a bash array for phases B (plan-doc
# renames) and C (dir renames).
mapfile -t MODULES < <(grep -vE '^(#|$)' "$mods")

echo "=== Phase A: content substitution ($mode) ==="
files_changed=0
while IFS= read -r -d '' file; do
  # Skip the self-files.
  if [[ "$file" =~ $exclude_re ]]; then
    continue
  fi
  # Cheap pre-filter: skip files with no lowercase pic8 at all (catches
  # pic8_, pic8-, and the github.com/.../pic8-hal URL; skips PIC8_-only
  # files). Case-sensitive.
  if ! grep -q 'pic8' -- "$file" 2>/dev/null; then
    continue
  fi

  tmp="$(mktemp)"
  awk -v MODS="$mods" -f "$awk_script" -- "$file" > "$tmp"

  # awk's `print` always appends a trailing newline, even for the last
  # record; if the original file had no trailing newline, undo that so
  # the rename doesn't bundle in an unrelated EOF-newline change.
  if [ -s "$file" ] && [ -z "$(tail -c1 -- "$file")" ]; then
    : # original ends in a newline, nothing to fix
  elif [ -s "$file" ]; then
    truncate -s -1 "$tmp"
  fi

  if ! cmp -s "$file" "$tmp"; then
    files_changed=$((files_changed + 1))
    if [ "$mode" = "dry-run" ]; then
      echo "=== $file ==="
      diff -u "$file" "$tmp" || true
      echo
    else
      mode_bits=$(stat -c '%a' "$file")
      mv "$tmp" "$file"
      chmod "$mode_bits" "$file"
    fi
  fi

  [ "$mode" = "dry-run" ] && rm -f "$tmp"
done < <(git ls-files -z)
echo "Phase A: files with content changes: $files_changed"

echo
echo "=== Phase B: file git mv ($mode) ==="
file_moves=0
while IFS= read -r -d '' file; do
  base="${file##*/}"
  dir="${file%$base}"
  newbase=""
  if [[ "$base" == *pic8_* ]]; then
    newbase="${base//pic8_/epic_}"
  else
    # docs/pic8-<module>-plan.md -> docs/epic-<module>-plan.md for the
    # 6 existing plan docs (only for real modules; pic8-vga-plan.md is
    # not a module and is left alone).
    for m in "${MODULES[@]}"; do
      if [ "$base" = "pic8-${m}-plan.md" ]; then
        newbase="epic-${m}-plan.md"
        break
      fi
    done
  fi
  if [ -z "$newbase" ] || [ "$newbase" = "$base" ]; then
    continue
  fi
  newpath="${dir}${newbase}"
  if [ "$mode" = "dry-run" ]; then
    echo "git mv -- \"$file\" \"$newpath\""
  else
    git mv -- "$file" "$newpath"
  fi
  file_moves=$((file_moves + 1))
done < <(git ls-files -z)
echo "Phase B: file moves: $file_moves"

echo
echo "=== Phase C: dir git mv ($mode) ==="
dir_moves=0
for m in "${MODULES[@]}"; do
  old="pic8-${m}"
  new="epic-${m}"
  if [ -d "$old" ]; then
    if [ "$mode" = "dry-run" ]; then
      echo "git mv -- \"$old\" \"$new\""
    else
      git mv -- "$old" "$new"
    fi
    dir_moves=$((dir_moves + 1))
  fi
done
echo "Phase C: dir moves: $dir_moves"

echo
echo "----------------------------------------"
echo "mode: $mode"
echo "Phase A content-changed files: $files_changed"
echo "Phase B file moves: $file_moves"
echo "Phase C dir moves: $dir_moves"
