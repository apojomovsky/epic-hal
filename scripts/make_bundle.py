#!/usr/bin/env python3
"""Assemble a per-family Epicurus bundle.

Copies one family's HAL, epic-common, and every module that builds on
that family into a self-contained tree, then writes the generated
consumer files into it. Nothing here is committed: bundles are build
outputs, attached to a GitHub Release.

Usage:
  python3 scripts/make_bundle.py --family PIC16F87XA --version v0.1.0
"""
from __future__ import annotations

import argparse
import pathlib
import shutil
import sys
import tarfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import bundlegen  # noqa: E402
import epicmanifest  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parents[1]

# Copied wholesale from any directory a bundle includes. Sources are
# authoritative in the manifest; these are the human-facing files that
# would be busywork to enumerate there.
DOC_NAMES = {"README.md", "MANUAL.md", "LICENSE"}
SKIP_DIRS = {"build", "build18", "__pycache__", ".git", "third_party"}


def _slug(family_name: str, manifest) -> str:
    return manifest.families[family_name].hal_dir.removesuffix("-hal")


def _is_mplabx_dir(part: str) -> bool:
    """A now-deleted-Makefile mcu/*-mplabx/ subdirectory, not the mcu/
    directory itself. Several modules (epic-adcfilter, epic-fsm,
    epic-pid, epic-encoder) name a real example source directly under
    mcu/ (mcu/target_sizecheck.c, per epic-common/manifest/modules.toml),
    so skipping every "mcu" path component would drop a file
    files_for_family names, confirmed the hard way: the first bundle
    generation run failed the missing-files check on exactly those four.
    """
    return part.endswith("-mplabx")


def _copy_tree(src: pathlib.Path, dst: pathlib.Path) -> None:
    """Copy a module or HAL directory, minus build output and mcu/*-mplabx/."""
    for path in sorted(src.rglob("*")):
        rel_parts = path.relative_to(src).parts
        if any(part in SKIP_DIRS or _is_mplabx_dir(part) for part in rel_parts):
            continue
        if path.is_dir():
            continue
        if path.suffix not in {".c", ".h", ".md", ".txt"} and path.name not in DOC_NAMES:
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

    # Source trees.
    _copy_tree(REPO / "epic-common", root / "epic-common")
    _copy_tree(REPO / fam.hal_dir, root / fam.hal_dir)
    modules = bundlegen.modules_for_family(manifest, args.family)
    for name in modules:
        _copy_tree(REPO / manifest.modules[name].dir, root / manifest.modules[name].dir)

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
