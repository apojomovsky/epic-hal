#!/usr/bin/env bash
# One-time setup, called by scripts/bootstrap.sh (or by hand after a clone):
# writes a small dispatcher per hook into the git hooks dir (that dir is
# not tracked by git, so this is needed once per clone). See
# scripts/README.md for what each hook checks.
#
# Worktree-aware: the hooks dir lives in the shared common dir, so one
# install covers every worktree. Each dispatcher runs the checks script
# from the worktree being committed in, falling back to the main
# checkout, so a branch that edits a hook script checks with its own
# copy instead of the main checkout's stale one.

set -euo pipefail

common_dir="$(cd "$(git rev-parse --git-common-dir)" && pwd)"
hooks="$common_dir/hooks"

mkdir -p "$hooks"
for hook in pre-commit commit-msg pre-push; do
    rm -f "$hooks/$hook"
    cat > "$hooks/$hook" <<EOF
#!/usr/bin/env bash
# $hook dispatcher: exec this worktree's own scripts/$hook-checks.sh, so
# a branch editing a hook checks with its edited copy. Falls back to the
# main checkout when the worktree copy is missing. Written by
# scripts/install-git-hooks.sh; edit the checks script, not this file.
set -euo pipefail
top="\$(git rev-parse --show-toplevel 2>/dev/null || true)"
if [ -n "\$top" ] && [ -f "\$top/scripts/$hook-checks.sh" ]; then
    exec bash "\$top/scripts/$hook-checks.sh" "\$@"
fi
common="\$(git rev-parse --git-common-dir 2>/dev/null || true)"
if [ -n "\$common" ]; then
    fallback="\$(cd "\$common" && pwd)/../scripts/$hook-checks.sh"
    if [ -f "\$fallback" ]; then
        exec bash "\$fallback" "\$@"
    fi
fi
echo "$hook: no scripts/$hook-checks.sh here or in the main checkout" >&2
exit 1
EOF
    chmod +x "$hooks/$hook"
    echo "Installed: $hooks/$hook"
done
