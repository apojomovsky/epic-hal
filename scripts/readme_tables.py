#!/usr/bin/env python3
"""Regenerate the README's docgen blocks from the manifest.

The family-bearing README tables drifted from the manifest every time a
family landed (epic-hal#219), so they are generated now: each table sits
between docgen markers and this script rewrites it in place. --check
fails without writing; scripts/tests/test_family_lists.py runs it so a
stale table cannot merge."""
from __future__ import annotations

import argparse
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import bundlegen  # noqa: E402
import epicmanifest  # noqa: E402

README = pathlib.Path(__file__).resolve().parents[1] / "README.md"

BLOCKS = {
    "packs": bundlegen.emit_readme_pack_table,
    "families": bundlegen.emit_readme_families_table,
    "bundles": bundlegen.emit_readme_bundle_table,
}


class DocgenError(Exception):
    """A docgen marker pair the README is supposed to carry is missing."""


def regenerate(text: str) -> tuple[str, list[str]]:
    """Rewrite each docgen block from its generator.

    Returns the new text and the keys whose block was stale."""
    manifest = epicmanifest.load(epicmanifest.default_path())
    stale = []
    for key, emit in BLOCKS.items():
        begin = f"<!-- docgen:{key} begin"
        end = f"<!-- docgen:{key} end -->"
        try:
            start = text.index(begin)
            stop = text.index(end, start) + len(end)
        except ValueError:
            raise DocgenError(
                f"no docgen:{key} markers in README.md; restore the"
                f" docgen:{key} begin/end comment pair") from None
        block = emit(manifest)
        if text[start:stop] != block:
            stale.append(key)
        text = text[:start] + block + text[stop:]
    return text, stale


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Regenerate the README's docgen blocks from the manifest.")
    parser.add_argument("--check", action="store_true",
                        help="fail on a stale block instead of rewriting it")
    args = parser.parse_args()
    try:
        text, stale = regenerate(README.read_text())
    except DocgenError as exc:
        print(f"readme_tables: {exc}", file=sys.stderr)
        return 2
    if args.check:
        if stale:
            print(f"readme_tables: stale docgen blocks: {', '.join(stale)};"
                  f" run scripts/readme_tables.py to regenerate",
                  file=sys.stderr)
            return 1
        return 0
    README.write_text(text)
    for key in stale:
        print(f"readme_tables: regenerated docgen:{key}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
