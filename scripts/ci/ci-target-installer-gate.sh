#!/usr/bin/env bash
# The release installer gate: install.sh must scaffold and build a working
# project for every family the manifest carries, derived rather than
# hand-listed so adding a family alone extends the gate (epic-hal#233).
# Runs on the bare runner (install.sh needs python3, which the toolchain
# image lacks); only the scaffolded build goes into the container.
#
# Usage: ci-target-installer-gate.sh <bundles-dir> <version>
#   env: EPIC_TOOLCHAIN_IMAGE, GITHUB_WORKSPACE (base for the scaffolds)

set -euo pipefail

bundles="${1:?usage: ci-target-installer-gate.sh <bundles-dir> <version>}"
version="${2:?usage: ci-target-installer-gate.sh <bundles-dir> <version>}"
base="${GITHUB_WORKSPACE:?GITHUB_WORKSPACE must be set}"
image="${EPIC_TOOLCHAIN_IMAGE:?EPIC_TOOLCHAIN_IMAGE must be set}"

fail=0

# Resolved from this script's own location, not the caller's cwd: the
# workflow runs it from the checkout root, but a local run may not.
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# --list must name exactly the families the manifest has: its output is
# derived here from parts.txt, which the build job generated from that
# same manifest. The size check keeps a truncated parts.txt from
# rubber-stamping an empty diff.
if [ ! -s "$bundles/parts.txt" ]; then
  echo "FAIL parts.txt is missing or empty"
  fail=1
elif ! diff <(awk '!seen[$2]++ { print $2 }' "$bundles/parts.txt") \
        <(EPIC_HAL_BASE_URL="file://$bundles" \
          sh "$repo_root/install.sh" --list "$version"); then
  echo "FAIL install --list"
  fail=1
fi

# emit_parts_map writes each family's variants in order, so the last line
# per family is variants[-1], the canonical part the PR gate builds
# (docs/adding-a-device.md); awk's last write per family gives one part
# each. Completeness is checked against the manifest, not against
# install.sh --list: --list reads this same parts.txt, so agreeing with it
# proves only self-consistency.
parts="$(awk '{ last[$2] = $1 } END { for (f in last) print last[f] }' \
  "$bundles/parts.txt" | sort)"
covered="$(awk '{ print $2 }' "$bundles/parts.txt" | sort -u | grep -c . || true)"
declared="$(python3 -c "
import sys
sys.path.insert(0, '$repo_root/scripts')
import epicmanifest
print(len(epicmanifest.load(epicmanifest.default_path()).families))
")"
echo "installer-gate: $covered family(families) in parts.txt, $declared in the manifest"
if [ "$covered" != "$declared" ]; then
  echo "FAIL parts.txt covers $covered family(families), the manifest declares $declared"
  fail=1
fi

# Part form: install.sh resolves each part to its family via parts.txt,
# which exercises that asset plus the family resolution end to end.
for part in $parts; do
  echo "=== install.sh $part ==="
  mkdir -p "$base/install-$part"
  # install.sh scaffolds in the current directory (in place), so run it
  # from the per-part dir to keep each scaffold isolated; the bundle is
  # vendored at ./epic-hal inside that dir and the scaffold's paths point
  # at it.
  if ! (
    cd "$base/install-$part"
    EPIC_HAL_BASE_URL="file://$bundles" \
    EPIC_HAL_DIR="$base/install-$part/epic-hal" \
      sh "$repo_root/install.sh" "$part" "$version" --with-xc8
  ) || ! docker run --rm -v "$base:/work" -w /work \
      "$image" \
      bash -c "cd /work/install-$part && make TOOLCHAIN=xc8 && test -f build/myapp.hex"; then
    echo "FAIL install-$part"
    fail=1
  fi
done

exit "$fail"
