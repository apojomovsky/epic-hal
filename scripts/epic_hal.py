#!/usr/bin/env python3
# scripts/epic_hal.py
"""epic-hal init: scaffold a ready PIC project from an Epic HAL bundle."""
from __future__ import annotations
import argparse, pathlib, sys
# The CLI runs from inside a vendored bundle; do not leave __pycache__
# next to the shipped modules in the consumer's tree.
sys.dont_write_bytecode = True
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import epicmanifest, epic_hal_init
from bundlegen import modules_for_family


def _find_manifest(bundle: pathlib.Path) -> pathlib.Path:
    cand = bundle / "epic-common" / "manifest" / "modules.toml"
    if cand.exists():
        return cand
    # The standalone CLI asset is flat (helpers next to the executable,
    # unlike the repo's scripts/ layout), so default_path() resolves one
    # level too high; the manifest it carries lives next to it.
    own = pathlib.Path(__file__).resolve().parent / "epic-common" / "manifest" / "modules.toml"
    if own.exists():
        return own
    return epicmanifest.default_path()  # repo checkout, scripts/ alongside


def cmd_init(args) -> int:
    bundle = pathlib.Path(args.bundle).resolve()
    manifest = epicmanifest.load(_find_manifest(bundle))
    part = epic_hal_init.normalize_part(args.part) if args.part is not None else None
    if args.family is not None:
        family = next(
            (n for n in manifest.families if n.lower() == args.family.lower()),
            None)
        if family is None:
            print(f"error: unknown family {args.family!r}; known: {', '.join(sorted(manifest.families))}",
                  file=sys.stderr)
            return 2
    elif part is not None:
        # A part implies its family; resolve it from the manifest.
        family = epic_hal_init.family_for_part(manifest, part)
        if family is None:
            known = sorted(v for f in manifest.families.values() for v in f.variants)
            print(f"error: unknown part {args.part!r}; known parts: {', '.join(known)}",
                  file=sys.stderr)
            return 2
    else:
        family = input(f"family [{', '.join(sorted(manifest.families))}]: ").strip()
        if family not in manifest.families:
            print(f"error: unknown family {family!r}; known: {', '.join(sorted(manifest.families))}", file=sys.stderr)
            return 2
    fam = manifest.families[family]
    part = part or (input(f"part [{', '.join(fam.variants)}]: ").strip() if fam else "")
    mods_s = args.modules or input("modules (comma-separated, e.g. serial,tick): ").strip()
    modules = [m.strip() for m in mods_s.split(",") if m.strip()]
    name = args.name or input("project name [myapp]: ").strip() or "myapp"
    # The scaffold lands in the current directory (in place), with the
    # bundle vendored alongside (e.g. third_party/epic-hal); -o overrides.
    out_dir = args.output or "."
    # The user gives short module names (serial,tick); init_project and
    # resolve_selection expect full manifest names (epic-serial, epic-tick).
    by_short = {f.removeprefix("epic-"): f for f in modules_for_family(manifest, family)}
    full_modules = []
    for m in modules:
        if m not in by_short:
            print(f"error: unknown module {m!r}; available: {', '.join(sorted(by_short))}", file=sys.stderr)
            return 2
        full_modules.append(by_short[m])
    try:
        epic_hal_init.init_project(manifest, family, part, full_modules, bundle, out_dir, name, toolchain=getattr(args, "toolchain", "epic-cc"))
    except epic_hal_init.SelectionError as e:
        print(f"error: {e}", file=sys.stderr)
        return 2
    except FileExistsError as e:
        print(f"error: {e}", file=sys.stderr)
        return 2
    print(f"created {name}.X, main.c, Makefile in {out_dir}")
    return 0


def main(argv=None) -> int:
    p = argparse.ArgumentParser(prog="epic-hal")
    sub = p.add_subparsers(dest="cmd", required=True)
    ip.add_argument("--family")
    ip.add_argument("--part")
    ip.add_argument("--modules")
    ip.add_argument("--bundle", default=".")
    ip.add_argument("-o", "--output", default=None,
                    help="output dir (default: current directory)")
    ip.add_argument("--name", default="myapp")
    ip.add_argument("--toolchain", choices=["epic-cc", "xc8"], default="epic-cc",
                    help="toolchain for the scaffolded Makefile (default: epic-cc)")
    ip.set_defaults(func=cmd_init)
    args = p.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
