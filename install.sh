#!/usr/bin/env sh
# Epic HAL installer: fetch a family bundle from a GitHub Release, verify
# its SHA-256, unpack it, and scaffold a project. One command to a
# buildable project, passing the part you target:
#
#   curl -fsSL https://github.com/apojomovsky/epic-hal/releases/latest/download/install.sh \
#     | sh -s -- 16F877A
#
# A part (16F877A) picks its family automatically; a family slug
# (pic16f87xa) installs that family's bundle.
#
# Leaves third_party/epic-hal/ (the vendored library, pinned to the
# resolved version) and, in the current directory, myapp.X, Makefile,
# and main.c. Build with `make`.
#
# Usage: install.sh <family> [<version>] [--part <part>] [--modules <a,b>]
#                   [--name <name>] [--force]
#        install.sh --list | --help
#
# Env: EPIC_HAL_BASE_URL  release base (default
#      https://github.com/apojomovsky/epic-hal/releases; when set, treated
#      as a flat asset dir and <version> becomes required, used by CI).
#      EPIC_HAL_DIR       install dir (default ./third_party/epic-hal).

set -eu

BASE_URL="${EPIC_HAL_BASE_URL:-https://github.com/apojomovsky/epic-hal/releases}"
DEST="${EPIC_HAL_DIR:-./third_party/epic-hal}"

usage() {
    cat <<EOF
usage: install.sh <part-or-family> [<version>] [--part <part>] [--modules <a,b>] \\
                   [--name <name>] [--force] [--with-xc8] [--toolchain epic-cc|xc8]
       install.sh --list | --help

A part (16F877A; case and a PIC/p prefix do not matter) picks its family
automatically; a family slug (pic16f87xa) installs that family's bundle.
--list prints the current families (read from the release, so it never
goes stale).
default toolchain is epic-cc (no Microchip download); pass --with-xc8 for the XC8 alternate.
EOF
}

# Uppercase a part token and drop a PIC/p prefix (pic16f877a -> 16F877A).
norm_part() {
    norm="$(printf '%s' "$1" | tr '[:lower:]' '[:upper:]')"
    case "$norm" in
        PIC*) norm="${norm#PIC}" ;;
        P*) norm="${norm#P}" ;;
    esac
    printf '%s' "$norm"
}

family=
version=
part=
list_only=0
modules=
name=myapp
force=0
toolchain=epic-cc

