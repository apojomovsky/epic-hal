"""Load, validate, and resolve epic-common/manifest/modules.toml.

This is the only module that knows the manifest's schema. Everything
else (the build driver, the CI matrix, the bundle generator) goes
through the dataclasses here, so a schema change has exactly one place
to land.

Paths are relative to two different roots by design, see the manifest's
own README.md: family-level paths are repo-root-relative because family
data spans directories; module-level paths are relative to that module's
`dir` because module data does not.

Two amendments over the original design live here and are load-bearing:
  - `needs_hal` (module) plus a per-example `hal` override, because
    whether the family HAL is needed is a property of the program, not
    the module: epic-math's library touches no HAL, but its smoke test
    includes the harness.
  - `sources_by_family`, because a module's sources can differ per
    family (epic-math compiles src/pic16 on PIC16, src/pic18 on PIC18).
  - `fosc_hz` (family), because the oscillator frequency is
    family-uniform and the emitter's default must match the Makefiles.
"""
from __future__ import annotations

import dataclasses
import pathlib
import tomllib


class ManifestError(Exception):
    """A manifest that parsed as TOML but is not internally consistent."""


@dataclasses.dataclass(frozen=True)
class ConditionalSource:
    path: str
    variants: list[str]


@dataclasses.dataclass(frozen=True)
class Family:
    name: str
    hal_dir: str
    variants: list[str]
    dfp: str
    fosc_hz: int
    includes: list[str]
    hal_sources: list[str]
    conditional_sources: list[ConditionalSource]


@dataclasses.dataclass(frozen=True)
class Example:
    name: str
    sources: list[str]
    config: dict[str, str]
    hal: bool


@dataclasses.dataclass(frozen=True)
class Module:
    name: str
    dir: str
    sources: list[str]
    sources_by_family: dict[str, list[str]]
    includes: list[str]
    depends_on: list[str]
    needs_hal: bool
    supported: dict[str, list[str]]
    excluded: dict[str, str]
    examples: dict[str, Example]


@dataclasses.dataclass(frozen=True)
class Manifest:
    families: dict[str, Family]
    modules: dict[str, Module]

    def _module(self, name):
        mod = self.modules.get(name)
        if mod is None:
            raise ManifestError(f"unknown module '{name}'")
        return mod

    def resolve_deps(self, module_name: str) -> list[str]:
        """The module plus every transitive dependency, dependencies first.

        Order is deterministic (depth-first over `depends_on` as written),
        so a source list is reproducible and a .hex is byte-comparable
        across runs. Cycles are already rejected at load time.
        """
        seen, ordered = set(), []

        def visit(name):
            if name in seen:
                return
            seen.add(name)
            for dep in self._module(name).depends_on:
                visit(dep)
            ordered.append(name)

        visit(module_name)
        return ordered

    def family_of(self, mcu: str) -> Family:
        for fam in self.families.values():
            if mcu in fam.variants:
                return fam
        known = sorted(v for f in self.families.values() for v in f.variants)
        raise ManifestError(f"unknown MCU '{mcu}'; known parts: {', '.join(known)}")

    def is_supported(self, module_name: str, family: str, mcu: str) -> bool:
        return mcu in self._module(module_name).supported.get(family, [])

    def exclusion_reason(self, module_name: str, mcu: str) -> str | None:
        return self._module(module_name).excluded.get(mcu)

    def example_for(self, module_name: str, family_name: str):
        """This module's example program for a family, or None."""
        return self._module(module_name).examples.get(family_name)

    def uses_hal(self, module_name: str, mcu: str) -> bool:
        """Does this (module, MCU) build need the family HAL?

        The example decides, because the HAL requirement belongs to the
        program: epic-math's library touches no HAL, but its smoke test
        includes the harness. Falls back to the module's own needs_hal
        when there is no example for this family.
        """
        fam = self.family_of(mcu)
        example = self.example_for(module_name, fam.name)
        if example is not None:
            return example.hal
        return self._module(module_name).needs_hal

    def sources_for(self, module_name: str, mcu: str) -> list[str]:
        """Repo-root-relative sources for one (module, MCU) build.

        Order: family HAL sources and applicable conditional sources (only
        when the build uses the HAL), each resolved module's own sources
        plus its per-family sources, then the requested module's example.
        Only the requested module's example is included; a dependency's
        example is a separate program.
        """
        fam = self.family_of(mcu)
        out = []
        if self.uses_hal(module_name, mcu):
            out += list(fam.hal_sources)
            out += [c.path for c in fam.conditional_sources if mcu in c.variants]

        for name in self.resolve_deps(module_name):
            mod = self._module(name)
            out += [f"{mod.dir}/{s}" for s in mod.sources]
            out += [f"{mod.dir}/{s}"
                    for s in mod.sources_by_family.get(fam.name, [])]

        example = self.example_for(module_name, fam.name)
        if example is not None:
            mod = self._module(module_name)
            out += [f"{mod.dir}/{s}" for s in example.sources]

        return _dedupe(out)

    def includes_for(self, module_name: str, mcu: str) -> list[str]:
        """Repo-root-relative include dirs, family first when the HAL is used.

        Family order is preserved verbatim: include/target must precede
        include so the platform header resolves to the real-target
        (volatile-dereference) version, not the host memory-backed one.
        A build that does not use the HAL gets none of the family includes.
        """
        fam = self.family_of(mcu)
        out = list(fam.includes) if self.uses_hal(module_name, mcu) else []
        for name in self.resolve_deps(module_name):
            mod = self._module(name)
            out += [f"{mod.dir}/{i}" for i in mod.includes]
        return _dedupe(out)


