#!/usr/bin/env python3
"""Flash/RAM table over epic-cc sim builds' JSON size reports.

Reads each expected demo's ``<mcu>-<sim name>.json`` beside its hex in
the sim build dir (written by the driver's ``--report`` flag, see
``epic_build.py build --report``) and prints one flash/RAM row per
demo. A demo with no readable report prints FAIL; tolerated demos
(``--tolerate``, the known-broken encoder page overflow) still print
FAIL but exit 0. Exit 1 = an untolerated demo has no report.
"""

from __future__ import annotations

import argparse
import json
import pathlib
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
import epic_build  # noqa: E402
import epicmanifest as manifest_lib  # noqa: E402


def parse_spec(text: str) -> tuple[str, str]:
    """Split a ``MODULE:MCU`` spec; argparse calls this, so errors exit 2."""
    try:
        module, mcu = text.split(":")
    except ValueError:
        raise argparse.ArgumentTypeError(f"expected MODULE:MCU, got {text!r}")
    if not module or not mcu:
        raise argparse.ArgumentTypeError(f"expected MODULE:MCU, got {text!r}")
    return module, mcu


def report_path(manifest, build_dir: pathlib.Path, module: str, mcu: str) -> pathlib.Path:
    """The JSON path the emitted sim build script writes for this demo."""
    name, _ = epic_build._example_name_and_config(manifest, module, mcu, "sim", "epic-cc")
    return build_dir / f"{mcu}-{name}.json"

def read_row(path: pathlib.Path) -> tuple[str, str] | None:
    """(flash, ram) used/total cells, or None when the report is missing."""
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
        flash = data["flash_words"]
        ram = data["ram_bytes"]
        return f"{flash['used']} / {flash['total']}", f"{ram['used']} / {ram['total']}"
    except (OSError, ValueError, KeyError, TypeError):
        return None


def collect(manifest, build_dir: pathlib.Path, specs: list[tuple[str, str]]):
    """One record per spec: the cells, or a FAIL with where it looked."""
    rows = []
    for module, mcu in specs:
        path = report_path(manifest, build_dir, module, mcu)
        cells = read_row(path)
        if cells is None:
            rows.append((module, mcu, f"FAIL no report at {path}"))
        else:
            rows.append((module, mcu, f"flash {cells[0]} words, RAM {cells[1]} bytes"))
    return rows


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--build-dir", default="build/epiccc-sim",
                        help="sim build dir holding the <mcu>-<name>.json reports")
    parser.add_argument("--tolerate", action="append", default=[], type=parse_spec,
                        metavar="MODULE:MCU",
                        help="a demo allowed to FAIL (repeatable)")
    parser.add_argument("specs", nargs="+", type=parse_spec, metavar="MODULE:MCU",
                        help="expected demos, e.g. epic-menu-demo:18F4550")
    args = parser.parse_args(argv)
    manifest = manifest_lib.load(manifest_lib.default_path())
    build_dir = pathlib.Path(args.build_dir)
    if not build_dir.is_absolute():
        build_dir = REPO / build_dir
    tolerated = set(args.tolerate)
    failed = False
    for module, mcu, text in collect(manifest, build_dir, args.specs):
        print(f"{module} {mcu} {text}")
        if text.startswith("FAIL") and (module, mcu) not in tolerated:
            failed = True
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
