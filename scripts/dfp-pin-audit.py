#!/usr/bin/env python3
"""DFP-pin audit: the device-pack version pinned in the toolchain image,
the manifest and the reference MPLAB X projects must not drift apart.

Two surfaces, checked by pure text parsing (no container, no XC8):

- The manifest's per-family dfp_version must equal the Dockerfile ARG
  for that family's pack, because scripts/bundlegen.py emits it into the
  bundle's QUICKSTART download URL and EPIC_HAL_DFP_VERSION. A mismatch
  tells a user to fetch a pack the image does not carry.

- Each reference project's three pins must agree with each other and
  name the pack its family uses: default.Pack.dfplocation in
  Makefile-genesis.properties, DFP_DIR in Makefile-local-default.mk, and
  the <pack .../> element in configurations.xml.

The projects pin the MPLAB X tree, not the XC8 tree, so their version is
deliberately NOT compared against the ARG: DEVELOPMENT.md's "The two
device-pack trees" records that the bundled demo .X is built against the
MPLAB X tree by the isolated bundle gate, where PIC16Fxxx_DFP is the
version MPLAB X ships (1.8.167 on 6.35) rather than the ARG's. Requiring
equality there would break that gate, the mistake epic-hal#218 made and
corrected. Runs in CI and `make audit`.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
import bundlegen  # noqa: E402
import epicmanifest as manifest_lib  # noqa: E402

DOCKERFILE = REPO / "docker" / "ci-toolchain" / "Dockerfile"

# The Dockerfile downloads each pack at the version named by its ARG,
# pairing the pack name with the ARG in one loop. Anchored on the
# Microchip.<name>_DFP shape so other `"x:${Y}"` strings (PATH entries,
# for one) are not mistaken for pack pairs.
PACK_ARG_RE = re.compile(r'"(Microchip\.[\w-]+_DFP):\$\{(\w+)\}"')
ARG_RE = re.compile(r"^ARG (\w+)=(.*)$", re.M)

# A pack path ends in <PackName>/<version>, in the two path-valued pins.
PATH_PIN_RE = re.compile(r"(?:dfplocation=|DFP_DIR=)(\S+)")
PACK_XML_RE = re.compile(r'<pack name="([^"]+)" vendor="([^"]+)" version="([^"]+)"')

# Every reference project must carry a pack pin in all three places. A
# project missing one is the epic-hal#229 defect class: pic16f5x and
# pic16f7x shipped a Makefile-genesis.properties that was a stray copy of
# Makefile-variables.mk, so their dfplocation never existed.
PIN_SOURCES = ("Makefile-genesis.properties", "Makefile-local-default.mk",
               "configurations.xml")


def dockerfile_arg_versions() -> dict[str, str]:
    """ARG name -> value, from the Dockerfile.

    Values are stripped: a trailing space or CRLF on the ARG line would
    otherwise read as a version mismatch against the manifest.
    """
    return {name: value.strip() for name, value in ARG_RE.findall(
        DOCKERFILE.read_text())}


def dockerfile_pack_args() -> dict[str, str]:
    """Pack name -> ARG name, from the download loop's literal pairs."""
    text = DOCKERFILE.read_text()
    return {pack: arg for pack, arg in PACK_ARG_RE.findall(text)}


def split_pack_path(path: str) -> tuple[str, str, str]:
    """(vendor, pack, version) from .../packs/<Vendor>/<Pack>/<version>.

    A path too short to hold all three yields empty strings rather than
    raising: a malformed pin is the caller's to report.
    """
    parts = path.rstrip("/").split("/")
    if len(parts) < 3:
        return "", "", ""
    return parts[-3], parts[-2], parts[-1]


def bare_pack(pack: str) -> str:
    """The pack name without its 'Microchip.' vendor prefix.

    The manifest and the Dockerfile carry the vendor-qualified form
    (`Microchip.PIC16Fxxx_DFP`); the MPLAB X project files carry the bare
    one (`PIC16Fxxx_DFP`). Same pack, two spellings.
    """
    return pack.split(".", 1)[1] if pack.startswith("Microchip.") else pack


