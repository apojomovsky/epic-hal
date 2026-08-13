#!/usr/bin/env sh
# Epicurus installer: fetch a family bundle from a GitHub Release, verify
# its SHA-256, unpack it, and scaffold a project. One command to a
# buildable project:
#
#   curl -fsSL https://github.com/apojomovsky/epicurus/releases/latest/download/install.sh \
#     | sh -s -- pic16f87xa
#
# Leaves third_party/epicurus/ (the vendored library, pinned to the
# resolved version) and, in the current directory, myapp.X, Makefile,
# and main.c. Build with `make`.
#
# Usage: install.sh <family> [<version>] [--part <part>] [--modules <a,b>]
#                   [--name <name>] [--force]
#        install.sh --list | --help
#
# Env: EPICURUS_BASE_URL  release base (default
#      https://github.com/apojomovsky/epicurus/releases; when set, treated
#      as a flat asset dir and <version> becomes required, used by CI).
#      EPICURUS_DIR       install dir (default ./third_party/epicurus).

set -eu

BASE_URL="${EPICURUS_BASE_URL:-https://github.com/apojomovsky/epicurus/releases}"
DEST="${EPICURUS_DIR:-./third_party/epicurus}"
FAMILIES="pic16f87xa pic18fxx5x pic16f193x"

usage() {
    cat <<EOF
usage: install.sh <family> [<version>] [--part <part>] [--modules <a,b>] \\
                   [--name <name>] [--force]
       install.sh --list | --help

families: $FAMILIES
EOF
}

family=
version=
part=
modules=
name=myapp
force=0

while [ "$#" -gt 0 ]; do
    case "$1" in
        --list)
            printf '%s\n' "$FAMILIES"
            exit 0
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        --force)
            force=1
            shift
            ;;
        --part)
            part="$2"
            shift 2
            ;;
        --modules)
            modules="$2"
            shift 2
            ;;
        --name)
            name="$2"
            shift 2
            ;;
        -*)
            echo "install.sh: unknown option: $1" >&2
            usage >&2
            exit 2
            ;;
        *)
            if [ -z "$family" ]; then
                family="$1"
            elif [ -z "$version" ]; then
                version="$1"
            else
                echo "install.sh: too many positional arguments" >&2
                usage >&2
                exit 2
            fi
            shift
            ;;
    esac
done

if [ -z "$family" ]; then
    if [ -t 0 ]; then
        printf 'family [%s]: ' "$FAMILIES"
        read -r family
    else
        echo "install.sh: no family given and stdin is not a tty (piped)" >&2
        echo "install.sh: run with --list to see the families" >&2
        exit 2
    fi
fi

case " $FAMILIES " in
    *" $family "*) ;;
    *)
        echo "install.sh: unknown family '$family'" >&2
        echo "install.sh: families: $FAMILIES" >&2
        exit 2
        ;;
esac

if [ -n "${EPICURUS_BASE_URL:-}" ]; then
    # CI override: treat the base as a flat directory of assets. The
    # version is part of the asset filename, so it must be explicit.
    if [ -z "$version" ]; then
        echo "install.sh: EPICURUS_BASE_URL is set, a version argument is required" >&2
        exit 2
    fi
    asset_dir="$BASE_URL"
else
    if [ -z "$version" ]; then
        resolved="$(curl -fsSL -o /dev/null -w '%{url_effective}' "$BASE_URL/latest")"
        version="${resolved##*/}"
        [ -n "$version" ] && [ "$version" != "latest" ] || {
            echo "install.sh: could not resolve the latest release version" >&2
            exit 1
        }
    fi
    asset_dir="$BASE_URL/download/$version"
fi

if [ -e "$DEST" ] && [ "$force" -ne 1 ]; then
    echo "install.sh: $DEST already exists; pass --force to replace it" >&2
    exit 2
fi

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT INT TERM

echo "install.sh: fetching epicurus-$family-$version.tar.gz from $asset_dir"
curl -fsSL "$asset_dir/epicurus-$family-$version.tar.gz" -o "$tmp/bundle.tar.gz"
curl -fsSL "$asset_dir/SHA256SUMS" -o "$tmp/SHA256SUMS"

expected="$(awk -v n="epicurus-$family-$version.tar.gz" '$2 == n || $2 == "./" n { print $1 }' "$tmp/SHA256SUMS")"
if [ -z "$expected" ]; then
    echo "install.sh: no checksum for epicurus-$family-$version.tar.gz in SHA256SUMS" >&2
    exit 1
fi
if command -v sha256sum >/dev/null 2>&1; then
    actual="$(sha256sum "$tmp/bundle.tar.gz" | awk '{ print $1 }')"
else
    actual="$(shasum -a 256 "$tmp/bundle.tar.gz" | awk '{ print $1 }')"
fi
if [ "$actual" != "$expected" ]; then
    echo "install.sh: checksum mismatch for epicurus-$family-$version.tar.gz" >&2
    exit 1
fi
echo "install.sh: checksum OK"

mkdir -p "$(dirname "$DEST")"
tar xzf "$tmp/bundle.tar.gz" -C "$tmp"
if [ "$force" -eq 1 ]; then
    rm -rf "$DEST"
fi
mv "$tmp/epicurus-$family-$version" "$DEST"

manifest_family="$(awk '/^EPICURUS_FAMILY[[:space:]]*:=/{ print $NF }' "$DEST/epicurus.mk")"
default_part="$(awk '/^EPICURUS_VARIANTS[[:space:]]*:=/{ print $NF }' "$DEST/epicurus.mk")"
if grep -q '^EPICURUS_MODULE_serial :=' "$DEST/epicurus.mk" \
   && grep -q '^EPICURUS_MODULE_tick :=' "$DEST/epicurus.mk"; then
    default_modules="serial,tick"
else
    default_modules="$(sed -n 's/^EPICURUS_MODULE_\([a-z0-9][a-z0-9]*\) := .*/\1/p' "$DEST/epicurus.mk" | head -n 2 | paste -sd, -)"
fi
[ -n "$part" ] || part="$default_part"
[ -n "$modules" ] || modules="$default_modules"

echo "install.sh: scaffolding with part=$part modules=$modules"
if ! command -v python3 >/dev/null 2>&1; then
    echo "install.sh: python3 is required to scaffold the project." >&2
    echo "install.sh: the bundle is installed and verified; once python3 exists, run:" >&2
    echo "  $DEST/epicurus init" >&2
    exit 1
fi
"$DEST/epicurus" init \
    --family "$manifest_family" \
    --part "$part" \
    --modules "$modules" \
    --name "$name" \
    --bundle "$DEST"

echo
echo "Epicurus $version ($family) installed in $DEST"
echo "Scaffolded project: ./$name.X"
if command -v xc8-cc >/dev/null 2>&1; then
    echo "XC8 found. Build it with:  make"
else
    echo "XC8 not found. Install MPLAB XC8 (free tier is enough), then:"
    echo "  make"
fi
echo "Or open ./$name.X in MPLAB X or the VS Code extension."
