#!/usr/bin/env python3
"""Reference MPLAB X projects name only files that exist, and agree.

Each examples/epic-hal-demo-<slug>.X carries its file list three
times: nbproject/configurations.xml (itemPath entries plus the
sourceRootList dir Elems), nbproject/project.xml (sourceRootElem
dirs), and the MPLAB-regenerated nbproject/Makefile-default.mk
(SOURCEFILES plus -I include dirs). All three drifted independently
across renames (epic-hal#314). Per project this checks:

1. every named source, dir and include resolves relative to the .X
   dir itself (Makefile-genesis.properties pins an absolute proj.dir,
   so it is never read; vendored third-party .X trees are out of
   scope, only examples/epic-hal-demo-*.X),
2. the configurations.xml itemPaths equal the Makefile SOURCEFILES,
3. every hal/ or common/ file the project builds is in the manifest
   family's hal_sources, harness_src, or the conditional_sources
   applying to the project's own targetDevice (the demo's own
   main.c and lib/ files need existence only).
"""
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import epicmanifest  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parents[1]
PROJECTS = sorted((REPO / "examples").glob("epic-hal-demo-*.X"))


def _item_paths(config_xml: str) -> set[str]:
    return set(re.findall(r"<itemPath>([^<]+)", config_xml)) - {"Makefile"}


def _elems(config_xml: str) -> set[str]:
    return set(re.findall(r"<Elem>([^<]+)", config_xml))


def _source_roots(project_xml: str) -> set[str]:
    return set(re.findall(r"<sourceRootElem>([^<]+)", project_xml))


def _makefile_sources(makefile: str) -> set[str]:
    match = re.search(r"^SOURCEFILES=(.*)$", makefile, re.M)
    return set(match.group(1).split()) if match else set()


def _makefile_includes(makefile: str) -> set[str]:
    return set(re.findall(r'-I"([^"]+)"', makefile))


def check_project(proj: pathlib.Path, family) -> list[str]:
    """Every drift this project has, as human-readable errors."""
    errors = []
    cfg = (proj / "nbproject" / "configurations.xml").read_text()
    items = _item_paths(cfg)
    mk = (proj / "nbproject" / "Makefile-default.mk").read_text()
    sources = _makefile_sources(mk)

    for kind, paths in (
        ("itemPath", items),
        ("source root", _elems(cfg)),
        ("project root", _source_roots(
            (proj / "nbproject" / "project.xml").read_text())),
        ("Makefile source", sources),
        ("Makefile include", _makefile_includes(mk)),
    ):
        for path in sorted(paths):
            if not (proj / path).exists():
                errors.append(f"{proj.name}: missing {kind} '{path}'")

    if items != sources:
        errors.append(
            f"{proj.name}: configurations.xml and Makefile disagree: "
            f"config-only {sorted(items - sources)}, "
            f"makefile-only {sorted(sources - items)}")

    # Conditional sources apply per part (same rule as
    # Manifest.sources_for): the project's targetDevice selects which
    # variants count, so a conditional gated to another die fails here
    # instead of hiding behind the family's full list.
    device = re.search(r"<targetDevice>([^<]+)", cfg)
    if device is None:
        errors.append(f"{proj.name}: no targetDevice in configurations.xml")
        return errors
    mcu = device.group(1)
    if mcu.startswith("PIC"):
        mcu = mcu[len("PIC"):]
    known = set(family.hal_sources)
    known.update(
        c.path for c in family.conditional_sources if mcu in c.variants)
    for path in sorted(items | sources):
        if path.startswith("../../hal/") or path.startswith("../../common/"):
            rel = "/".join(pathlib.PurePosixPath(path).parts[2:])
            if rel not in known:
                errors.append(
                    f"{proj.name}: '{path}' is not in the manifest "
                    f"family '{family.name}'")
    return errors


def main() -> int:
    manifest = epicmanifest.load(epicmanifest.default_path())
    by_slug = {fam.slug: fam for fam in manifest.families.values()}
    errors = []
    for proj in PROJECTS:
        slug = proj.name[len("epic-hal-demo-"):-len(".X")]
        family = by_slug.get(slug)
        if family is None:
            errors.append(f"{proj.name}: no manifest family for slug '{slug}'")
            continue
        errors.extend(check_project(proj, family))
    for error in errors:
        print(f"mplabx audit: {error}")
    if errors:
        print("mplabx audit: reference projects drifted")
        return 1
    print(f"mplabx audit: {len(PROJECTS)} reference projects agree")
    return 0


if __name__ == "__main__":
    sys.exit(main())
