#!/usr/bin/env bash
# pre-push: block force pushes. Run from .git/hooks/pre-push by the
# dispatcher install-git-hooks.sh installs.
#
# A force push drops the branch tip for every other agent pulling it.
# A genuine rewrite needs a human go-ahead, then:
#   EPIC_FORCE_PUSH_APPROVED=1 git push --force-with-lease

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

# ---- prose lint gate (fires even when the ritual was never run) ----

if ! bash scripts/prose-diff.sh --verify; then
    echo "pre-push: comment blocks violating the prose rules (see above)" >&2
    echo "  Fix the flagged blocks, or reword them so they hold up." >&2
    exit 1
fi
exit 0
