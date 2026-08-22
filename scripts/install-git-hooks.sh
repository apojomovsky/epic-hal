#!/usr/bin/env bash
# One-time setup, called by scripts/bootstrap.sh (or by hand after a clone):
# symlinks the repo's hook scripts into the git hooks dir (that dir is not
# tracked by git, so this is needed once per clone). See scripts/README.md
# for what each hook checks.
#
# Worktree-aware: hooks live in the shared common dir, so installing from
# any worktree under .worktrees/ installs them for every worktree. The
# symlinks target the main checkout, which outlives the worktrees; a
# relative ../../scripts/ link would dangle there anyway, since a linked
# worktree's .git is a file, not a directory.

set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"

here="$(cd .. && pwd)"
common_dir="$(cd "$(git rev-parse --git-common-dir)" && pwd)"
main_root="$(cd "$common_dir/.." && pwd)"
hooks="$common_dir/hooks"

mkdir -p "$hooks"
for hook in pre-commit commit-msg; do
    script="$main_root/scripts/$hook-checks.sh"
    # A hook script that exists only on this branch is not in the main
    # checkout yet; link the invoking tree's copy so the hook works now,
    # and re-run after merging to repoint it at the durable location.
    if [ ! -f "$script" ]; then
        script="$here/scripts/$hook-checks.sh"
        echo "Note: $hook-checks.sh not in the main checkout yet, linking $here"
    fi
    chmod +x "$script"
    ln -sf "$script" "$hooks/$hook"
    echo "Installed: $hooks/$hook -> $script"
done
