#!/usr/bin/env python3
"""MPLAB X projects name only files that exist, and agree.

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

lib/usb's bring-up project (lib/usb/mcu/pic18fxx5x-usb-mplabx/) is
checked differently: it carries no committed Makefile-default.mk (the
IDE generates that one), and what matters is that it describes the
same build the manifest gates. So its itemPaths, include dirs and
configuration words are compared against the manifest's own answer
for lib/usb on the project's targetDevice.
"""
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "build"))

import epicmanifest  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parents[2]
PROJECTS = sorted((REPO / "examples").glob("epic-hal-demo-*.X"))

# The module bring-up project, checked against the manifest.
USB_PROJECT = (REPO / "lib" / "usb" / "mcu" / "pic18fxx5x-usb-mplabx" /
               "pic18fxx5x-usb.X")
USB_MODULE = "lib/usb"


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


def check_usb_project(manifest: epicmanifest.Manifest) -> list[str]:
    """The USB bring-up project must describe the manifest's own build.

    Its file list, include dirs, macros and configuration words are
    compared against what the manifest resolves for lib/usb on the
    project's own targetDevice, so a change to the module that forgets
    the project fails here instead of silently building something
    else in the IDE.
    """
    errors = []
    cfg_path = USB_PROJECT / "nbproject" / "configurations.xml"
    if not cfg_path.is_file():
        return [f"{USB_PROJECT.name}: no nbproject/configurations.xml"]
    cfg = cfg_path.read_text()

    device = re.search(r"<targetDevice>([^<]+)", cfg)
    if device is None:
        return [f"{USB_PROJECT.name}: no targetDevice in configurations.xml"]
    mcu = device.group(1)
    if mcu.startswith("PIC"):
        mcu = mcu[len("PIC"):]

    mod = manifest.modules.get(USB_MODULE)
    if mod is None:
        return [f"{USB_PROJECT.name}: no manifest module '{USB_MODULE}'"]
    example = manifest.example_for(USB_MODULE, manifest.family_of(mcu).name,
                                   mcu)
    if example is None:
        return [f"{USB_PROJECT.name}: {USB_MODULE} has no example for {mcu}"]

    # The paths the project names, relative to the .X directory.
    depth = len(USB_PROJECT.relative_to(REPO).parts)
    prefix = "/".join([".."] * depth)
    want_sources = {
        f"{prefix}/{epicmanifest.join_module_path(mod.dir, s)}"
        for s in mod.sources
    }
    want_sources |= {
        f"{prefix}/{epicmanifest.join_module_path(mod.dir, s)}"
        for s in example.sources
    }
    want_includes = {f"{prefix}/{epicmanifest.join_module_path(mod.dir, i)}"
                     for i in mod.includes}

    items = _item_paths(cfg)
    got_sources = {p for p in items if p.endswith(".c") and p != "config.c"}
    if got_sources != want_sources:
        errors.append(
            f"{USB_PROJECT.name}: configurations.xml sources disagree with "
            f"the manifest: project-only {sorted(got_sources - want_sources)}, "
            f"manifest-only {sorted(want_sources - got_sources)}")

    inc = re.search(r'key="extra-include-directories" value="([^"]*)"', cfg)
    got_includes = set(inc.group(1).split(";")) if inc else set()
    if got_includes != want_includes:
        errors.append(
            f"{USB_PROJECT.name}: configurations.xml include dirs disagree "
            f"with the manifest: project-only "
            f"{sorted(got_includes - want_includes)}, manifest-only "
            f"{sorted(want_includes - got_includes)}")

    defs = re.search(r'key="define-macros" value="([^"]*)"', cfg)
    got_defs = set(defs.group(1).split(";")) if defs else set()
    fam = manifest.family_of(mcu)
    want_defs = {f"PIC{mcu}", f"FOSC_HZ={fam.fosc_hz}"}
    if got_defs != want_defs:
        errors.append(
            f"{USB_PROJECT.name}: configurations.xml macros disagree with "
            f"the manifest: project-only {sorted(got_defs - want_defs)}, "
            f"manifest-only {sorted(want_defs - got_defs)}")

    # The configuration words: the project's own config TU must carry
    # the manifest example's config, key for key.
    config_c = USB_PROJECT / "config.c"
    if not config_c.is_file():
        errors.append(f"{USB_PROJECT.name}: no config.c")
    else:
        got_config = dict(re.findall(r"#pragma config (\S+) = (\S+)",
                                      config_c.read_text()))
        if got_config != example.config:
            errors.append(
                f"{USB_PROJECT.name}: config.c disagrees with the manifest "
                f"example config: project-only "
                f"{sorted(set(got_config) - set(example.config))}, "
                f"manifest-only "
                f"{sorted(set(example.config) - set(got_config))}, "
                f"value-mismatch "
                f"{sorted(k for k in set(got_config) & set(example.config) if got_config[k] != example.config[k])}")

    for kind, paths in (
        ("itemPath", items),
        ("source root", _elems(cfg)),
        ("project root", _source_roots(
            (USB_PROJECT / "nbproject" / "project.xml").read_text())),
    ):
        for path in sorted(paths):
            if not (USB_PROJECT / path).exists():
                errors.append(
                    f"{USB_PROJECT.name}: missing {kind} '{path}'")
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
    errors.extend(check_usb_project(manifest))
    for error in errors:
        print(f"mplabx audit: {error}")
    if errors:
        print("mplabx audit: reference projects drifted")
        return 1
    print(f"mplabx audit: {len(PROJECTS)} reference projects agree, "
          f"{USB_PROJECT.name} matches the manifest")
    return 0


if __name__ == "__main__":
    sys.exit(main())
