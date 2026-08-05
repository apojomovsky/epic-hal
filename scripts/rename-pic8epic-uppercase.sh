#!/usr/bin/env bash
# Driver for the PIC8_ -> EPIC_ uppercase rename
# (docs/pic8-epic-uppercase-rename-plan.md). Two phases:
#   A. content substitution (scripts/rename-pic8epic-uppercase.awk):
#      PIC8_EPIC_ -> EPIC_ (collapse), then PIC8_ -> EPIC_ (the rest),
#      on every tracked file except the excluded rename-meta files.
#   B. targeted fixups the gsub structurally cannot catch:
#      B1. include-guard collisions: the HAL_ pass left the hal_* shim/
#          family headers with guards EPIC_IRQ_H / EPIC_LCD_H; this pass
#          renamed the common/module epic_irq.h / epic_lcd.h guards
#          (PIC8_IRQ_H / PIC8_LCD_H) to the same EPIC_IRQ_H / EPIC_LCD_H,
#          colliding. Disambiguate by pushing the hal_* guards to
#          EPIC_HAL_IRQ_H / EPIC_HAL_LCD_H (matches their hal_ filenames;
#          the common/module epic_* headers keep EPIC_IRQ_H / EPIC_LCD_H).
#      B2. magic-string dispatch: pic16f193x_harness_sim_target.c compares
#          the log format string char-by-char against "PIC8_HARNESS_RESULT
#          ..." to drive RA0. The gsub renamed the string literal (in
#          epic_harness.h's epic_harness_report) to "EPIC_HARNESS_RESULT
#          ..." but could NOT rename this comparison: 'P','I','C','8' is
#          split across char literals (no contiguous PIC8_ token), so the
#          sender emits "EPIC_..." while the receiver still checks
#          "PIC8_..." and never drives RA0 -> the PIC16F193X mdb gate
#          silently fails. Update chars 0-3 to 'E','P','I','C' (both the
#          PASS and FAIL branches; chars 4+ are unchanged).
# Content-only: no filename contains uppercase PIC8_, so no git mv.
#
# Usage: scripts/rename-pic8epic-uppercase.sh [--dry-run|--apply]
#   --dry-run (default): print Phase A diffs per changed file, the planned
#     Phase B edits, and a summary. Writes nothing.
#   --apply: write Phase A content in place (temp + mv, preserving mode),
#     then run the Phase B fixups.
#
# Must be run from the repo root.

set -euo pipefail

mode="dry-run"
if [ "${1:-}" = "--apply" ]; then
  mode="apply"
elif [ -n "${1:-}" ] && [ "${1:-}" != "--dry-run" ]; then
  echo "usage: $0 [--dry-run|--apply]" >&2
  exit 1
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
awk_script="$script_dir/rename-pic8epic-uppercase.awk"

if [ ! -f "$awk_script" ]; then
  echo "error: expected $awk_script to exist" >&2
  exit 1
fi

# Rename-meta files excluded from the Phase A content pass: they
# deliberately encode old names as data/logic or historical record.
exclude_re='^(docs/pic8-epic-uppercase-rename-plan\.md|scripts/rename-pic8epic-uppercase\.sh|scripts/rename-pic8epic-uppercase\.awk|docs/pic8-epic-rename-plan\.md|scripts/rename-pic8-epic\.sh|scripts/rename-pic8-epic\.awk|scripts/pic8-epic-modules\.txt|docs/hal-epic-rename-plan\.md|scripts/rename-hal-epic\.sh|scripts/rename-hal-epic\.awk|scripts/hal-epic-exceptions\.txt)$'

echo "=== Phase A: content substitution ($mode) ==="
files_changed=0
while IFS= read -r -d '' file; do
  if [[ "$file" =~ $exclude_re ]]; then
    continue
  fi
  if ! grep -q 'PIC8_' -- "$file" 2>/dev/null; then
    continue
  fi

  tmp="$(mktemp)"
  awk -f "$awk_script" -- "$file" > "$tmp"

  if [ -s "$file" ] && [ -z "$(tail -c1 -- "$file")" ]; then
    :
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
echo "=== Phase B: targeted fixups ($mode) ==="
fixups=0

# B1: guard collisions. hal_irq.h (3 families) and hal_lcd.h (PIC16F193X).
for f in pic16f87xa-hal/include/core/hal_irq.h \
         pic18fxx5x-hal/include/core/hal_irq.h \
         pic16f193x-hal/include/core/hal_irq.h; do
  if [ -f "$f" ] && grep -qE '\bEPIC_IRQ_H\b' "$f"; then
    if [ "$mode" = "dry-run" ]; then
      echo "guard fix: $f: EPIC_IRQ_H -> EPIC_HAL_IRQ_H"
    else
      perl -i -pe 's/\bEPIC_IRQ_H\b/EPIC_HAL_IRQ_H/g' "$f"
    fi
    fixups=$((fixups + 1))
  fi
done
f=pic16f193x-hal/include/peripherals/hal_lcd.h
if [ -f "$f" ] && grep -qE '\bEPIC_LCD_H\b' "$f"; then
  if [ "$mode" = "dry-run" ]; then
    echo "guard fix: $f: EPIC_LCD_H -> EPIC_HAL_LCD_H"
  else
    perl -i -pe 's/\bEPIC_LCD_H\b/EPIC_HAL_LCD_H/g' "$f"
  fi
  fixups=$((fixups + 1))
fi

# B2: magic-string dispatch in pic16f193x_harness_sim_target.c. Update the
# char-by-char prefix from 'P','I','C','8' to 'E','P','I','C' in both the
# PASS and FAIL branches (chars 4+ unchanged). Runs after Phase A, which
# left this comparison untouched (no contiguous PIC8_ token).
f=pic16f193x-hal/src/core/pic16f193x_harness_sim_target.c
if [ -f "$f" ] && grep -q "fmt\[0\] == 'P' && fmt\[1\] == 'I' && fmt\[2\] == 'C'" "$f"; then
  if [ "$mode" = "dry-run" ]; then
    echo "magic-string fix: $f: char-by-char prefix 'P','I','C','8' -> 'E','P','I','C' (both branches)"
  else
    perl -i -0777 -pe "s/fmt\[0\] == 'P' && fmt\[1\] == 'I' && fmt\[2\] == 'C' &&(\s+)fmt\[3\] == '8'/fmt[0] == 'E' && fmt[1] == 'P' && fmt[2] == 'I' &&\${1}fmt[3] == 'C'/g" "$f"
  fi
  fixups=$((fixups + 1))
else
  # Already fixed (idempotent re-run): detect the post-fix form so --apply
  # is a no-op rather than silently doing nothing.
  if [ -f "$f" ] && grep -q "fmt\[0\] == 'E' && fmt\[1\] == 'P' && fmt\[2\] == 'I'" "$f"; then
    :
  else
    echo "warn: $f not found or magic-string comparison not in expected form" >&2
  fi
fi
echo "Phase B: fixups applied: $fixups"

echo
echo "----------------------------------------"
echo "mode: $mode"
echo "Phase A content-changed files: $files_changed"
echo "Phase B fixups: $fixups"
