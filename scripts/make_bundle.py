#!/usr/bin/env python3
"""Assemble a per-family Epicurus bundle (a build output, attached to a
GitHub Release, never committed): copy one family's HAL, epic-common, and
every module that builds on it into a self-contained tree with the
generated consumer files. Called by CI's emit step and
scripts/ci-local-emit.py.

Usage: python3 scripts/make_bundle.py --family <FAMILY> --version <VERSION>
"""
from __future__ import annotations

import argparse
import os
import pathlib
import shutil
import sys
import tarfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import bundlegen  # noqa: E402
import epicmanifest  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parents[1]

# Consumer-bundle content policy: the manifest is the single source of
# truth for what builds on a real target. Sources are exactly the
# manifest-resolved target list (bundlegen.files_for_family), headers
# come from the manifest include dirs, docs are the living consumer
# files (README.md/MANUAL.md at each module/HAL/epic-common root), and
# the generated consumer files, the reference .X (for `epicurus init`),
# the CLI, and the manifest ship so the bundle is self-sufficient.
# Nothing else: no tests, no sim/mdb backends, no design docs, no mcu/
# scaffolding. The gates below make that load-bearing.
DOC_NAMES = {"README.md", "MANUAL.md"}
HEADER_SKIP = {"host", "tests", "sim", "mdb", "build", "build18",
               "__pycache__", "third_party"}


def _slug(family_name: str, manifest) -> str:
    return manifest.families[family_name].hal_dir.removesuffix("-hal")


def _is_sim_mdb(rel: str) -> bool:
    """True when a bundle-relative path names a sim or mdb artifact.

    The split src/ layout mirrors the build environments: src/sim/ holds
    host-simulation sources, src/mdb/ holds the MPLAB SIM gate harness,
    and every such file carries _sim/_mdb in its name (including the
    include/pic16f87xa_sim.h API headers) or a sim_ fixture prefix
    (tests/sim_bus.c and friends). None of it is consumer-facing.
    """
    parts = rel.split("/")
    if "_sim" in rel or "_mdb" in rel:
        return True
    if any(part.startswith("sim_") for part in parts):
        return True
    return any(parts[i] == "src" and parts[i + 1] in ("sim", "mdb")
               for i in range(len(parts) - 1))


def _sim_mdb_offenders(paths) -> list[str]:
    """Bundle-relative paths that must never ship, in sorted order."""
    return sorted(p for p in paths if _is_sim_mdb(p))


def _nonconsumer_offenders(paths) -> list[str]:
    """Bundle-relative paths that must never ship, in sorted order.

    The release gate's load-bearing policy: a consumer bundle carries no
    tests, no host-simulation or MPLAB-SIM backends, no host include
    dir, and no design docs (ARCHITECTURE.md). Anything matching here
    fails the bundle build instead of silently shipping.
    """
    out = []
    for p in paths:
        parts = p.split("/")
        if "tests" in parts or "host" in parts or _is_sim_mdb(p):
            out.append(p)
        elif p.endswith("ARCHITECTURE.md"):
            out.append(p)
    return sorted(set(out))