while [ "$#" -gt 0 ]; do
    case "$1" in
        --list)
            list_only=1
            shift
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        --force)
            force=1
            shift
            ;;
        --with-xc8)
            toolchain=xc8
            shift
            ;;
        --toolchain)
            if [ $# -lt 2 ] || [ -z "${2:-}" ]; then echo "install.sh: --toolchain requires an argument (epic-cc or xc8)" >&2; exit 2; fi
            case "$2" in
                epic-cc|xc8) toolchain="$2" ;;
                *) echo "install.sh: --toolchain must be epic-cc or xc8" >&2; exit 2 ;;
            esac
            shift 2
            ;;
        --part)
            if [ $# -lt 2 ] || [ -z "${2:-}" ]; then echo "install.sh: --part requires an argument" >&2; exit 2; fi
            part="$2"
            shift 2
            ;;
        --modules)
            if [ $# -lt 2 ] || [ -z "${2:-}" ]; then echo "install.sh: --modules requires an argument" >&2; exit 2; fi
            modules="$2"
            shift 2
            ;;
        --name)
            if [ $# -lt 2 ] || [ -z "${2:-}" ]; then echo "install.sh: --name requires an argument" >&2; exit 2; fi
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

# "--list v0.1.0": a lone positional with --list is a version pin, not
# a family; the list must describe that release's families.
if [ "$list_only" -eq 1 ] && [ -z "$version" ] && [ -n "$family" ]; then
    version="$family"
    family=
fi

if [ -z "$family" ] && [ "$list_only" -eq 0 ]; then
    if [ -t 0 ]; then
        printf 'part or family [e.g. 16F877A or pic16f87xa]: '
        read -r family
    else
        echo "install.sh: no part or family given and stdin is not a tty (piped)" >&2
        echo "install.sh: pass a part like 16F877A, or run with --list for the families" >&2
        exit 2
    fi
fi

if [ -n "${EPIC_HAL_BASE_URL:-}" ]; then
    # CI override: treat the base as a flat directory of assets. The
    # version is part of the bundle filename, so it must be explicit;
    # --list only reads parts.txt, whose name carries no version.
    if [ -z "$version" ] && [ "$list_only" -eq 0 ]; then
        echo "install.sh: EPIC_HAL_BASE_URL is set, a version argument is required" >&2
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

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT INT TERM

# One fetch of the parts map drives everything: part->family
# resolution, the slug check, and --list. It is generated from the
# manifest at release time, so the installer never carries a family
# list of its own to go stale.
parts_file="$tmp/parts.txt"
if ! curl -fsSL "$asset_dir/parts.txt" -o "$parts_file"; then
    echo "install.sh: could not fetch the parts map from $asset_dir" >&2
    exit 1
fi

# parts.txt groups its lines by family in the manifest's order, so
# first appearance is the family order and no sort is needed.
families_list() {
    awk '!seen[$2]++ { print $2 }' "$parts_file"
}

if [ "$list_only" -eq 1 ]; then
    families_list
    echo "(or pass a part like 16F877A and the family is picked for you)" >&2
    exit 0
fi

# A family slug (any case) installs its bundle directly; anything else
# is a part, whose family resolves from parts.txt below. An explicit
# --part still overrides (install.sh 16F877A --part X).
token="$(printf '%s' "$family" | tr '[:upper:]' '[:lower:]')"
if families_list | grep -Fxq "$token"; then
    family="$token"
else
    part="${part:-$(norm_part "$family")}"
    family=
fi

if [ -n "$part" ]; then
    part="$(norm_part "$part")"
fi

if [ -z "$family" ]; then
    # Part form: resolve the family before downloading the full bundle.
    echo "install.sh: resolving the family for part $part"
    family="$(awk -v p="$part" '$1 == p { print $2; exit }' "$parts_file")"
    if [ -z "$family" ]; then
        echo "install.sh: unknown part '$part'" >&2
        echo "install.sh: families: $(families_list | paste -sd ' ' -)" >&2
        exit 2
    fi
fi

if ! families_list | grep -Fxq "$family"; then
    echo "install.sh: unknown family '$family'" >&2
    echo "install.sh: families: $(families_list | paste -sd ' ' -)" >&2
    exit 2
fi

if [ -e "$DEST" ] && [ "$force" -ne 1 ]; then
    echo "install.sh: $DEST already exists; pass --force to replace it" >&2
    exit 2
fi

echo "install.sh: fetching epic-hal-$family-$version.tar.gz from $asset_dir"
curl -fsSL "$asset_dir/epic-hal-$family-$version.tar.gz" \
    -o "$tmp/epic-hal-$family-$version.tar.gz"
# The consumer bundles are pure libraries; the scaffolder CLI ships as
# its own asset and is fetched alongside.
curl -fsSL "$asset_dir/epic-hal-cli-$version.tar.gz" \
    -o "$tmp/epic-hal-cli-$version.tar.gz"
curl -fsSL "$asset_dir/SHA256SUMS" -o "$tmp/SHA256SUMS"

for art in "epic-hal-$family-$version.tar.gz" "epic-hal-cli-$version.tar.gz"; do
    expected="$(awk -v n="$art" '$2 == n || $2 == "./" n { print $1 }' "$tmp/SHA256SUMS")"
    if [ -z "$expected" ]; then
        echo "install.sh: no checksum for $art in SHA256SUMS" >&2
        exit 1
    fi
    if command -v sha256sum >/dev/null 2>&1; then
        actual="$(sha256sum "$tmp/$art" | awk '{ print $1 }')"
    else
        actual="$(shasum -a 256 "$tmp/$art" | awk '{ print $1 }')"
    fi
    if [ "$actual" != "$expected" ]; then
        echo "install.sh: checksum mismatch for $art" >&2
        exit 1
    fi
done
echo "install.sh: checksum OK"

mkdir -p "$(dirname "$DEST")"
tar xzf "$tmp/epic-hal-$family-$version.tar.gz" -C "$tmp"
if [ "$force" -eq 1 ]; then
    rm -rf "$DEST"
fi
mv "$tmp/epic-hal-$family-$version" "$DEST"
tar xzf "$tmp/epic-hal-cli-$version.tar.gz" -C "$tmp"
CLI="$tmp/epic-hal-cli-$version/epic-hal"

manifest_family="$(awk '/^EPIC_HAL_FAMILY[[:space:]]*:=/{ print $NF }' "$DEST/epic-hal.mk")"
default_part="$(awk '/^EPIC_HAL_VARIANTS[[:space:]]*:=/{ print $NF }' "$DEST/epic-hal.mk")"
# Default to tick-only: the blink main.c uses just tick + GPIO, and
# linking serial pushes the PIC16 call graph past the 8-level hardware
# stack (XC8 warning 1393). Users add serial with --modules serial,tick.
if grep -q '^EPIC_HAL_MODULE_tick :=' "$DEST/epic-hal.mk"; then
    default_modules="tick"
else
    default_modules="$(sed -n 's/^EPIC_HAL_MODULE_\([a-z0-9][a-z0-9]*\) := .*/\1/p' "$DEST/epic-hal.mk" | head -n 2 | paste -sd, -)"
fi
[ -n "$part" ] || part="$default_part"
[ -n "$modules" ] || modules="$default_modules"

echo "install.sh: scaffolding with part=$part modules=$modules"
if ! command -v python3 >/dev/null 2>&1; then
    echo "install.sh: python3 is required to scaffold the project." >&2
    echo "install.sh: the bundle is installed and verified; install python3 and" >&2
    echo "install.sh: rerun this installer, or use:" >&2
    echo "install.sh:   pipx install git+https://github.com/apojomovsky/epic-hal" >&2
    exit 1
fi
"$CLI" init \
    --family "$manifest_family" \
    --part "$part" \
    --modules "$modules" \
    --name "$name" \
    --bundle "$DEST" \
    --toolchain "$toolchain"

echo
echo "Epic HAL $version ($family) installed in $DEST (toolchain=$toolchain)"
echo "Scaffolded project: ./$name.X"
if [ "$toolchain" = "epic-cc" ]; then
    # Epic-cc is the default: zero Microchip downloads.
    if command -v epic-cc >/dev/null 2>&1; then
        echo "epic-cc found on PATH. Build it with:  make"
    else
        echo "epic-cc not found on PATH. Install it from https://github.com/apojomovsky/epic-cc/releases" >&2
        echo "  or build it: cargo install --git https://github.com/apojomovsky/epic-cc epic-cc" >&2
        echo "then:  make   (or: make TOOLCHAIN=epic-cc)" >&2
    fi
    echo "XC8 alternate:  ./install.sh --with-xc8  or  make TOOLCHAIN=xc8"
else
    # XC8 alternate: keep the existing reporting.
    xc8_ok=0
    dfp_ok=0
    dfp_name=
    if command -v xc8-cc >/dev/null 2>&1; then
        xc8_ok=1
        xc8_root="$(dirname "$(dirname "$(command -v xc8-cc)")")"
        dfp_name="$(awk '/^EPIC_HAL_DFP[[:space:]]*:=/{ print $NF }' "$DEST/epic-hal.mk")"
        if [ -d "$xc8_root/pic/packs/$dfp_name/xc8" ]; then
            dfp_ok=1
        fi
    fi
    if [ "$xc8_ok" -eq 1 ] && [ "$dfp_ok" -eq 1 ]; then
        echo "XC8 and the $dfp_name device pack are ready. Build it with:  make TOOLCHAIN=xc8"
    elif [ "$xc8_ok" -eq 1 ]; then
        dfp_version="$(awk '/^EPIC_HAL_DFP_VERSION[[:space:]]*:=/{ print $NF }' "$DEST/epic-hal.mk")"
        echo "xc8-cc is on PATH, but the $dfp_name device pack is missing." >&2
        if [ -n "$dfp_version" ]; then
            echo "Download it (Microchip's official pack CDN):" >&2
            echo "  curl -fsSL -o $dfp_name.$dfp_version.atpack \\" >&2
            echo "    https://packs.download.microchip.com/$dfp_name.$dfp_version.atpack" >&2
        else
            echo "Download it from https://packs.download.microchip.com/ (any recent version)." >&2
        fi
        echo "then unzip it into $xc8_root/pic/packs/, and run:  make TOOLCHAIN=xc8" >&2
    else
        echo "xc8-cc not found on PATH. Add MPLAB XC8's bin/ to PATH, e.g.:" >&2
        echo "  export PATH=\$PATH:/opt/microchip/xc8/v4.00/bin" >&2
        echo "then:  make TOOLCHAIN=xc8" >&2
    fi
fi
echo "Or open ./$name.X in MPLAB X or the VS Code extension."
