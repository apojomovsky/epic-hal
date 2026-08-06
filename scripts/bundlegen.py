#!/usr/bin/env python3
"""Generate a self-contained, per-family source bundle.

A bundle is what someone outside this repo actually consumes: one
family's HAL, epic-common, every module that builds on that family, and
a set of generated files that tell their build system what to compile.

Module dependencies are flattened here, at generation time, so the
emitted epicurus.mk is a table of precomputed variables plus a guard.
Recursive dependency resolution in GNU make is possible and unpleasant;
doing it in Python and emitting the answer is neither.

See docs/superpowers/specs/2026-08-05-distribution-design.md.
"""
from __future__ import annotations

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import epicmanifest  # noqa: E402


class BundleError(Exception):
    """A bundle that cannot be generated as asked."""


def _family(manifest, family_name):
    fam = manifest.families.get(family_name)
    if fam is None:
        known = ", ".join(sorted(manifest.families))
        raise BundleError(f"unknown family '{family_name}'; known: {known}")
    return fam


def modules_for_family(manifest, family_name: str) -> list[str]:
    """Every module with at least one supported part in this family.

    Excludes a module whose dir equals the family's own hal_dir: that
    shape (epic-pic16f193x-firmware in the real manifest) is CI-coverage
    plumbing for a family with no real modules yet, plan 1's manifest-
    equivalence stand-in for the family's bare-HAL build, not a library
    a bundle consumer would ever ask for by name. Excluding it is what
    keeps PIC16F193X a genuine HAL-only bundle.
    """
    fam = _family(manifest, family_name)
    return sorted(
        name for name, mod in manifest.modules.items()
        if mod.supported.get(family_name) and mod.dir != fam.hal_dir
    )


def files_for_family(manifest, family_name: str) -> list[str]:
    """Every repo-root-relative file the bundle must copy.

    Sources only. Documentation and headers are copied wholesale by
    make_bundle.py's directory walk; this is the list that has to be
    exactly right because it is what the emitted epicurus.mk names.
    """
    fam = _family(manifest, family_name)
    files = set(fam.hal_sources)
    files |= {c.path for c in fam.conditional_sources}

    for name in modules_for_family(manifest, family_name):
        mod = manifest.modules[name]
        files |= {f"{mod.dir}/{s}" for s in mod.sources}
        files |= {f"{mod.dir}/{s}" for s in mod.sources_by_family.get(family_name, [])}
        example = mod.examples.get(family_name)
        if example is not None:
            files |= {f"{mod.dir}/{s}" for s in example.sources}

    return sorted(files)
