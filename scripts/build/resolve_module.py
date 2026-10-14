#!/usr/bin/env python3
"""Resolve a MODULE value (a path module id, epic-hal#308) to either form.

The id is the directory, so both spellings print the same string; the
flags stay so existing Makefile recipes keep working unchanged.

Usage: resolve_module.py --id|--dir <value>
"""
import argparse
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import epicmanifest  # noqa: E402


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    form = parser.add_mutually_exclusive_group(required=True)
    form.add_argument("--id", action="store_true", help="print the manifest id")
    form.add_argument("--dir", action="store_true", help="print the module dir")
    parser.add_argument("value", help="a manifest id or a module directory")
    args = parser.parse_args()

    manifest = epicmanifest.load(epicmanifest.default_path())
    try:
        mod = manifest.resolve_module(args.value)
    except epicmanifest.ManifestError as exc:
        sys.exit(f"error: {exc}")
    print(mod.name if args.id else mod.dir)


if __name__ == "__main__":
    main()
