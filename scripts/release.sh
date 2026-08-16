#!/usr/bin/env sh
# Cut a release: sync with upstream, work out the next version, show the
# notes that would publish, and push the tag that triggers
# release-bundles.yml.
#
#   scripts/release.sh patch     0.3.7 -> 0.3.8
#   scripts/release.sh minor     0.3.7 -> 0.4.0
#   scripts/release.sh major     0.3.7 -> 1.0.0
#   scripts/release.sh v0.5.0    an explicit version
#
# Options: -y skip the confirmation, --watch follow the release run,
#          --dry-run stop before tagging, --remote NAME (default origin).
#
# Pushing the tag is the point of no return: it publishes a public
# GitHub Release. Everything before the confirmation is local and
# undoable, and declining deletes the local tag it made to preview with.

set -eu

REMOTE=origin
BRANCH=master
BUMP=
ASSUME_YES=0
WATCH=0
DRY_RUN=0

usage() {
    cat <<EOF
usage: scripts/release.sh <major|minor|patch|vX.Y.Z> [-y] [--watch]
                          [--dry-run] [--remote NAME] [--branch NAME]

Syncs with the remote, computes the next version from the latest tag,
previews the release notes, then tags and pushes to trigger the release.
EOF
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        -h|--help) usage; exit 0 ;;
        -y|--yes) ASSUME_YES=1; shift ;;
        --watch) WATCH=1; shift ;;
        --dry-run) DRY_RUN=1; shift ;;
        --remote) REMOTE="$2"; shift 2 ;;
        --branch) BRANCH="$2"; shift 2 ;;
        -*) echo "release: unknown option: $1" >&2; usage >&2; exit 2 ;;
        *)
            if [ -n "$BUMP" ]; then
                echo "release: too many arguments" >&2
                exit 2
            fi
            BUMP="$1"
            shift
            ;;
    esac
done

if [ -z "$BUMP" ]; then
    echo "release: say what to bump: major, minor, patch, or an explicit vX.Y.Z" >&2
    usage >&2
    exit 2
fi

cd "$(git rev-parse --show-toplevel)"

# ---- preflight -------------------------------------------------------
# A dirty tree is the dangerous case: the tag would point at a commit
# that does not contain what is on disk, and the bundles are built from
# the tag, not from the working copy.
if [ -n "$(git status --porcelain)" ]; then
    echo "release: working tree is dirty; commit or stash first" >&2
    git status --short >&2
    exit 1
fi

current_branch="$(git rev-parse --abbrev-ref HEAD)"
if [ "$current_branch" != "$BRANCH" ]; then
    echo "release: on '$current_branch', expected '$BRANCH'" >&2
    echo "release: switch first, or pass --branch $current_branch" >&2
    exit 1
fi

echo "release: syncing with $REMOTE"
git fetch --prune --tags "$REMOTE"

# --ff-only: if master and the remote have diverged, stop rather than
# silently merging something into the release.
if ! git merge-base --is-ancestor "$REMOTE/$BRANCH" HEAD 2>/dev/null; then
    git pull --ff-only "$REMOTE" "$BRANCH"
fi

local_head="$(git rev-parse HEAD)"
remote_head="$(git rev-parse "$REMOTE/$BRANCH")"
if [ "$local_head" != "$remote_head" ]; then
    echo "release: $BRANCH is not level with $REMOTE/$BRANCH after syncing." >&2
    echo "release: local  $local_head" >&2
    echo "release: remote $remote_head" >&2
    echo "release: push or reset before releasing" >&2
    exit 1
fi

# ---- work out the version -------------------------------------------
latest="$(git tag --sort=-v:refname | head -n 1)"
[ -n "$latest" ] || latest=