def _copy_file(src: pathlib.Path, dst: pathlib.Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


def _copy_sources(manifest, family_name: str, root: pathlib.Path) -> None:
    """The manifest-resolved target sources, exactly as epicurus.mk names them."""
    for f in bundlegen.files_for_family(manifest, family_name):
        src = REPO / f
        if not src.is_file():
            sys.exit(f"error: referenced source {f} does not exist")
        _copy_file(src, root / f)


def _copy_headers(manifest, family_name: str, root: pathlib.Path) -> None:
    """Headers from the manifest include dirs, minus host/tests/sim/mdb."""
    fam = manifest.families[family_name]
    dirs = list(fam.includes)
    for name in bundlegen.modules_for_family(manifest, family_name):
        mod = manifest.modules[name]
        dirs += [f"{mod.dir}/{i}" for i in mod.includes]
    seen = set()
    for d in dirs:
        d = os.path.normpath(d)
        if d in seen:
            continue
        seen.add(d)
        src_dir = REPO / d
        if not src_dir.is_dir():
            continue
        for path in sorted(src_dir.rglob("*.h")):
            full = path.relative_to(REPO).as_posix()
            # Check the full repo-relative path: an include dir may
            # itself be epic-math/tests, and sim API headers can sit in
            # the base include dir (pic16f87xa_sim.h) rather than under
            # src/sim/.
            if any(p in HEADER_SKIP for p in full.split("/")):
                continue
            if _is_sim_mdb(full):
                continue
            _copy_file(path, root / d / path.relative_to(src_dir))


def _copy_docs(manifest, family_name: str, root: pathlib.Path) -> None:
    """The living consumer docs at each module/HAL/epic-common root."""
    fam = manifest.families[family_name]
    dirs = ["epic-common", fam.hal_dir]
    dirs += [manifest.modules[name].dir
             for name in bundlegen.modules_for_family(manifest, family_name)]
    for d in dirs:
        for name in DOC_NAMES:
            src = REPO / d / name
            if src.is_file():
                _copy_file(src, root / d / name)


def _copy_project(src: pathlib.Path, dst: pathlib.Path) -> None:
    """Copy an MPLAB X .X project wholesale.

    Unlike a source tree, everything here matters: nbproject holds .xml,
    .mk, and .properties files, and a project missing any of them opens
    broken. Only build output is skipped.
    """
    for path in sorted(src.rglob("*")):
        parts = path.relative_to(src).parts
        if "build" in parts or "dist" in parts or "__pycache__" in parts:
            continue
        if path.is_dir():
            continue
        target = dst / path.relative_to(src)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)