def _dedupe(items):
    """Order-preserving de-duplication."""
    seen, out = set(), []
    for item in items:
        if item not in seen:
            seen.add(item)
            out.append(item)
    return out


def _require(table, key, where):
    if key not in table:
        raise ManifestError(f"{where}: missing required key '{key}'")
    return table[key]


def _parse_family(name, table):
    return Family(
        name=name,
        hal_dir=_require(table, "hal_dir", f"families.{name}"),
        variants=list(_require(table, "variants", f"families.{name}")),
        dfp=_require(table, "dfp", f"families.{name}"),
        fosc_hz=_require(table, "fosc_hz", f"families.{name}"),
        includes=list(_require(table, "includes", f"families.{name}")),
        hal_sources=list(_require(table, "hal_sources", f"families.{name}")),
        conditional_sources=[
            ConditionalSource(path=c["path"], variants=list(c["variants"]))
            for c in table.get("conditional_sources", [])
        ],
    )


def _parse_example(module_name, family_name, table, default_hal):
    return Example(
        name=_require(table, "name", f"modules.{module_name}.example.{family_name}"),
        sources=list(_require(table, "sources",
                              f"modules.{module_name}.example.{family_name}")),
        config=dict(table.get("config", {})),
        hal=bool(table.get("hal", default_hal)),
    )


def _parse_module(name, table):
    needs_hal = bool(table.get("needs_hal", True))
    return Module(
        name=name,
        dir=_require(table, "dir", f"modules.{name}"),
        sources=list(_require(table, "sources", f"modules.{name}")),
        sources_by_family={
            fam: list(paths)
            for fam, paths in table.get("sources_by_family", {}).items()
        },
        includes=list(table.get("includes", [])),
        depends_on=list(table.get("depends_on", [])),
        needs_hal=needs_hal,
        supported={
            fam: list(variants)
            for fam, variants in table.get("supported", {}).items()
        },
        excluded=dict(table.get("excluded", {})),
        examples={
            fam: _parse_example(name, fam, tbl, needs_hal)
            for fam, tbl in table.get("example", {}).items()
        },
    )


def _check_cycles(modules):
    """Depth-first search for a dependency cycle, reporting the path."""
    WHITE, GREY, BLACK = 0, 1, 2
    colour = {name: WHITE for name in modules}

    def visit(name, stack):
        if colour[name] == GREY:
            joined = " -> ".join(stack + [name])
            raise ManifestError(f"dependency cycle: {joined}")
        if colour[name] == BLACK:
            return
        colour[name] = GREY
        for dep in modules[name].depends_on:
            visit(dep, stack + [name])
        colour[name] = BLACK

    for name in modules:
        visit(name, [])


def _validate(manifest):
    for mod in manifest.modules.values():
        for dep in mod.depends_on:
            if dep not in manifest.modules:
                raise ManifestError(
                    f"modules.{mod.name}: depends_on unknown module '{dep}'"
                )
        for fam_name in mod.sources_by_family:
            if fam_name not in manifest.families:
                raise ManifestError(
                    f"modules.{mod.name}.sources_by_family: "
                    f"unknown family '{fam_name}'"
                )
        for fam_name in mod.examples:
            if fam_name not in manifest.families:
                raise ManifestError(
                    f"modules.{mod.name}.example: unknown family '{fam_name}'"
                )
        for fam_name, variants in mod.supported.items():
            fam = manifest.families.get(fam_name)
            if fam is None:
                raise ManifestError(
                    f"modules.{mod.name}.supported: unknown family '{fam_name}'"
                )
            for v in variants:
                if v not in fam.variants:
                    raise ManifestError(
                        f"modules.{mod.name}.supported.{fam_name}: "
                        f"'{v}' is not a variant of {fam_name} ({fam.variants})"
                    )
                if v in mod.excluded:
                    raise ManifestError(
                        f"modules.{mod.name}: '{v}' is both supported and excluded"
                    )
        # An example may name a family; its config is family-scoped, so no
        # per-family subkeys to validate here.
    _check_cycles(manifest.modules)


def load(path: pathlib.Path) -> Manifest:
    """Parse and validate the manifest at `path`."""
    with open(path, "rb") as fh:
        raw = tomllib.load(fh)

    manifest = Manifest(
        families={
            name: _parse_family(name, table)
            for name, table in raw.get("families", {}).items()
        },
        modules={
            name: _parse_module(name, table)
            for name, table in raw.get("modules", {}).items()
        },
    )
    _validate(manifest)
    return manifest


def default_path() -> pathlib.Path:
    """The manifest's location relative to this file."""
    return (
        pathlib.Path(__file__).resolve().parents[1]
        / "epic-common" / "manifest" / "modules.toml"
    )
