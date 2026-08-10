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
    after: str | None = None


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
    harness_src: str | None = None


@dataclasses.dataclass(frozen=True)
class SimVariant:
    """The HARNESS=sim build: a bounded, self-reporting firmware used to
    drive MPLAB SIM (see sim-tests.yml), distinct from the real-target
    HARNESS=target build the same example otherwise produces.

    harness_src replaces the family's harness_src at its recorded
    position in hal_sources (not appended: XC8 lays out psects in link
    order). sources, when set, replaces the target example's own
    sources entirely (pic16f193x-hal's sim variant links a different
    diagnostic program than its target example); when unset the target
    example's sources are reused unchanged (epic-tick's sim variant
    links the same example_tick.c, only the harness and config differ).
    config is a full override, not a merge: PIC16's WDTE / PIC18's WDT
    is off in every sim variant seen so far, and restating the whole
    config table keeps this dataclass's contract simple (no partial-
    override merge logic to get subtly wrong).
    """
    name: str
    harness_src: str
    config: dict[str, str]
    sources: list[str] | None = None


@dataclasses.dataclass(frozen=True)
class Example:
    name: str
    sources: list[str]
    config: dict[str, str]
    hal: bool
    sim: SimVariant | None = None
    # Per-MCU overrides: a variant replaces the family example for one
    # MCU (flash budgets differ within a family, e.g. the 4K-word
    # 16F873A/16F874A cannot hold epic-math's golden-vector replay).
    # example_for(module, family, mcu) resolves the variant.
    variants: dict[str, Example] | None = None


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

    def example_for(self, module_name: str, family_name: str,
                    mcu: str | None = None):
        """This module's example program for a family, or None.

        With mcu, a per-MCU variant override (Example.variants) wins;
        without one the family example is returned unchanged.
        """
        example = self._module(module_name).examples.get(family_name)
        if example is not None and mcu is not None and example.variants:
            return example.variants.get(mcu, example)
        return example

    def uses_hal(self, module_name: str, mcu: str) -> bool:
        """Does this (module, MCU) build need the family HAL?

        The example decides, because the HAL requirement belongs to the
        program: epic-math's library touches no HAL, but its smoke test
        includes the harness. Falls back to the module's own needs_hal
        when there is no example for this family.
        """
        fam = self.family_of(mcu)
        example = self.example_for(module_name, fam.name, mcu)
        if example is not None:
            return example.hal
        return self._module(module_name).needs_hal

    def sim_variant_for(self, module_name: str, family_name: str) -> SimVariant | None:
        """This module's HARNESS=sim variant for a family, or None.

        Only three (module, family) pairs have one today: epic-tick on
        PIC16F87XA and PIC18Fxx5x, and the PIC16F193X bare-HAL firmware
        module, mirroring the three sim-tests.yml legs that exist.
        """
        example = self.example_for(module_name, family_name)
        return None if example is None else example.sim

    def sources_for(self, module_name: str, mcu: str, variant: str = "target") -> list[str]:
        """Repo-root-relative sources for one (module, MCU) build.

        Order: family HAL sources with applicable conditional sources
        spliced in at their recorded position (only when the build uses
        the HAL), each resolved module's own sources plus its per-family
        sources, then the requested module's example. Only the requested
        module's example is included; a dependency's example is a
        separate program.

        Conditional-source position matters, not just presence: XC8 lays
        out psects in link order, so a source inserted at the wrong point
        in the list changes the .hex even though the same files compile.
        A conditional with `after` set is spliced in right after that
        sibling path; one with no `after` is appended at the end (the
        historical default, still correct for the one family that needs
        it that way).

        variant="sim" (see sim-tests.yml) swaps the family's harness_src
        for the sim variant's own harness_src, at the same position
        (same link-order reasoning as conditional sources), and uses the
        sim variant's own sources in place of the example's, when it
        overrides them.
        """
        fam = self.family_of(mcu)
        sim = self.sim_variant_for(module_name, fam.name) if variant == "sim" else None
        if variant == "sim" and sim is None:
            raise ManifestError(
                f"{module_name} has no sim variant for {fam.name}"
            )

        out = []
        if self.uses_hal(module_name, mcu):
            applicable = [c for c in fam.conditional_sources if mcu in c.variants]
            for hal_src in fam.hal_sources:
                if sim is not None and hal_src == fam.harness_src:
                    out.append(sim.harness_src)
                else:
                    out.append(hal_src)
                for c in applicable:
                    if c.after == hal_src:
                        out.append(c.path)
            out += [c.path for c in applicable if c.after is None]

        for name in self.resolve_deps(module_name):
            mod = self._module(name)
            out += [f"{mod.dir}/{s}" for s in mod.sources]
            out += [f"{mod.dir}/{s}"
                    for s in mod.sources_by_family.get(fam.name, [])]

        example = self.example_for(module_name, fam.name, mcu)
        if example is not None:
            mod = self._module(module_name)
            example_sources = example.sources
            if sim is not None and sim.sources is not None:
                example_sources = sim.sources
            out += [f"{mod.dir}/{s}" for s in example_sources]

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
            ConditionalSource(path=c["path"], variants=list(c["variants"]),
                              after=c.get("after"))
            for c in table.get("conditional_sources", [])
        ],
        harness_src=table.get("harness_src"),
    )


def _parse_sim_variant(module_name, family_name, table):
    if table is None:
        return None
    where = f"modules.{module_name}.example.{family_name}.sim"
    return SimVariant(
        name=_require(table, "name", where),
        harness_src=_require(table, "harness_src", where),
        config=dict(_require(table, "config", where)),
        sources=(list(table["sources"]) if "sources" in table else None),
    )


def _parse_example(module_name, family_name, table, default_hal):
    variants = table.get("variants", {})
    return Example(
        name=_require(table, "name", f"modules.{module_name}.example.{family_name}"),
        sources=list(_require(table, "sources",
                              f"modules.{module_name}.example.{family_name}")),
        config=dict(table.get("config", {})),
        hal=bool(table.get("hal", default_hal)),
        sim=_parse_sim_variant(module_name, family_name, table.get("sim")),
        variants={vname: _parse_example(
            module_name, f"{family_name}.variants.{vname}", vtable, default_hal)
            for vname, vtable in variants.items()} or None,
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
        for fam_name, example in mod.examples.items():
            if fam_name not in manifest.families:
                raise ManifestError(
                    f"modules.{mod.name}.example: unknown family '{fam_name}'"
                )
            if example.sim is not None and manifest.families[fam_name].harness_src is None:
                raise ManifestError(
                    f"modules.{mod.name}.example.{fam_name}.sim: "
                    f"families.{fam_name} has no harness_src to swap"
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
