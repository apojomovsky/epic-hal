# scripts/epicurus_init.py
"""Project scaffolder for Epicurus: manifest in, ready main.c + Makefile +
patched MPLAB X .X out. Pure helpers here; scripts/epicurus.py is the CLI."""
from __future__ import annotations
import bundlegen, epicmanifest


class SelectionError(Exception):
    """A (family, part, module) selection the manifest says does not build."""


def hal_pseudo_module(manifest: epicmanifest.Manifest, fam: epicmanifest.Family) -> str:
    """The family's bare-HAL 'pseudo-module' (dir == hal_dir): the module
    whose example carries the working #pragma config set for the family."""
    for name, mod in manifest.modules.items():
        if mod.dir == fam.hal_dir:
            return name
    raise SelectionError(f"no HAL pseudo-module for {fam.name} (dir={fam.hal_dir})")


def resolve_selection(manifest, family_name, part, modules) -> list[str]:
    """Expand `modules` to an ordered, deduped (selected + deps) list,
    refusing any module unsupported on `part` with the manifest's reason."""
    if family_name not in manifest.families:
        raise SelectionError(f"unknown family {family_name!r}; "
                             f"known: {', '.join(sorted(manifest.families))}")
    fam = manifest.families[family_name]
    if part not in fam.variants:
        raise SelectionError(f"{part} is not a {family_name} part; "
                             f"known: {', '.join(fam.variants)}")
    known = set(bundlegen.modules_for_family(manifest, family_name))
    known.add(hal_pseudo_module(manifest, fam))
    ordered: list[str] = []
    seen: set[str] = set()
    for m in modules:
        if m not in known:
            raise SelectionError(f"unknown module {m!r} for {family_name}")
        for name in manifest.resolve_deps(m):
            if name in seen:
                continue
            if not manifest.is_supported(name, family_name, part):
                reason = manifest.exclusion_reason(name, part) or ""
                sup = manifest.modules[name].supported.get(family_name, [])
                raise SelectionError(
                    f"{name} is not supported on {part}"
                    + (f" ({reason})" if reason else "")
                    + f". Supported on {family_name}: {', '.join(sup) or 'none'}.")
            seen.add(name)
            ordered.append(name)
    return ordered
