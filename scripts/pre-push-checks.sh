#!/usr/bin/env bash
# pre-push: block force pushes. Installed into .git/hooks/pre-push by
# scripts/install-git-hooks.sh.
#
# A push is a force when the remote ref is not an ancestor of the local
# oid (a non-fast-forward rewrite): the pushed commit drops the current
# branch tip for every other agent pulling that branch. That loss is
# unrecoverable without a reflog on each downstream clone, which is why
# it must be an explicit human decision, not a default.
#
# When a rewrite is genuinely needed (messy history), get the human's
# explicit go-ahead, then re-run with:
#   EPIC_FORCE_PUSH_APPROVED=1 git push --force-with-lease
#
# New branches and branch deletions are not rewrites and always pass.

set -u

approved="${EPIC_FORCE_PUSH_APPROVED:-0}"
force_refs=""

while read -r local_ref local_oid remote_ref remote_oid; do
    [ -z "$local_ref" ] && continue
    [ "$remote_oid" = "0000000000000000000000000000000000000000" ] && continue
    [ "$local_oid" = "0000000000000000000000000000000000000000" ] && continue
    if ! git merge-base --is-ancestor "$remote_oid" "$local_oid" 2>/dev/null; then
        force_refs="$force_refs $local_ref"
    fi
done

if [ -n "$force_refs" ]; then
    if [ "$approved" = "1" ]; then
        echo "pre-push: force push approved for:$force_refs (EPIC_FORCE_PUSH_APPROVED=1)"
        exit 0
    fi
    echo "pre-push: refusing force push of:$force_refs" >&2
    echo "  A force push rewrites shared branch history and drops the current" >&2
    echo "  branch commits for every other agent working this repo." >&2
    echo "  If the rewrite is genuinely needed, get the human's explicit" >&2
    echo "  go-ahead, then re-run with:" >&2
    echo "    EPIC_FORCE_PUSH_APPROVED=1 git push --force-with-lease" >&2
    exit 1
fi
exit 0
