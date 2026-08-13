#!/usr/bin/env python3
# scripts/epicurus.py
"""epicurus init: scaffold a ready PIC project from an Epicurus bundle."""
from __future__ import annotations
import argparse, pathlib, sys
# The CLI runs from inside a vendored bundle; do not leave __pycache__
# next to the shipped modules in the consumer's tree.
sys.dont_write_bytecode = True
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import epicmanifest, epicurus_init
from bundlegen import modules_for_family


def _find_manifest(bundle: pathlib.Path) -> pathlib.Path:
    cand = bundle / "epic-common" / "manifest" / "modules.toml"
    if cand.exists():
        return cand
    return epicmanifest.default_path()  # repo checkout, scripts/ alongside


def cmd_init(args) -> int:
    bundle = pathlib.Path(args.bundle).resolve()
    manifest = epicmanifest.load(_find_manifest(bundle))
    family = args.family or input(f"family [{', '.join(manifest.families)}]: ").strip()
    if family not in manifest.families:
        print(f"error: unknown family {family!r}; known: {', '.join(sorted(manifest.families))}", file=sys.stderr)
        return 2
    fam = manifest.families.get(family)
    part = args.part or (input(f"part [{', '.join(fam.variants)}]: ").strip() if fam else "")
    mods_s = args.modules or input("modules (comma-separated, e.g. serial,tick): ").strip()
    modules = [m.strip() for m in mods_s.split(",") if m.strip()]
    name = args.name or input("project name [myapp]: ").strip() or "myapp"
    # The .X references the bundle via ../../, so the project must sit one
    # level below the bundle root (same layout as the reference project).
    # Default to <bundle>/projects so the out-of-box flow satisfies that;
    # -o overrides for an explicit location.
    out_dir = args.output or str(bundle / "projects")
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
        epicurus_init.init_project(manifest, family, part, full_modules, bundle, out_dir, name)
    except epicurus_init.SelectionError as e:
        print(f"error: {e}", file=sys.stderr)
        return 2
    except FileExistsError as e:
        print(f"error: {e}", file=sys.stderr)
        return 2
    print(f"created {name}.X, main.c, Makefile in {out_dir}")
    return 0


def main(argv=None) -> int:
    p = argparse.ArgumentParser(prog="epicurus")
    sub = p.add_subparsers(dest="cmd", required=True)
    ip = sub.add_parser("init", help="scaffold a new PIC project")
    ip.add_argument("--family")
    ip.add_argument("--part")
    ip.add_argument("--modules")
    ip.add_argument("--bundle", default=".")
    ip.add_argument("-o", "--output", default=None,
                    help="output dir (default: <bundle>/projects)")
    ip.add_argument("--name", default="myapp")
    ip.set_defaults(func=cmd_init)
    args = p.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