case "$BUMP" in
    v[0-9]*)
        new="$BUMP"
        ;;
    major|minor|patch)
        if [ -z "$latest" ]; then
            echo "release: no existing tag to bump from; pass an explicit vX.Y.Z" >&2
            exit 1
        fi
        core="${latest#v}"
        major="${core%%.*}"
        rest="${core#*.}"
        minor="${rest%%.*}"
        patch="${rest#*.}"
        # Drop any prerelease/build suffix (0.4.0-rc1 -> 0.4.0).
        patch="${patch%%-*}"
        patch="${patch%%+*}"
        case "$major$minor$patch" in
            *[!0-9]*)
                echo "release: cannot parse '$latest' as vMAJOR.MINOR.PATCH" >&2
                exit 1
                ;;
        esac
        case "$BUMP" in
            major) major=$((major + 1)); minor=0; patch=0 ;;
            minor) minor=$((minor + 1)); patch=0 ;;
            patch) patch=$((patch + 1)) ;;
        esac
        new="v${major}.${minor}.${patch}"
        ;;
    *)
        echo "release: '$BUMP' is not major, minor, patch, or a vX.Y.Z version" >&2
        exit 2
        ;;
esac

if git rev-parse -q --verify "refs/tags/$new" >/dev/null; then
    echo "release: tag $new already exists" >&2
    exit 1
fi

echo "release: $latest -> $new  ($(git rev-parse --short HEAD))"

if [ "$latest" != "" ] && [ -z "$(git log --oneline "$latest..HEAD")" ]; then
    echo "release: no commits since $latest; this would republish the same tree" >&2
    if [ "$ASSUME_YES" -eq 0 ]; then
        printf 'release: continue anyway? [y/N] '
        read -r reply </dev/tty || reply=n
        case "$reply" in [yY]*) ;; *) echo "release: aborted"; exit 1 ;; esac
    fi
fi

# ---- preview ---------------------------------------------------------
# Tag locally first so the preview is generated from the real tag rather
# than an approximation of it. Declining removes the tag again.
git tag -a "$new" -m "Epic HAL $new"

cleanup_tag() {
    git tag -d "$new" >/dev/null 2>&1 || true
}

repo_url="$(git remote get-url "$REMOTE" \
    | sed -e 's#^git@github\.com:#https://github.com/#' \
          -e 's#^ssh://git@github\.com/#https://github.com/#' \
          -e 's#\.git$##')"

echo
if [ -x scripts/release_notes.py ] || [ -f scripts/release_notes.py ]; then
    if ! python3 scripts/release_notes.py "$new" --repo-url "$repo_url"; then
        cleanup_tag
        echo "release: could not generate the notes preview" >&2
        exit 1
    fi
else
    echo "## What changed (preview unavailable, scripts/release_notes.py missing)"
    git log --pretty='- %s' "$latest..$new"
fi
echo

if [ "$DRY_RUN" -eq 1 ]; then
    cleanup_tag
    echo "release: dry run, nothing pushed (local tag removed)"
    exit 0
fi

if [ "$ASSUME_YES" -eq 0 ]; then
    if [ ! -t 0 ] && [ ! -e /dev/tty ]; then
        cleanup_tag
        echo "release: no terminal to confirm on; pass -y to publish" >&2
        exit 1
    fi
    printf 'release: push %s and publish? [y/N] ' "$new"
    read -r reply </dev/tty || reply=n
    case "$reply" in
        [yY]*) ;;
        *) cleanup_tag; echo "release: aborted, local tag removed"; exit 1 ;;
    esac
fi

# ---- publish ---------------------------------------------------------
if ! git push "$REMOTE" "refs/tags/$new"; then
    cleanup_tag
    echo "release: push failed, local tag removed" >&2
    exit 1
fi

echo "release: pushed $new; release-bundles.yml is building"
echo "release: $repo_url/actions/workflows/release-bundles.yml"

if [ "$WATCH" -eq 1 ]; then
    if command -v gh >/dev/null 2>&1; then
        # The run needs a moment to exist before it can be watched.
        sleep 5
        run_id="$(gh run list --workflow release-bundles.yml \
            --limit 1 --json databaseId --jq '.[0].databaseId' 2>/dev/null || true)"
        if [ -n "$run_id" ]; then
            gh run watch "$run_id" --exit-status
        else
            echo "release: could not find the run yet; check the URL above" >&2
        fi
    else
        echo "release: gh not installed, cannot watch; check the URL above" >&2
    fi
fi