def project_pins(project: pathlib.Path) -> list[tuple[str, str, str, str]]:
    """Every pin in one reference project as (source, pack, vendor, version).

    The three files spell the pack three ways: the path-valued pins carry
    it as `.../packs/<Vendor>/<Pack>/<version>`, the XML element has
    name/vendor/version attributes. Both are normalized here so the
    caller compares one shape.

    A project missing a pin is not reported here: the caller compares the
    set it gets against the expected sources, so a dropped pin surfaces as
    a missing entry rather than passing silently.
    """
    nb = project / "nbproject"
    pins = []
    for name in ("Makefile-genesis.properties", "Makefile-local-default.mk"):
        path = nb / name
        if not path.is_file():
            continue
        # Commented-out pins must not count: MPLAB X regeneration and hand
        # edits both leave them behind, and a stale one would either
        # trip the disagreement check or mask a missing real pin.
        lines = [line for line in path.read_text().splitlines()
                 if not line.lstrip().startswith("#")]
        for value in PATH_PIN_RE.findall("\n".join(lines)):
            vendor, pack, version = split_pack_path(value)
            if pack:
                pins.append((name, pack, vendor, version))
    xml = nb / "configurations.xml"
    if xml.is_file():
        for pack, vendor, version in PACK_XML_RE.findall(xml.read_text()):
            pins.append(("configurations.xml", pack, vendor, version))
    return pins


def check(m, repo: pathlib.Path, argv: dict[str, str],
          pack_arg: dict[str, str], family: str | None = None) -> list[str]:
    """Every pin mismatch, as printable lines. Empty means the pins agree.

    Split from `main` so the failure branches are unit-testable against a
    fixture tree without going through argv and the real checkout.
    """
    problems = []

    # Surface 1: manifest dfp_version against the image's ARG.
    for name, fam in sorted(m.families.items()):
        if family is not None and name != family:
            continue
        if not fam.dfp_version:
            continue
        arg = pack_arg.get(fam.dfp)
        if arg is None:
            problems.append(
                f"{name}: dfp {fam.dfp} is not one of the Dockerfile's "
                f"packs ({', '.join(sorted(pack_arg))})")
            continue
        want = argv.get(arg)
        if want != fam.dfp_version:
            problems.append(
                f"{name}: manifest dfp_version {fam.dfp_version} != "
                f"Dockerfile {arg}={want} for {fam.dfp}")

    # Surface 2: each reference project's pins agree and name its pack.
    for name, fam in sorted(m.families.items()):
        if family is not None and name != family:
            continue
        project = repo / bundlegen.reference_project_dir(m, name)
        if not project.is_dir():
            problems.append(
                f"{name}: no reference project at "
                f"{project.relative_to(repo)} (a family without one is "
                "invisible to this audit and to the bundle gate)")
            continue
        pins = project_pins(project)
        if not pins:
            problems.append(f"{name}: no DFP pin found under "
                            f"{project.relative_to(repo)}/nbproject")
            continue
        found = {src for src, _pack, _vendor, _ver in pins}
        for missing in (s for s in PIN_SOURCES if s not in found):
            problems.append(
                f"{name}: {project.name} carries no DFP pin in {missing}")
        packs = {pack for _src, pack, _vendor, _ver in pins}
        vendors = {vendor for _src, _pack, vendor, _ver in pins}
        versions = {ver for _src, _pack, _vendor, ver in pins}
        if packs != {bare_pack(fam.dfp)}:
            problems.append(
                f"{name}: {project.name} pins pack "
                f"{', '.join(sorted(packs))}, family uses {fam.dfp}")
        if vendors != {"Microchip"}:
            problems.append(
                f"{name}: {project.name} pins vendor "
                f"{', '.join(sorted(vendors))}, expected Microchip")
        if len(versions) > 1:
            detail = ", ".join(f"{src}={ver}" for src, _p, _v, ver in pins)
            problems.append(
                f"{name}: {project.name} pins disagree ({detail})")

    return problems


def main() -> int:
    m = manifest_lib.load(manifest_lib.default_path())
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--family", choices=sorted(m.families), default=None,
                    help="only this manifest family (the sharded CI jobs)")
    args = ap.parse_args()

    argv = dockerfile_arg_versions()
    pack_arg = dockerfile_pack_args()
    if not pack_arg:
        print("dfp-pin audit: no pack/ARG pairs parsed from the Dockerfile "
              "(the download loop changed shape)")
        return 1

    problems = check(m, REPO, argv, pack_arg, args.family)
    if problems:
        for line in problems:
            print(line)
        print(f"dfp-pin audit: {len(problems)} mismatch(es)")
        return 1
    print("dfp-pin audit: manifest and reference-project DFP pins agree")
    return 0


if __name__ == "__main__":
    sys.exit(main())
