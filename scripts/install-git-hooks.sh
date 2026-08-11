#!/usr/bin/env bash
# One-time setup, called by scripts/bootstrap.sh (or by hand after a clone):
# symlinks scripts/pre-commit-checks.sh into .git/hooks/pre-commit (that
# dir is not tracked by git, so this is needed once per clone). See
# scripts/README.md for what the hook checks.

set -euo pipefail

repo_root="$(git -C "$(dirname "${BASH_SOURCE[0]}")" rev-parse --show-toplevel)"
hook="$repo_root/.git/hooks/pre-commit"

ln -sf ../../scripts/pre-commit-checks.sh "$hook"
chmod +x "$repo_root/scripts/pre-commit-checks.sh"

echo "Installed: $hook -> scripts/pre-commit-checks.sh"