def _quickstart(manifest, family_name: str, version: str) -> str:
    """QUICKSTART.md, or a HAL-only fallback for a family with no module.

    PIC16F193X's only manifest entry for its family is the family-HAL-
    wrapper pseudo-module (bundlegen.modules_for_family excludes it), so
    there is no real module to write a worked EPICURUS_MODULES example
    for; emit_quickstart_md raises BundleError in that case rather than
    naming a module that does not exist for a consumer.
    """
    try:
        return bundlegen.emit_quickstart_md(manifest, family_name, version)
    except bundlegen.BundleError:
        fam = manifest.families[family_name]
        return "\n".join([
            f"# Quick start, Epicurus {version} ({family_name})",
            "",
            "This bundle is HAL-only: no higher-level module is wired up",
            "for this family yet. There is no `EPICURUS_MODULES` value to",
            "give a worked example for.",
            "",
            "Build the HAL directly against `epic-common/src/core/",
            "epic_harness_target.c` and this bundle's own peripheral",
            "sources under the family's HAL directory; see the family's",
            "own `README.md`/`MANUAL.md` for a real-target example.",
            "",
            f"Supported parts in this bundle: {', '.join(fam.variants)}.",
            "",
        ]) + "\n"


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--family", required=True)
    ap.add_argument("--version", required=True)
    ap.add_argument("--out-dir", default="bundles")
    ap.add_argument("--no-tarball", action="store_true")
    args = ap.parse_args()

    manifest = epicmanifest.load(epicmanifest.default_path())
    try:
        fam = manifest.families[args.family]
    except KeyError:
        sys.exit(
            f"error: unknown family '{args.family}'; "
            f"known: {', '.join(sorted(manifest.families))}"
        )

    slug = _slug(args.family, manifest)
    root = REPO / args.out_dir / f"epicurus-{slug}-{args.version}"
    if root.exists():
        shutil.rmtree(root)
    root.mkdir(parents=True)

    # Consumer content: manifest-resolved target sources, headers from
    # the include dirs, and the living consumer docs. No tests, no
    # sim/mdb backends, no design docs, no mcu/ scaffolding (the gates
    # below make that load-bearing).
    modules = bundlegen.modules_for_family(manifest, args.family)
    _copy_sources(manifest, args.family, root)
    _copy_headers(manifest, args.family, root)
    _copy_docs(manifest, args.family, root)

    # Generated files.
    (root / "epicurus.mk").write_text(
        bundlegen.emit_epicurus_mk(manifest, args.family, args.version))
    (root / "epicurus-sources.json").write_text(
        bundlegen.emit_sources_json(manifest, args.family, args.version))
    (root / "SUPPORT.md").write_text(
        bundlegen.emit_support_md(manifest, args.family, args.version))
    (root / "QUICKSTART.md").write_text(
        _quickstart(manifest, args.family, args.version))
    (root / "MPLABX.md").write_text(
        bundlegen.emit_mplabx_md(manifest, args.family, args.version))
    (root / "VERSION").write_text(args.version + "\n")
    shutil.copy2(REPO / "LICENSE", root / "LICENSE")
    # Ship the `epicurus` CLI inside the bundle so a consumer can run
    # `./epicurus init` with no install. epicurus.py becomes `epicurus`
    # (no extension, executable); the three helper modules it imports
    # from its own directory ship alongside it so `import epicmanifest`,
    # `import epicurus_init` (which imports `bundlegen`) all resolve from
    # the bundle root.
    shutil.copy2(REPO / "scripts" / "epicurus.py", root / "epicurus")
    (root / "epicurus").chmod(0o755)
    for mod in ("epicurus_init.py", "epicmanifest.py", "bundlegen.py"):
        shutil.copy2(REPO / "scripts" / mod, root / mod)
    # _copy_tree only ships .c/.h/.md/.txt under epic-common/, so the
    # manifest (.toml) is dropped. _find_manifest looks for it at
    # epic-common/manifest/modules.toml; copy it there explicitly.
    manifest_dst = root / "epic-common" / "manifest" / "modules.toml"
    manifest_dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(epicmanifest.default_path(), manifest_dst)

    project_src = REPO / bundlegen.reference_project_dir(manifest, args.family)
    if not project_src.is_dir():
        sys.exit(f"error: no reference project at {project_src.relative_to(REPO)}")
    _copy_project(project_src, root / "examples" / "epicurus-demo.X")

    # Gate: sim/mdb and other non-consumer files must never ship in a
    # release bundle. They are CI plumbing (host-simulation backends,
    # MPLAB SIM gate harnesses, tests, host include dirs, design docs),
    # not consumer-facing, and the allowlist copy above excludes them;
    # this assertion turns an accidental inclusion into a hard build
    # error instead of a silent packaging bug.
    rel_paths = [p.relative_to(root).as_posix() for p in root.rglob("*")]
    offenders = _sim_mdb_offenders(rel_paths) + _nonconsumer_offenders(rel_paths)
    if offenders:
        sys.exit("error: non-consumer files must never ship in a release bundle:\n  " +
                 "\n  ".join(sorted(set(offenders))))

    # Every source epicurus.mk names must actually be in the bundle. A
    # bundle that ships a source list referring to a file it does not
    # contain is the exact failure mode packaging introduces.
    missing = [
        f for f in bundlegen.files_for_family(manifest, args.family)
        if not (root / f).exists()
    ]
    if missing:
        sys.exit("error: bundle is missing files it references:\n  " +
                 "\n  ".join(missing))

    print(f"bundle: {root.relative_to(REPO)} ({len(modules)} modules)")

    if not args.no_tarball:
        # Not root.with_suffix(".tar.gz"): pathlib treats the version's
        # own dots (v0.1.0) as suffixes and truncates the name, verified
        # against a real run (produced epicurus-pic16f193x-v0.1.tar.gz,
        # silently dropping the ".0").
        tarball = root.parent / f"{root.name}.tar.gz"
        with tarfile.open(tarball, "w:gz") as tf:
            tf.add(root, arcname=root.name)
        print(f"tarball: {tarball.relative_to(REPO)}")


if __name__ == "__main__":
    main()
