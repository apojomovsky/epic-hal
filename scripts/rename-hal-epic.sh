#!/usr/bin/env bash
# Driver for the HAL_ -> EPIC_ rename (docs/hal-epic-rename-plan.md).
# Enumerates input files via `git ls-files` (repo-relative paths,
# automatically respects .gitignore, so build*/ and
# docker/ci-toolchain/vendor/ are excluded with no extra path-exclude
# list needed), then runs each file that contains "HAL_" through
# scripts/rename-hal-epic.awk.
#
# Usage: scripts/rename-hal-epic.sh [--dry-run|--apply]
#   --dry-run (default): print a unified diff per changed file, plus a
#     summary count. Writes nothing.
#   --apply: write the transformed content back in place (temp file +
#     mv, preserving the original file's mode).
#
# Must be run from the repo root (relies on `git ls-files` and the
# exceptions file's paths being repo-relative).

set -euo pipefail

mode="dry-run"
if [ "${1:-}" = "--apply" ]; then
  mode="apply"
elif [ -n "${1:-}" ] && [ "${1:-}" != "--dry-run" ]; then
  echo "usage: $0 [--dry-run|--apply]" >&2
  exit 1
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
awk_script="$script_dir/rename-hal-epic.awk"
exceptions="$script_dir/hal-epic-exceptions.txt"

if [ ! -f "$awk_script" ] || [ ! -f "$exceptions" ]; then
  echo "error: expected both $awk_script and $exceptions to exist" >&2
  exit 1
fi

changed=0
files_changed=0

while IFS= read -r -d '' file; do
  # Cheap pre-filter: skip files with no HAL_ at all, no point paying
  # for an awk pass + diff on the other ~90% of the tree.
  if ! grep -q 'HAL_' -- "$file" 2>/dev/null; then
    continue
  fi

  tmp="$(mktemp)"
  awk -v EXC="$exceptions" -f "$awk_script" -- "$file" > "$tmp"

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
    this_count=$(diff "$file" "$tmp" | grep -c '^[<>]' || true)
    changed=$((changed + this_count))
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

echo "----------------------------------------"
echo "mode: $mode"
echo "files with changes: $files_changed"
echo "changed lines (diff +/- count, not token count): $changed"
