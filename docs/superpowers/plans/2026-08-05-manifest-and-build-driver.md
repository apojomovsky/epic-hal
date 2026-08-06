# Manifest and Build Driver Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace 29 hand-maintained `mcu/*-mplabx/Makefile`s and the `KNOWN_BROKEN` literal in `scripts/ci-discover-xc8-matrix.py` with one declarative manifest plus a Python build driver, proving equivalence by byte-identical `.hex` output.

**Architecture:** A TOML manifest (`epic-common/manifest/modules.toml`) holds per-family HAL source sets and per-module sources, dependencies, examples, config words, and supported parts. `scripts/epic-build.py` reads it, resolves module dependencies transitively, and emits a self-contained POSIX `sh` script of `xc8-cc` invocations. Resolution runs where python3 exists; execution needs only `sh` and `xc8-cc`. The manifest is bootstrapped mechanically from the existing Makefiles (not hand-transcribed), and every migration step is gated on the new path producing `.hex` files byte-identical to the old path's.

**Tech Stack:** Python 3.11+ (stdlib only: `tomllib`, `argparse`, `json`, `unittest`), GNU make (for bootstrap extraction only), POSIX `sh`, MPLAB XC8 v4.00, Docker, GitHub Actions.

**Design spec:** `docs/superpowers/specs/2026-08-05-distribution-design.md`. Read it before starting. This plan implements its phases 1 and 2 only.

## Global Constraints

- **No em-dashes.** Not in docs, not in commit messages, not in code comments. Use a comma, a colon, or a period and a new sentence. A pre-commit hook enforces this and will reject the commit.
- **Python 3.11 minimum.** `tomllib` is stdlib from 3.11. Do not add any third-party Python dependency; tests use `unittest`, not `pytest`.
- **The toolchain container has no python3.** `docker/ci-toolchain/Dockerfile` is `debian:12-slim` plus `ca-certificates curl unzip make tar`, GTK runtime libs, and `cmake build-essential`. Nothing this plan adds may require python3 inside that container. Do not modify the Dockerfile.
- **Conventional Commits.** `type(scope): summary`, types `feat`/`docs`/`plan`/`fix`/`refactor`/`style`. Scope is the module or `manifest`. Commit whenever a piece of work is finished; do not batch unrelated changes.
- **Files end with a trailing newline and have no trailing whitespace.** The pre-commit hook enforces both.
- **Do not fix any `KNOWN_BROKEN` build failure.** They move into the manifest as `excluded` entries with reasons, and stay failing. Repairing them is explicitly out of scope.
- **Do not change the host CMake/ctest build or `.github/workflows/host-tests.yml`.**
- **XC8 version:** the toolchain image pins v4.00. Every equivalence comparison must use one single compiler build for both sides of the diff, because `.hex` output is compiler-version-dependent.

## Prerequisites

The hex-diff gate needs `xc8-cc`. Two ways to get it, in order of preference:

1. **CI (recommended, no local install).** Task 6 adds a workflow that runs the gate inside the existing toolchain container. Push the branch and read the job summary. This is the supported path and requires nothing locally.
2. **Local Docker.** Needs `docker/ci-toolchain/vendor/` populated with two Microchip installers that only a human can download (`make check-vendor` names them), then `make image`. If `make check-vendor` fails, use path 1.

You do **not** need MPLAB X, a licence, or hardware. Do not block on obtaining XC8 locally; write code and run unit tests, then let CI run the gate.

## File Structure

**Created:**

| Path | Responsibility |
|---|---|
| `epic-common/manifest/modules.toml` | The single source of truth: families, HAL source sets, modules, deps, examples, config words, supported/excluded parts. Data only. |
| `epic-common/manifest/README.md` | Schema reference and how to add a module or part. |
| `scripts/epicmanifest.py` | Importable module: load, validate, and resolve the manifest. No I/O beyond reading the TOML. The only file that knows the schema. |
| `scripts/epic_build.py` | CLI driver: `build`, `matrix`, `report` subcommands. Imports `epicmanifest`. |
| `scripts/bootstrap_manifest.py` | One-shot generator that extracts manifest data from the existing Makefiles. Deleted in Task 11. |
| `scripts/tests/test_epicmanifest.py` | Unit tests for load/validate/resolve. |
| `scripts/tests/test_epic_build.py` | Unit tests for script emission and log parsing. |
| `scripts/equivalence-gate.sh` | POSIX sh, runs old and new build paths and diffs `.hex`. Runs in-container. Deleted in Task 11. |
| `.github/workflows/manifest-equivalence.yml` | Temporary workflow running the gate. Deleted in Task 11. |

**Modified:** `scripts/ci-discover-xc8-matrix.py` (Task 8, then deleted Task 10), `.github/workflows/xc8-build.yml` (Task 9), `.github/workflows/sim-tests.yml` (Task 9), root `Makefile` (Task 10), `docs/mplabx-link-gaps-plan.md` and `docs/ci-plan.md` and `README.md` (Task 11).

**Deleted (Task 10, only after the gate is green):** all 29 `*/mcu/*-mplabx/Makefile`, and `epic-common/mk/epic_family.mk`.

Note `scripts/epicmanifest.py` and `scripts/epic_build.py` use underscores, not hyphens: they must be importable as Python modules. Existing hyphenated scripts (`ci-discover-xc8-matrix.py`) are never imported, which is why they get away with hyphens.

---

## Task 1: Manifest schema and loader

**Files:**
- Create: `scripts/epicmanifest.py`
- Create: `scripts/tests/test_epicmanifest.py`
- Create: `scripts/tests/__init__.py` (empty)

**Interfaces:**
- Consumes: nothing.
- Produces:
  - `load(path: pathlib.Path) -> Manifest`
  - `class Manifest` with `.families: dict[str, Family]`, `.modules: dict[str, Module]`
  - `class Family` with `.name: str`, `.hal_dir: str`, `.variants: list[str]`, `.dfp: str`, `.includes: list[str]`, `.hal_sources: list[str]`, `.conditional_sources: list[ConditionalSource]`
  - `class ConditionalSource` with `.path: str`, `.variants: list[str]`
  - `class Module` with `.name: str`, `.dir: str`, `.sources: list[str]`, `.includes: list[str]`, `.depends_on: list[str]`, `.supported: dict[str, list[str]]`, `.excluded: dict[str, str]`, `.example: Example | None`
  - `class Example` with `.name: str`, `.sources: list[str]`, `.config: dict[str, dict[str, str]]`
  - `class ManifestError(Exception)`

The canonical schema, which Task 3 fills in for real:

```toml
[families.PIC16F87XA]
hal_dir  = "pic16f87xa-hal"
variants = ["16F873A", "16F874A", "16F876A", "16F877A"]
dfp      = "Microchip.PIC16Fxxx_DFP"
includes = ["pic16f87xa-hal/include/target", "pic16f87xa-hal/include", "epic-common/include"]
hal_sources = [
  "pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c",
  "epic-common/src/core/epic_harness_target.c",
]

[[families.PIC16F87XA.conditional_sources]]
path     = "pic16f87xa-hal/src/peripherals/pic16f87xa_psp.c"
variants = ["16F874A", "16F877A"]

[modules.epic-serial]
dir        = "epic-serial"
sources    = ["src/epic_serial.c"]
includes   = ["include"]
depends_on = []

[modules.epic-serial.supported]
PIC16F87XA = ["16F876A", "16F877A"]

[modules.epic-serial.excluded]
"16F873A" = "RAM: 32-byte rx buffer does not fit"

[modules.epic-serial.example]
name    = "serial-echo"
sources = ["examples/example_serial_target.c"]

[modules.epic-serial.example.config.PIC16F87XA]
FOSC = "HS"
WDTE = "ON"
```

Paths in `hal_sources`, `includes` at family level, and `conditional_sources.path` are **repo-root-relative**. Paths in a module's `sources`, `includes`, and `example.sources` are **relative to that module's `dir`**. This split is deliberate: family data spans directories, module data does not, and it keeps module entries short and movable.

- [ ] **Step 1: Write the failing tests**

Create `scripts/tests/__init__.py` as an empty file, then `scripts/tests/test_epicmanifest.py`:

```python
"""Unit tests for scripts/epicmanifest.py."""
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

import epicmanifest  # noqa: E402

MINIMAL = """
[families.PIC16F87XA]
hal_dir  = "pic16f87xa-hal"
variants = ["16F873A", "16F877A"]
dfp      = "Microchip.PIC16Fxxx_DFP"
includes = ["pic16f87xa-hal/include", "epic-common/include"]
hal_sources = ["pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c"]

[[families.PIC16F87XA.conditional_sources]]
path     = "pic16f87xa-hal/src/peripherals/pic16f87xa_psp.c"
variants = ["16F877A"]

[modules.epic-tick]
dir        = "epic-tick"
sources    = ["src/epic_tick.c"]
includes   = ["include"]
depends_on = []

[modules.epic-tick.supported]
PIC16F87XA = ["16F873A", "16F877A"]

[modules.epic-tick.example]
name    = "tick-blink"
sources = ["examples/example_tick.c"]

[modules.epic-tick.example.config.PIC16F87XA]
FOSC = "HS"

[modules.epic-serial]
dir        = "epic-serial"
sources    = ["src/epic_serial.c"]
includes   = ["include"]
depends_on = ["epic-tick"]

[modules.epic-serial.supported]
PIC16F87XA = ["16F877A"]

[modules.epic-serial.excluded]
"16F873A" = "RAM: 32-byte rx buffer does not fit"
"""


def write(text):
    tmp = tempfile.NamedTemporaryFile("w", suffix=".toml", delete=False)
    tmp.write(text)
    tmp.close()
    return pathlib.Path(tmp.name)


class TestLoad(unittest.TestCase):
    def setUp(self):
        self.m = epicmanifest.load(write(MINIMAL))

    def test_families_parsed(self):
        fam = self.m.families["PIC16F87XA"]
        self.assertEqual(fam.hal_dir, "pic16f87xa-hal")
        self.assertEqual(fam.variants, ["16F873A", "16F877A"])
        self.assertEqual(fam.dfp, "Microchip.PIC16Fxxx_DFP")

    def test_family_name_is_populated_from_the_table_key(self):
        self.assertEqual(self.m.families["PIC16F87XA"].name, "PIC16F87XA")

    def test_conditional_sources_parsed(self):
        cond = self.m.families["PIC16F87XA"].conditional_sources
        self.assertEqual(len(cond), 1)
        self.assertEqual(cond[0].variants, ["16F877A"])

    def test_modules_parsed(self):
        mod = self.m.modules["epic-serial"]
        self.assertEqual(mod.name, "epic-serial")
        self.assertEqual(mod.dir, "epic-serial")
        self.assertEqual(mod.depends_on, ["epic-tick"])
        self.assertEqual(mod.supported["PIC16F87XA"], ["16F877A"])
        self.assertEqual(mod.excluded["16F873A"], "RAM: 32-byte rx buffer does not fit")

    def test_example_parsed(self):
        ex = self.m.modules["epic-tick"].example
        self.assertEqual(ex.name, "tick-blink")
        self.assertEqual(ex.config["PIC16F87XA"]["FOSC"], "HS")

    def test_example_is_none_when_absent(self):
        self.assertIsNone(self.m.modules["epic-serial"].example)

    def test_excluded_defaults_to_empty(self):
        self.assertEqual(self.m.modules["epic-tick"].excluded, {})


class TestValidation(unittest.TestCase):
    def test_unknown_dependency_is_rejected(self):
        bad = MINIMAL.replace('depends_on = ["epic-tick"]', 'depends_on = ["epic-nope"]')
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("epic-nope", str(cm.exception))

    def test_unknown_family_in_supported_is_rejected(self):
        bad = MINIMAL.replace("[modules.epic-tick.supported]\nPIC16F87XA",
                              "[modules.epic-tick.supported]\nPIC99XXXX")
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("PIC99XXXX", str(cm.exception))

    def test_supported_variant_not_in_family_is_rejected(self):
        bad = MINIMAL.replace('PIC16F87XA = ["16F873A", "16F877A"]',
                              'PIC16F87XA = ["16F873A", "16F999X"]')
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("16F999X", str(cm.exception))

    def test_variant_both_supported_and_excluded_is_rejected(self):
        bad = MINIMAL.replace('"16F873A" = "RAM: 32-byte rx buffer does not fit"',
                              '"16F877A" = "contradiction"')
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("16F877A", str(cm.exception))

    def test_dependency_cycle_is_rejected(self):
        bad = MINIMAL.replace('[modules.epic-tick]\ndir        = "epic-tick"\nsources    = ["src/epic_tick.c"]\nincludes   = ["include"]\ndepends_on = []',
                              '[modules.epic-tick]\ndir        = "epic-tick"\nsources    = ["src/epic_tick.c"]\nincludes   = ["include"]\ndepends_on = ["epic-serial"]')
        with self.assertRaises(epicmanifest.ManifestError) as cm:
            epicmanifest.load(write(bad))
        self.assertIn("cycle", str(cm.exception).lower())


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `python3 -m unittest discover -s scripts/tests -v`
Expected: FAIL, `ModuleNotFoundError: No module named 'epicmanifest'`.

- [ ] **Step 3: Implement the loader**

Create `scripts/epicmanifest.py`:

```python
"""Load, validate, and resolve epic-common/manifest/modules.toml.

This is the only module that knows the manifest's schema. Everything
else (the build driver, the CI matrix, the bundle generator) goes
through the dataclasses here, so a schema change has exactly one place
to land.

Paths are relative to two different roots by design, see the manifest's
own README.md: family-level paths are repo-root-relative because family
data spans directories; module-level paths are relative to that module's
`dir` because module data does not.
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
    includes: list[str]
    hal_sources: list[str]
    conditional_sources: list[ConditionalSource]


@dataclasses.dataclass(frozen=True)
class Example:
    name: str
    sources: list[str]
    config: dict[str, dict[str, str]]


@dataclasses.dataclass(frozen=True)
class Module:
    name: str
    dir: str
    sources: list[str]
    includes: list[str]
    depends_on: list[str]
    supported: dict[str, list[str]]
    excluded: dict[str, str]
    example: Example | None


@dataclasses.dataclass(frozen=True)
class Manifest:
    families: dict[str, Family]
    modules: dict[str, Module]


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
        includes=list(_require(table, "includes", f"families.{name}")),
        hal_sources=list(_require(table, "hal_sources", f"families.{name}")),
        conditional_sources=[
            ConditionalSource(path=c["path"], variants=list(c["variants"]))
            for c in table.get("conditional_sources", [])
        ],
    )


def _parse_example(module_name, table):
    if table is None:
        return None
    return Example(
        name=_require(table, "name", f"modules.{module_name}.example"),
        sources=list(_require(table, "sources", f"modules.{module_name}.example")),
        config={
            fam: dict(pragmas)
            for fam, pragmas in table.get("config", {}).items()
        },
    )


def _parse_module(name, table):
    return Module(
        name=name,
        dir=_require(table, "dir", f"modules.{name}"),
        sources=list(_require(table, "sources", f"modules.{name}")),
        includes=list(table.get("includes", [])),
        depends_on=list(table.get("depends_on", [])),
        supported={
            fam: list(variants)
            for fam, variants in table.get("supported", {}).items()
        },
        excluded=dict(table.get("excluded", {})),
        example=_parse_example(name, table.get("example")),
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
        if mod.example is not None:
            for fam_name in mod.example.config:
                if fam_name not in manifest.families:
                    raise ManifestError(
                        f"modules.{mod.name}.example.config: "
                        f"unknown family '{fam_name}'"
                    )
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
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `python3 -m unittest discover -s scripts/tests -v`
Expected: PASS, 12 tests (7 in `TestLoad`, 5 in `TestValidation`).

- [ ] **Step 5: Commit**

```bash
git add scripts/epicmanifest.py scripts/tests/
git commit -m "feat(manifest): add manifest schema loader and validator"
```

---

## Task 2: Dependency and source resolution

**Files:**
- Modify: `scripts/epicmanifest.py`
- Modify: `scripts/tests/test_epicmanifest.py`

**Interfaces:**
- Consumes: everything from Task 1.
- Produces:
  - `Manifest.resolve_deps(module_name: str) -> list[str]` returning the module plus all transitive dependencies, dependencies first, deterministic order.
  - `Manifest.is_supported(module_name: str, family: str, mcu: str) -> bool`
  - `Manifest.exclusion_reason(module_name: str, mcu: str) -> str | None`
  - `Manifest.family_of(mcu: str) -> Family` raising `ManifestError` on an unknown part.
  - `Manifest.sources_for(module_name: str, mcu: str) -> list[str]` returning repo-root-relative paths: family HAL sources, applicable conditional sources, then each resolved module's sources, then the requested module's example sources. Deterministic, no duplicates.
  - `Manifest.includes_for(module_name: str, mcu: str) -> list[str]` returning repo-root-relative include directories, family includes first (order preserved, because `include/target` must precede `include`).

- [ ] **Step 1: Write the failing tests**

Append to `scripts/tests/test_epicmanifest.py`, before the `if __name__` block:

```python
class TestResolution(unittest.TestCase):
    def setUp(self):
        self.m = epicmanifest.load(write(MINIMAL))

    def test_resolve_deps_puts_dependencies_first(self):
        self.assertEqual(
            self.m.resolve_deps("epic-serial"), ["epic-tick", "epic-serial"]
        )

    def test_resolve_deps_of_a_leaf_is_just_itself(self):
        self.assertEqual(self.m.resolve_deps("epic-tick"), ["epic-tick"])

    def test_resolve_deps_rejects_unknown_module(self):
        with self.assertRaises(epicmanifest.ManifestError):
            self.m.resolve_deps("epic-nope")

    def test_family_of_maps_a_part_to_its_family(self):
        self.assertEqual(self.m.family_of("16F877A").name, "PIC16F87XA")

    def test_family_of_rejects_unknown_part(self):
        with self.assertRaises(epicmanifest.ManifestError):
            self.m.family_of("16F999X")

    def test_is_supported(self):
        self.assertTrue(self.m.is_supported("epic-serial", "PIC16F87XA", "16F877A"))
        self.assertFalse(self.m.is_supported("epic-serial", "PIC16F87XA", "16F873A"))

    def test_exclusion_reason(self):
        self.assertEqual(
            self.m.exclusion_reason("epic-serial", "16F873A"),
            "RAM: 32-byte rx buffer does not fit",
        )
        self.assertIsNone(self.m.exclusion_reason("epic-serial", "16F877A"))

    def test_sources_include_hal_module_and_example(self):
        srcs = self.m.sources_for("epic-tick", "16F877A")
        self.assertIn("pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c", srcs)
        self.assertIn("epic-tick/src/epic_tick.c", srcs)
        self.assertIn("epic-tick/examples/example_tick.c", srcs)

    def test_conditional_source_included_only_on_matching_variant(self):
        psp = "pic16f87xa-hal/src/peripherals/pic16f87xa_psp.c"
        self.assertIn(psp, self.m.sources_for("epic-tick", "16F877A"))
        self.assertNotIn(psp, self.m.sources_for("epic-tick", "16F873A"))

    def test_sources_pull_in_dependency_sources(self):
        srcs = self.m.sources_for("epic-serial", "16F877A")
        self.assertIn("epic-tick/src/epic_tick.c", srcs)
        self.assertIn("epic-serial/src/epic_serial.c", srcs)

    def test_sources_only_include_the_requested_modules_example(self):
        srcs = self.m.sources_for("epic-serial", "16F877A")
        self.assertNotIn("epic-tick/examples/example_tick.c", srcs)

    def test_sources_have_no_duplicates(self):
        srcs = self.m.sources_for("epic-serial", "16F877A")
        self.assertEqual(len(srcs), len(set(srcs)))

    def test_includes_preserve_family_order_then_modules(self):
        incs = self.m.includes_for("epic-serial", "16F877A")
        self.assertEqual(incs[0], "pic16f87xa-hal/include")
        self.assertEqual(incs[1], "epic-common/include")
        self.assertIn("epic-tick/include", incs)
        self.assertIn("epic-serial/include", incs)
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `python3 -m unittest discover -s scripts/tests -v`
Expected: FAIL, `AttributeError: 'Manifest' object has no attribute 'resolve_deps'`.

- [ ] **Step 3: Implement resolution**

In `scripts/epicmanifest.py`, replace the `Manifest` dataclass with this version (the fields are unchanged; methods are added):

```python
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

    def sources_for(self, module_name: str, mcu: str) -> list[str]:
        """Repo-root-relative sources for one (module, MCU) build.

        Order: family HAL sources, applicable conditional sources, each
        resolved module's own sources (dependencies first), then the
        requested module's example. Only the requested module's example
        is included; a dependency's example is a separate program.
        """
        fam = self.family_of(mcu)
        out = list(fam.hal_sources)
        out += [c.path for c in fam.conditional_sources if mcu in c.variants]

        for name in self.resolve_deps(module_name):
            mod = self._module(name)
            out += [f"{mod.dir}/{s}" for s in mod.sources]

        mod = self._module(module_name)
        if mod.example is not None:
            out += [f"{mod.dir}/{s}" for s in mod.example.sources]

        return _dedupe(out)

    def includes_for(self, module_name: str, mcu: str) -> list[str]:
        """Repo-root-relative include dirs, family first.

        Family order is preserved verbatim: include/target must precede
        include so the platform header resolves to the real-target
        (volatile-dereference) version, not the host memory-backed one.
        """
        fam = self.family_of(mcu)
        out = list(fam.includes)
        for name in self.resolve_deps(module_name):
            mod = self._module(name)
            out += [f"{mod.dir}/{i}" for i in mod.includes]
        return _dedupe(out)
```

And add this helper at module level, just above `_require`:

```python
def _dedupe(items):
    """Order-preserving de-duplication."""
    seen, out = set(), []
    for item in items:
        if item not in seen:
            seen.add(item)
            out.append(item)
    return out
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `python3 -m unittest discover -s scripts/tests -v`
Expected: PASS, 25 tests (12 from Task 1 plus 13 in `TestResolution`).

- [ ] **Step 5: Commit**

```bash
git add scripts/epicmanifest.py scripts/tests/test_epicmanifest.py
git commit -m "feat(manifest): resolve module dependencies, sources, and includes"
```

---

## Task 3: Bootstrap the manifest from the existing Makefiles

**Files:**
- Create: `scripts/bootstrap_manifest.py`
- Create: `epic-common/manifest/modules.toml` (generated output, committed)

**Interfaces:**
- Consumes: nothing from earlier tasks (it writes TOML that Task 1's loader must accept).
- Produces: `epic-common/manifest/modules.toml` covering all three families and all 26 module `mcu/` directories.

The manifest is **extracted mechanically, never hand-transcribed.** Hand-copying 29 source lists is exactly the error the hex-diff gate exists to catch, and avoiding it up front is cheaper than debugging it later. Two extraction mechanisms, both exact:

1. **Variables** come from GNU make itself, by feeding it a second makefile that prints any variable. No regex over Makefile text:

   ```sh
   make -C <dir> -f Makefile -f - MCU=<mcu> print-SRCS <<'EOF'
   print-%: ; @echo $($*)
   EOF
   ```

   This resolves `SRCS`, `INC`, `EPIC_SOURCES`, and `TARGET` exactly as the real build would, including the `$(filter ...)` conditionals that add PSP only on 40/44-pin parts.

2. **Config words** come from running the real recipe. Generating `build/config_<MCU>.c` only needs `mkdir` and `printf`, never a compiler, so `make -C <dir> MCU=<mcu> build/config_<MCU>.c` genuinely runs anywhere and the generated file is then parsed for `#pragma config KEY = VALUE`.

- [ ] **Step 1: Write the bootstrap script**

Create `scripts/bootstrap_manifest.py`:

```python
#!/usr/bin/env python3
"""One-shot generator: build modules.toml from the existing Makefiles.

Deleted once the migration is done (see this plan's Task 11). It exists
so the manifest is extracted mechanically rather than hand-transcribed:
29 hand-copied source lists is precisely the mistake the equivalence
gate would then have to catch one at a time.

Two extraction mechanisms, both exact:
  - variables: GNU make prints them itself, via a second makefile
    supplying a `print-%` rule. No regex over Makefile source text, so
    $(filter ...) conditionals (PSP on 40/44-pin parts only) resolve
    correctly per MCU.
  - config words: `make build/config_<MCU>.c` needs only mkdir and
    printf, never a compiler, so the real recipe runs anywhere and its
    output is parsed.

Usage:  python3 scripts/bootstrap_manifest.py > epic-common/manifest/modules.toml
"""
from __future__ import annotations

import os
import pathlib
import re
import subprocess
import sys

REPO = pathlib.Path(__file__).resolve().parents[1]

FAMILIES = {
    "pic16f87xa": {
        "name": "PIC16F87XA",
        "hal_dir": "pic16f87xa-hal",
        "variants": ["16F873A", "16F874A", "16F876A", "16F877A"],
        "dfp": "Microchip.PIC16Fxxx_DFP",
    },
    "pic18fxx5x": {
        "name": "PIC18Fxx5x",
        "hal_dir": "pic18fxx5x-hal",
        "variants": ["18F2455", "18F2550", "18F4455", "18F4550"],
        "dfp": "Microchip.PIC18Fxxxx_DFP",
    },
    "pic16f193x": {
        "name": "PIC16F193X",
        "hal_dir": "pic16f193x-hal",
        "variants": ["16F1933", "16F1934", "16F1936", "16F1937", "16F1938", "16F1939"],
        "dfp": "Microchip.PIC12-16F1xxx_DFP",
    },
}

PRINT_RULE = "print-%: ; @echo $($*)\n"


def make_var(directory: pathlib.Path, mcu: str, var: str) -> str:
    """Ask GNU make for one resolved variable's value."""
    proc = subprocess.run(
        ["make", "-C", str(directory), "-f", "Makefile", "-f", "-",
         f"MCU={mcu}", f"print-{var}"],
        input=PRINT_RULE, capture_output=True, text=True,
    )
    if proc.returncode != 0:
        sys.exit(f"make failed in {directory} for {var} (MCU={mcu}):\n{proc.stderr}")
    return proc.stdout.strip()


def normalise(directory: pathlib.Path, path: str) -> str:
    """Turn a Makefile-relative path into a repo-root-relative one."""
    resolved = (directory / path).resolve()
    return str(resolved.relative_to(REPO))


def config_pragmas(directory: pathlib.Path, mcu: str) -> dict[str, str]:
    """Run the real config-word recipe and parse what it emitted."""
    target = f"build/config_{mcu}.c"
    proc = subprocess.run(
        ["make", "-C", str(directory), f"MCU={mcu}", target],
        capture_output=True, text=True,
    )
    generated = directory / target
    if proc.returncode != 0 or not generated.exists():
        sys.exit(f"could not generate {target} in {directory}:\n{proc.stderr}")
    pragmas = {}
    for line in generated.read_text().splitlines():
        m = re.match(r"#pragma\s+config\s+(\w+)\s*=\s*(\S+)", line)
        if m:
            pragmas[m.group(1)] = m.group(2)
    return pragmas


def family_of_dir(directory: pathlib.Path) -> str:
    for key in FAMILIES:
        if key in directory.name:
            return key
    sys.exit(f"unrecognised family for {directory}")


def mcu_dirs() -> list[pathlib.Path]:
    out = subprocess.run(
        ["git", "ls-files", "--", "*/mcu/*-mplabx/Makefile"],
        capture_output=True, text=True, check=True, cwd=REPO,
    ).stdout
    return [REPO / line.rsplit("/Makefile", 1)[0] for line in out.splitlines()]


def toml_str(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def toml_list(values, indent="  ") -> str:
    if not values:
        return "[]"
    body = "".join(f"\n{indent}{toml_str(v)}," for v in values)
    return "[" + body + "\n]"


def main():
    hal_dirs, module_dirs = [], []
    for d in mcu_dirs():
        (hal_dirs if d.parent.parent.name.endswith("-hal") else module_dirs).append(d)

    out = []
    out.append("# GENERATED by scripts/bootstrap_manifest.py from the")
    out.append("# mcu/*-mplabx/Makefiles it replaces. Reviewed by hand")
    out.append("# afterwards; edit this file directly from now on.")
    out.append("")

    # Families: the HAL source set is what that family's own HAL Makefile
    # compiles. A source only some variants compile becomes a conditional.
    for hal_dir in sorted(hal_dirs):
        key = family_of_dir(hal_dir)
        fam = FAMILIES[key]
        per_variant = {
            mcu: [normalise(hal_dir, p) for p in make_var(hal_dir, mcu, "EPIC_SOURCES").split()]
            for mcu in fam["variants"]
        }
        common = [p for p in per_variant[fam["variants"][0]]
                  if all(p in per_variant[m] for m in fam["variants"])]
        conditional = {}
        for mcu, paths in per_variant.items():
            for p in paths:
                if p not in common:
                    conditional.setdefault(p, []).append(mcu)

        includes = [normalise(hal_dir, tok[2:])
                    for tok in make_var(hal_dir, fam["variants"][0], "INC").split()
                    if tok.startswith("-I")]

        out.append(f'[families.{fam["name"]}]')
        out.append(f'hal_dir  = {toml_str(fam["hal_dir"])}')
        out.append(f'variants = {toml_list(fam["variants"])}')
        out.append(f'dfp      = {toml_str(fam["dfp"])}')
        out.append(f"includes = {toml_list(includes)}")
        out.append(f"hal_sources = {toml_list(common)}")
        out.append("")
        for path, variants in sorted(conditional.items()):
            out.append(f'[[families.{fam["name"]}.conditional_sources]]')
            out.append(f"path     = {toml_str(path)}")
            out.append(f"variants = {toml_list(sorted(variants))}")
            out.append("")

    # Modules: whatever a module's Makefile compiles beyond its family's
    # HAL set is that module's own sources plus its example.
    by_module = {}
    for d in module_dirs:
        by_module.setdefault(d.parent.parent.name, []).append(d)

    for module_name in sorted(by_module):
        module_dir = REPO / module_name
        supported, excluded, configs = {}, {}, {}
        module_sources, example_sources, includes = [], [], []

        for d in sorted(by_module[module_name]):
            key = family_of_dir(d)
            fam = FAMILIES[key]
            hal_dir = REPO / fam["hal_dir"] / "mcu" / f'{key}-mplabx'
            hal_set = set()
            for mcu in fam["variants"]:
                hal_set |= {normalise(hal_dir, p)
                            for p in make_var(hal_dir, mcu, "EPIC_SOURCES").split()}

            first = fam["variants"][0]
            srcs = [normalise(d, p) for p in make_var(d, first, "SRCS").split()]
            own = [s for s in srcs if s not in hal_set and s.startswith(module_name + "/")]
            module_sources += [s.split("/", 1)[1] for s in own if "/examples/" not in s]
            example_sources += [s.split("/", 1)[1] for s in own if "/examples/" in s]

            includes += [normalise(d, tok[2:]).split("/", 1)[1]
                         for tok in make_var(d, first, "INC").split()
                         if tok.startswith("-I") and normalise(d, tok[2:]).startswith(module_name + "/")]

            supported[fam["name"]] = list(fam["variants"])
            configs[fam["name"]] = config_pragmas(d, first)

        out.append(f"[modules.{module_name}]")
        out.append(f"dir        = {toml_str(module_name)}")
        out.append(f"sources    = {toml_list(sorted(set(module_sources)))}")
        out.append(f"includes   = {toml_list(sorted(set(includes)))}")
        out.append("depends_on = []  # REVIEW: fill in by hand, see Task 3 Step 3")
        out.append("")
        out.append(f"[modules.{module_name}.supported]")
        for fam_name, variants in sorted(supported.items()):
            out.append(f"{fam_name} = {toml_list(variants)}")
        out.append("")
        if example_sources:
            out.append(f"[modules.{module_name}.example]")
            out.append(f'name    = {toml_str(module_name.replace("epic-", ""))}')
            out.append(f"sources = {toml_list(sorted(set(example_sources)))}")
            out.append("")
            for fam_name, pragmas in sorted(configs.items()):
                if not pragmas:
                    continue
                out.append(f"[modules.{module_name}.example.config.{fam_name}]")
                for k, v in pragmas.items():
                    out.append(f"{k} = {toml_str(v)}")
                out.append("")

    print("\n".join(out))


if __name__ == "__main__":
    main()
```

- [ ] **Step 2: Generate the manifest**

```bash
mkdir -p epic-common/manifest
python3 scripts/bootstrap_manifest.py > epic-common/manifest/modules.toml
git checkout -- . 2>/dev/null || true   # discard build/ dirs the recipe created
```

Expected: a `modules.toml` of roughly 400 to 600 lines with three `[families.*]` tables and 13 `[modules.*]` tables.

If the script exits with `make failed`, read the error: some Makefile has a variable this script assumes. Fix the script, not the manifest by hand.

- [ ] **Step 3: Fill in the three hand-review items**

Three things the extraction cannot infer. Edit `epic-common/manifest/modules.toml` directly:

1. **`depends_on`**, currently `[]` everywhere with a `# REVIEW` comment. Set from each module's README/docs and remove the comment. Known from the repo's own documentation:
   - `epic-modbus` depends on `["epic-serial", "epic-tick"]`
   - `epic-console` depends on `["epic-serial"]`
   - every other module: `[]`

   Verify each by checking the module's `README.md` "built on" wording before setting it. A wrong `depends_on` shows up as a hex diff in Task 6, so this is checked, not trusted.

2. **`supported` and `excluded`**, currently every variant listed as supported. Transcribe `KNOWN_BROKEN` from `scripts/ci-discover-xc8-matrix.py`: for each `(dir, mcu)` pair there, remove `mcu` from that module's `supported` list for the matching family and add an `excluded` entry whose value is a short reason drawn from that file's own comments. For example, in `[modules.epic-serial.excluded]`:

   ```toml
   "16F873A" = "RAM: 32-byte g_rx_buf does not fit (XC8 error 1250)"
   "16F874A" = "RAM: 32-byte g_rx_buf does not fit (XC8 error 1250)"
   ```

   For `epic-console` and `epic-settings`, every variant of both families is excluded; the reason is `"link: irq dispatch needs every peripheral handler (docs/mplabx-link-gaps-plan.md root cause 1)"`. For `epic-modbus` on PIC16F87XA, all four variants are excluded, so its `supported` table has no `PIC16F87XA` key at all.

3. **Example names**, currently derived by stripping `epic-`. Check each against the `TARGET` its Makefile defined (`make -C <dir> -f Makefile -f - MCU=<mcu> print-TARGET <<< 'print-%: ; @echo $($*)'`) and use the name from that target, so the emitted `.hex` filename matches the old one and the gate can compare like with like. For example `epic-taskmgr`'s target is `$(BUILD_DIR)/$(MCU)-multi-blink`, so its example name is `multi-blink`, not `taskmgr`.

- [ ] **Step 4: Verify the manifest loads and validates**

```bash
python3 -c "
import sys; sys.path.insert(0, 'scripts')
import epicmanifest
m = epicmanifest.load(epicmanifest.default_path())
print(f'{len(m.families)} families, {len(m.modules)} modules')
for name in sorted(m.modules):
    mod = m.modules[name]
    pairs = sum(len(v) for v in mod.supported.values())
    print(f'  {name}: {pairs} supported pairs, {len(mod.excluded)} excluded')
"
```

Expected: `3 families, 13 modules`, then a per-module line. Any `ManifestError` names the exact offending key; fix it and rerun.

- [ ] **Step 5: Commit**

```bash
git add epic-common/manifest/modules.toml scripts/bootstrap_manifest.py
git commit -m "feat(manifest): generate modules.toml from the existing Makefiles"
```

---

## Task 4: Manifest README

**Files:**
- Create: `epic-common/manifest/README.md`

**Interfaces:**
- Consumes: the schema from Tasks 1 to 3.
- Produces: documentation only.

Repo convention is that every directory carrying a contract documents it. The manifest is now load-bearing for CI, the build, and (in later plans) bundles, so it gets a reference.

- [ ] **Step 1: Write the README**

Create `epic-common/manifest/README.md`:

````markdown
# `modules.toml`, the build manifest

One declarative description of what every module needs and where it
builds. Three consumers read it and nothing else encodes the knowledge:

- `scripts/epic_build.py`, the real-target build driver
- `scripts/epic_build.py matrix`, which feeds `xc8-build.yml`
- `scripts/make-bundle.sh`, which generates release bundles

## Path conventions

Two roots, deliberately:

- **Family-level** paths (`hal_sources`, `includes`,
  `conditional_sources.path`) are **repo-root-relative**, because family
  data spans directories.
- **Module-level** paths (`sources`, `includes`, `example.sources`) are
  relative to that module's own `dir`, so a module entry stays short and
  the module can be moved without rewriting it.

## Families

```toml
[families.PIC16F87XA]
hal_dir  = "pic16f87xa-hal"
variants = ["16F873A", "16F874A", "16F876A", "16F877A"]
dfp      = "Microchip.PIC16Fxxx_DFP"
includes = ["pic16f87xa-hal/include/target", "pic16f87xa-hal/include",
            "epic-common/include"]
hal_sources = ["pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c"]
```

`includes` order is significant and preserved verbatim:
`include/target` must precede `include` so the platform header resolves
to the real-target (volatile-dereference) version rather than the host
memory-backed one.

`hal_sources` is the full peripheral set for the family. It is not
trimmed per module: `pic16_irq_dispatch.c` takes strong references to
every peripheral's `_IRQHandler` specifically to force the linker to
resolve them all, so a partial set is a guaranteed link failure. See
`docs/mplabx-link-gaps-plan.md` root cause 1.

A source only some variants compile is a conditional:

```toml
[[families.PIC16F87XA.conditional_sources]]
path     = "pic16f87xa-hal/src/peripherals/pic16f87xa_psp.c"
variants = ["16F874A", "16F877A"]   # PSP is 40/44-pin only
```

## Modules

```toml
[modules.epic-modbus]
dir        = "epic-modbus"
sources    = ["src/epic_modbus.c"]
includes   = ["include"]
depends_on = ["epic-serial", "epic-tick"]
```

`depends_on` is resolved transitively, so a consumer naming `epic-modbus`
gets `epic-serial` and `epic-tick` automatically. Cycles are rejected at
load time.

## Supported and excluded parts

```toml
[modules.epic-serial.supported]
PIC16F87XA = ["16F876A", "16F877A"]
PIC18Fxx5x = ["18F4455", "18F4550"]

[modules.epic-serial.excluded]
"16F873A" = "RAM: 32-byte g_rx_buf does not fit (XC8 error 1250)"
```

Every part of a family must appear in exactly one of the two, and the
loader rejects a part listed in both. `excluded` reasons are user-facing:
they are printed by the build driver and, in a later plan, by the
generated `epicurus.mk` and `SUPPORT.md`.

This pair replaces the `KNOWN_BROKEN` literal that used to live in
`scripts/ci-discover-xc8-matrix.py`. `docs/mplabx-link-gaps-plan.md`'s
exit criterion is now "no `excluded` entries remain."

## Examples

```toml
[modules.epic-taskmgr.example]
name    = "multi-blink"
sources = ["examples/example_multi_blink.c"]

[modules.epic-taskmgr.example.config.PIC16F87XA]
FOSC  = "HS"
WDTE  = "ON"
```

`name` becomes the `.hex` basename: `build/16F877A-multi-blink.hex`.
The `config` table generates the `#pragma config` translation unit, which
is why the pragmas are per family: PIC16 has one configuration word,
PIC18 has several with unrelated fields.

## Adding a module

1. Add a `[modules.<name>]` table with `dir`, `sources`, `includes`, and
   `depends_on`.
2. List the parts it builds on under `supported`, and every other part of
   those families under `excluded` with a reason.
3. Add an `example` table if it ships a target program.
4. `python3 scripts/epic_build.py build --module <name> --mcu <part> --run`
5. Nothing else. CI's matrix picks it up automatically.

## Adding a part to an existing family

Add it to that family's `variants`, then add it to every module's
`supported` or `excluded`. The loader fails until every module has
classified it, which is intentional: a new part should not silently
appear supported everywhere.
````

- [ ] **Step 2: Verify no em-dashes and no trailing whitespace**

```bash
grep -nP '\x{2014}' epic-common/manifest/README.md && echo "FAIL: em-dash found" || echo "OK"
grep -nE ' +$' epic-common/manifest/README.md && echo "FAIL: trailing ws" || echo "OK"
```

Expected: `OK` twice.

- [ ] **Step 3: Commit**

```bash
git add epic-common/manifest/README.md
git commit -m "docs(manifest): document the modules.toml schema"
```

---

## Task 5: Build-script emitter

**Files:**
- Create: `scripts/epic_build.py`
- Create: `scripts/tests/test_epic_build.py`

**Interfaces:**
- Consumes: `epicmanifest.load`, `Manifest.sources_for`, `Manifest.includes_for`, `Manifest.family_of`, `Manifest.is_supported`, `Manifest.exclusion_reason` from Tasks 1 and 2.
- Produces:
  - `emit_config_source(manifest, module, mcu) -> str` returning the generated config-word C.
  - `emit_build_script(manifest, module, mcu, build_dir, dfp_dir, fosc_hz) -> str` returning POSIX `sh`.
  - CLI: `epic_build.py build --module M --mcu X [--run] [--build-dir DIR] [--dfp-dir DIR] [--fosc-hz N]`

The emitted script must reproduce what `epic-common/mk/epic_family.mk` plus the per-module Makefile did: compile each source to `build/<MCU>/<basename>.p1` with `xc8-cc -c`, compile the generated config translation unit the same way, then link all `.p1` to `<build-dir>/<MCU>-<example>.hex` with `-ginhx32`.

Flag order matters for byte-identical output. The old `CFLAGS` were, in order: `$(DFP_FLAG)`, `-mcpu=<lowercase mcu>`, `-O2 -std=c99 -Wall -Wextra`, `-D<PARTDEFINE>`, the `-I` list, `-DFOSC_HZ=<n>`. Reproduce exactly that order.

- [ ] **Step 1: Write the failing tests**

Create `scripts/tests/test_epic_build.py`:

```python
"""Unit tests for scripts/epic_build.py."""
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

import epic_build  # noqa: E402
import epicmanifest  # noqa: E402

MANIFEST = """
[families.PIC16F87XA]
hal_dir  = "pic16f87xa-hal"
variants = ["16F873A", "16F877A"]
dfp      = "Microchip.PIC16Fxxx_DFP"
includes = ["pic16f87xa-hal/include/target", "pic16f87xa-hal/include"]
hal_sources = ["pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c"]

[[families.PIC16F87XA.conditional_sources]]
path     = "pic16f87xa-hal/src/peripherals/pic16f87xa_psp.c"
variants = ["16F877A"]

[modules.epic-tick]
dir        = "epic-tick"
sources    = ["src/epic_tick.c"]
includes   = ["include"]
depends_on = []

[modules.epic-tick.supported]
PIC16F87XA = ["16F877A"]

[modules.epic-tick.excluded]
"16F873A" = "RAM: does not fit"

[modules.epic-tick.example]
name    = "tick-blink"
sources = ["examples/example_tick.c"]

[modules.epic-tick.example.config.PIC16F87XA]
FOSC = "HS"
WDTE = "ON"
"""


def load():
    tmp = tempfile.NamedTemporaryFile("w", suffix=".toml", delete=False)
    tmp.write(MANIFEST)
    tmp.close()
    return epicmanifest.load(pathlib.Path(tmp.name))


class TestConfigSource(unittest.TestCase):
    def test_emits_one_pragma_per_entry(self):
        out = epic_build.emit_config_source(load(), "epic-tick", "16F877A")
        self.assertIn("#include <xc.h>", out)
        self.assertIn("#pragma config FOSC = HS", out)
        self.assertIn("#pragma config WDTE = ON", out)

    def test_marks_the_file_generated(self):
        out = epic_build.emit_config_source(load(), "epic-tick", "16F877A")
        self.assertIn("Auto-generated", out)


class TestBuildScript(unittest.TestCase):
    def script(self, mcu="16F877A", dfp_dir="/opt/dfp"):
        return epic_build.emit_build_script(
            load(), "epic-tick", mcu,
            build_dir="build", dfp_dir=dfp_dir, fosc_hz=20000000,
        )

    def test_starts_with_a_posix_shebang_and_errexit(self):
        lines = self.script().splitlines()
        self.assertEqual(lines[0], "#!/bin/sh")
        self.assertIn("set -e", lines)

    def test_compiles_every_source_to_p1(self):
        s = self.script()
        self.assertIn("pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c", s)
        self.assertIn("epic-tick/src/epic_tick.c", s)
        self.assertIn("epic-tick/examples/example_tick.c", s)
        self.assertIn("build/16F877A/epic_tick.p1", s)

    def test_includes_conditional_source_only_on_matching_variant(self):
        self.assertIn("pic16f87xa_psp.c", self.script())

    def test_flag_order_matches_the_makefiles(self):
        s = self.script()
        self.assertIn(
            "-mdfp=/opt/dfp -mcpu=16f877a -O2 -std=c99 -Wall -Wextra -DPIC16F877A",
            s,
        )

    def test_omits_dfp_flag_when_dfp_dir_is_empty(self):
        s = self.script(dfp_dir="")
        self.assertNotIn("-mdfp=", s)
        self.assertIn("-mcpu=16f877a", s)

    def test_include_flags_preserve_manifest_order(self):
        s = self.script()
        self.assertIn(
            "-Ipic16f87xa-hal/include/target -Ipic16f87xa-hal/include -Iepic-tick/include",
            s,
        )

    def test_defines_fosc_hz_last(self):
        self.assertIn("-DFOSC_HZ=20000000", self.script())

    def test_links_to_the_example_named_hex_with_ginhx32(self):
        s = self.script()
        self.assertIn("build/16F877A-tick-blink.hex", s)
        self.assertIn("-ginhx32", s)

    def test_unsupported_pair_raises_with_the_reason(self):
        with self.assertRaises(epic_build.UnsupportedError) as cm:
            self.script(mcu="16F873A")
        self.assertIn("RAM: does not fit", str(cm.exception))


class TestReport(unittest.TestCase):
    LOG = """
Memory Summary:
    Program space        used   102Ch (  4140) of  2000h words   ( 50.5%)
    Data space           used    5Bh (    91) of   170h bytes   ( 24.7%)
"""

    def test_parses_flash_and_ram(self):
        usage = epic_build.parse_memory_summary(self.LOG)
        self.assertEqual(usage["flash_bytes"], 4140)
        self.assertEqual(usage["ram_bytes"], 91)

    def test_returns_none_when_absent(self):
        self.assertIsNone(epic_build.parse_memory_summary("no summary here"))


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `python3 -m unittest discover -s scripts/tests -v`
Expected: FAIL, `ModuleNotFoundError: No module named 'epic_build'`.

- [ ] **Step 3: Implement the emitter**

Create `scripts/epic_build.py`:

```python
#!/usr/bin/env python3
"""Real-target build driver: manifest in, xc8-cc build script out.

Replaces the 29 hand-maintained mcu/*-mplabx/Makefiles. Resolution
(reading the manifest, resolving dependencies, computing the source and
include lists) happens here in Python; execution is a plain POSIX sh
script of xc8-cc invocations.

That split is not incidental. The toolchain container
(docker/ci-toolchain/Dockerfile) has no python3 in it, deliberately, so
a driver that called xc8-cc directly could not run where xc8-cc lives.
Emitting a script means resolution runs wherever python3 exists (a dev
host, or the GitHub runner, which already runs Python for CI discovery)
and execution needs only sh. The script is also a debugging artifact: it
records the exact command line for every translation unit, which suits a
codebase whose convention is to inspect generated output.

Usage:
  epic_build.py build --module epic-tick --mcu 16F877A --run
  epic_build.py matrix
  epic_build.py report --log build/16F877A/build.log
"""
from __future__ import annotations

import argparse
import json
import pathlib
import re
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import epicmanifest  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parents[1]


class UnsupportedError(Exception):
    """A (module, MCU) pair the manifest says does not build."""


def _check_supported(manifest, module, mcu):
    fam = manifest.family_of(mcu)
    if manifest.is_supported(module, fam.name, mcu):
        return fam
    reason = manifest.exclusion_reason(module, mcu)
    supported = manifest.modules[module].supported.get(fam.name, [])
    detail = ", ".join(supported) if supported else "none"
    raise UnsupportedError(
        f"{module} is not supported on {mcu}"
        + (f" ({reason})" if reason else "")
        + f". Supported on {fam.name}: {detail}."
    )


def emit_config_source(manifest, module, mcu) -> str:
    """The #pragma config translation unit, from manifest data."""
    fam = manifest.family_of(mcu)
    example = manifest.modules[module].example
    pragmas = {} if example is None else example.config.get(fam.name, {})
    lines = [
        "/* Auto-generated by scripts/epic_build.py. Do not edit. */",
        "#include <xc.h>",
    ]
    lines += [f"#pragma config {key} = {value}" for key, value in pragmas.items()]
    return "\n".join(lines) + "\n"


def emit_build_script(manifest, module, mcu, build_dir, dfp_dir, fosc_hz) -> str:
    """A self-contained POSIX sh script that produces the .hex.

    Flag order reproduces the Makefiles this replaces exactly (DFP, then
    -mcpu, then optimisation and warnings, then the part define, then
    includes, then FOSC_HZ). Byte-identical .hex output is the migration
    gate, and XC8's output is sensitive to command-line order, so this
    ordering is load-bearing, not cosmetic.
    """
    _check_supported(manifest, module, mcu)
    example = manifest.modules[module].example
    if example is None:
        raise UnsupportedError(f"{module} has no example to build")

    sources = manifest.sources_for(module, mcu)
    includes = manifest.includes_for(module, mcu)
    objdir = f"{build_dir}/{mcu}"

    flags = []
    if dfp_dir:
        flags.append(f"-mdfp={dfp_dir}")
    flags.append(f"-mcpu={mcu.lower()}")
    flags += ["-O2", "-std=c99", "-Wall", "-Wextra", f"-D{_part_define(mcu)}"]
    flags += [f"-I{inc}" for inc in includes]
    flags.append(f"-DFOSC_HZ={fosc_hz}")
    cflags = " ".join(flags)

    target = f"{build_dir}/{mcu}-{example.name}.hex"

    out = [
        "#!/bin/sh",
        "# GENERATED by scripts/epic_build.py. Do not edit; regenerate.",
        f"# module={module} mcu={mcu}",
        "set -e",
        "",
        'cd "${EPIC_REPO_ROOT:-.}"',
        f"mkdir -p {objdir}",
        "",
    ]

    objs = []
    for src in sources:
        obj = f"{objdir}/{pathlib.Path(src).stem}.p1"
        objs.append(obj)
        out.append(f"xc8-cc {cflags} -c {src} -o {obj}")

    config_obj = f"{objdir}/config_{mcu}.p1"
    objs.append(config_obj)
    out += [
        "",
        f"xc8-cc {cflags} -c {objdir}/config_{mcu}.c -o {config_obj}",
        "",
        f"xc8-cc {cflags} {' '.join(objs)} -o {target} -ginhx32",
        "",
        f'echo "Built {target}"',
    ]
    return "\n".join(out) + "\n"


def _part_define(mcu: str) -> str:
    """16F877A -> PIC16F877A, matching the Makefiles' CFLAGS_DEVICE."""
    return f"PIC{mcu}"


def parse_memory_summary(log: str):
    """Pull flash and RAM byte counts out of XC8's Memory Summary."""
    flash = re.search(r"Program space\s+used\s+\S+\s+\(\s*(\d+)\)", log)
    ram = re.search(r"Data space\s+used\s+\S+\s+\(\s*(\d+)\)", log)
    if not flash or not ram:
        return None
    return {"flash_bytes": int(flash.group(1)), "ram_bytes": int(ram.group(1))}


def cmd_build(args):
    manifest = epicmanifest.load(epicmanifest.default_path())
    try:
        script = emit_build_script(
            manifest, args.module, args.mcu,
            build_dir=args.build_dir, dfp_dir=args.dfp_dir, fosc_hz=args.fosc_hz,
        )
    except (UnsupportedError, epicmanifest.ManifestError) as exc:
        sys.exit(f"error: {exc}")

    objdir = REPO / args.build_dir / args.mcu
    objdir.mkdir(parents=True, exist_ok=True)
    (objdir / f"config_{args.mcu}.c").write_text(
        emit_config_source(manifest, args.module, args.mcu)
    )
    script_path = objdir / "build.sh"
    script_path.write_text(script)
    script_path.chmod(0o755)
    print(f"wrote {script_path.relative_to(REPO)}")

    if not args.run:
        return

    log_path = objdir / "build.log"
    proc = subprocess.run(
        ["sh", str(script_path)], cwd=REPO,
        capture_output=True, text=True,
        env={"EPIC_REPO_ROOT": str(REPO), "PATH": _path()},
    )
    log_path.write_text(proc.stdout + proc.stderr)
    sys.stdout.write(proc.stdout)
    sys.stderr.write(proc.stderr)
    if proc.returncode != 0:
        sys.exit(proc.returncode)
    usage = parse_memory_summary(log_path.read_text())
    if usage:
        print(f"flash {usage['flash_bytes']} bytes, RAM {usage['ram_bytes']} bytes")


def _path():
    import os
    return os.environ.get("PATH", "")


def cmd_matrix(args):
    manifest = epicmanifest.load(epicmanifest.default_path())
    entries = []
    for fam in manifest.families.values():
        modules = []
        for name in sorted(manifest.modules):
            mcus = manifest.modules[name].supported.get(fam.name, [])
            if mcus and manifest.modules[name].example is not None:
                modules.append(f"{name}={','.join(mcus)}")
        if modules:
            entries.append({
                "family": fam.name,
                "dfp": fam.dfp,
                "modules": ";".join(modules),
            })
    if not entries:
        sys.exit("manifest yields no buildable (module, MCU) pairs")
    print(json.dumps(entries))


def cmd_report(args):
    usage = parse_memory_summary(pathlib.Path(args.log).read_text())
    if usage is None:
        sys.exit(f"no Memory Summary found in {args.log}")
    print(json.dumps(usage))


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)

    b = sub.add_parser("build", help="emit (and optionally run) a build script")
    b.add_argument("--module", required=True)
    b.add_argument("--mcu", required=True)
    b.add_argument("--build-dir", default="build")
    b.add_argument("--dfp-dir", default="")
    b.add_argument("--fosc-hz", type=int, default=20000000)
    b.add_argument("--run", action="store_true", help="execute the script (needs xc8-cc)")
    b.set_defaults(func=cmd_build)

    m = sub.add_parser("matrix", help="print the CI build matrix as JSON")
    m.set_defaults(func=cmd_matrix)

    r = sub.add_parser("report", help="parse flash/RAM usage from a build log")
    r.add_argument("--log", required=True)
    r.set_defaults(func=cmd_report)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
```

Note the `cd "${EPIC_REPO_ROOT:-.}"` line: the emitted script uses repo-root-relative source paths, so it must run from the repo root. `--run` sets `EPIC_REPO_ROOT`; a container runs it with the repo as the working directory, so the `:-.` default applies there.

Note also that `--build-dir` must be unique per module. Two modules built for the same part with the same `--build-dir` would write `build.sh` to the same path and clobber each other. Every caller in this plan passes `--build-dir build/<module>` or `build-manifest/<module>` for that reason.

- [ ] **Step 4: Run the tests to verify they pass**

Run: `python3 -m unittest discover -s scripts/tests -v`
Expected: PASS, 38 tests.

- [ ] **Step 5: Emit a real script and read it**

```bash
python3 scripts/epic_build.py build --module epic-taskmgr --mcu 16F877A
cat build/16F877A/build.sh
```

Expected: one `xc8-cc ... -c ... -o build/16F877A/*.p1` line per source, one for the config unit, then a link line producing `build/16F877A-multi-blink.hex`. Compare the flag string against `epic-taskmgr/mcu/pic16f87xa-taskmgr-mplabx/Makefile`'s `CFLAGS` by eye before moving on; a mismatch here is what Task 6 will otherwise report as a hex diff.

- [ ] **Step 6: Commit**

```bash
git add scripts/epic_build.py scripts/tests/test_epic_build.py
git commit -m "feat(manifest): add the xc8-cc build-script emitter"
```

---

## Task 6: Equivalence gate

**Files:**
- Create: `scripts/equivalence-gate.sh`
- Create: `.github/workflows/manifest-equivalence.yml`

**Interfaces:**
- Consumes: `epic_build.py build` from Task 5, the existing Makefiles.
- Produces: a CI job that fails if any `.hex` differs between the old and new paths.

This is the correctness argument for the whole migration. Identical sources, identical flags, and one compiler produce byte-identical output, so any diff is the manifest disagreeing with the Makefile it replaces.

The gate script is POSIX `sh` with no python3, because it runs inside the toolchain container. The Python resolution step runs before the container, on the runner.

- [ ] **Step 1: Write the gate script**

Create `scripts/equivalence-gate.sh`:

```sh
#!/bin/sh
# Prove the manifest-driven build reproduces the Makefile build exactly.
#
# For every (module dir, MCU) pair given on stdin as "dir=mcu,mcu;...",
# build the .hex both ways and compare byte for byte. Identical sources,
# identical flags, one compiler: any difference means the manifest and
# the Makefile disagree, which is the one thing this migration must not
# get wrong.
#
# POSIX sh with no python3 on purpose: this runs inside the toolchain
# container (docker/ci-toolchain/Dockerfile), which deliberately has no
# Python. The manifest side is resolved into build scripts beforehand,
# outside the container.
#
# Usage (from the repo root):
#   echo "epic-tick/mcu/pic16f87xa-tick-mplabx=16F877A" \
#     | DFP_ROOT=$XC8_INSTALL_DIR/pic/packs sh scripts/equivalence-gate.sh
set -eu

fail=0
pass=0

while IFS= read -r line; do
  [ -n "$line" ] || continue
  dir="${line%%=*}"
  mcus="${line#*=}"
  module="$(echo "$dir" | cut -d/ -f1)"

  case "$dir" in
    *pic16f87xa*) dfp="Microchip.PIC16Fxxx_DFP" ;;
    *pic18fxx5x*) dfp="Microchip.PIC18Fxxxx_DFP" ;;
    *pic16f193x*) dfp="Microchip.PIC12-16F1xxx_DFP" ;;
    *) echo "unrecognised family for $dir" >&2; exit 1 ;;
  esac
  dfp_dir="$DFP_ROOT/$dfp/xc8"

  old_ifs="$IFS"; IFS=','
  for mcu in $mcus; do
    IFS="$old_ifs"
    echo "=== $module $mcu ==="

    # Old path: the Makefile, untouched.
    make -C "$dir" clean >/dev/null 2>&1 || true
    if ! make -C "$dir" MCU="$mcu" DFP_DIR="$dfp_dir" >/dev/null 2>&1; then
      echo "SKIP $module $mcu (Makefile build fails; excluded pair)"
      continue
    fi
    old_hex="$(ls "$dir"/build/"$mcu"-*.hex 2>/dev/null | head -1)"
    if [ -z "$old_hex" ]; then
      echo "FAIL $module $mcu (Makefile produced no .hex)"
      fail=$((fail + 1))
      continue
    fi
    cp "$old_hex" "/tmp/old-$module-$mcu.hex"

    # New path: the pre-emitted manifest build script. Note the
    # per-module build dir: two modules built for the same part with a
    # shared --build-dir would overwrite each other's build.sh.
    new_script="build-manifest/$module/$mcu/build.sh"
    if [ ! -f "$new_script" ]; then
      echo "FAIL $module $mcu (no emitted script at $new_script)"
      fail=$((fail + 1))
      continue
    fi
    rm -rf "build-manifest/$module/$mcu"/*.p1
    if ! EPIC_REPO_ROOT="$PWD" sh "$new_script" >/dev/null 2>&1; then
      echo "FAIL $module $mcu (manifest build failed)"
      fail=$((fail + 1))
      continue
    fi
    new_hex="$(ls build-manifest/"$module"/"$mcu"-*.hex 2>/dev/null | head -1)"
    if [ -z "$new_hex" ]; then
      echo "FAIL $module $mcu (manifest build produced no .hex)"
      fail=$((fail + 1))
      continue
    fi

    if cmp -s "/tmp/old-$module-$mcu.hex" "$new_hex"; then
      echo "PASS $module $mcu (byte-identical)"
      pass=$((pass + 1))
    else
      echo "FAIL $module $mcu (.hex differs)"
      echo "  old: $old_hex"
      echo "  new: $new_hex"
      fail=$((fail + 1))
    fi
  done
  IFS="$old_ifs"
done

echo ""
echo "equivalence: $pass identical, $fail differing"
[ "$fail" -eq 0 ]
```

- [ ] **Step 2: Write the workflow**

Create `.github/workflows/manifest-equivalence.yml`:

```yaml
name: manifest-equivalence

# Temporary gate for docs/superpowers/plans/2026-08-05-manifest-and-build-driver.md.
# Proves scripts/epic_build.py reproduces every mcu/*-mplabx/Makefile's
# .hex byte for byte before those Makefiles are deleted. Removed in that
# plan's Task 11, once the Makefiles are gone and there is no longer an
# old path to compare against.
#
# The manifest is resolved into build scripts on the runner (which has
# python3) and only the resulting sh scripts are executed in the
# toolchain container (which deliberately does not).

on:
  push:
    branches: [master]
  pull_request:
  workflow_dispatch:

permissions:
  contents: read
  packages: read

jobs:
  toolchain-image:
    runs-on: ubuntu-latest
    outputs:
      image: ${{ steps.resolve.outputs.image }}
    steps:
      - uses: actions/checkout@v4
      - id: resolve
        run: |
          xc8_version="$(grep -m1 '^ARG XC8_VERSION=' docker/ci-toolchain/Dockerfile | cut -d= -f2)"
          pic16_dfp_version="$(grep -m1 '^ARG PIC16FXXX_DFP_VERSION=' docker/ci-toolchain/Dockerfile | cut -d= -f2)"
          pic18_dfp_version="$(grep -m1 '^ARG PIC18FXXXX_DFP_VERSION=' docker/ci-toolchain/Dockerfile | cut -d= -f2)"
          pic1216f1_dfp_version="$(grep -m1 '^ARG PIC12_16F1XXX_DFP_VERSION=' docker/ci-toolchain/Dockerfile | cut -d= -f2)"
          mplabx_version="$(grep -m1 '^ARG MPLABX_VERSION=' docker/ci-toolchain/Dockerfile | cut -d= -f2)"
          tag="xc8-v${xc8_version}-dfp${pic16_dfp_version}-${pic18_dfp_version}-${pic1216f1_dfp_version}-mplabx${mplabx_version}"
          echo "image=ghcr.io/${{ github.repository_owner }}/pic8-hal-ci:${tag}" >> "$GITHUB_OUTPUT"
      - run: echo "${{ secrets.GITHUB_TOKEN }}" | docker login ghcr.io -u "${{ github.actor }}" --password-stdin
      - run: docker pull "${{ steps.resolve.outputs.image }}"

  emit:
    runs-on: ubuntu-latest
    outputs:
      pairs: ${{ steps.pairs.outputs.pairs }}
    steps:
      - uses: actions/checkout@v4
      - id: pairs
        name: Emit a build script for every supported pair
        run: |
          python3 - <<'PY' > pairs.txt
          import subprocess, sys
          sys.path.insert(0, "scripts")
          import epicmanifest
          m = epicmanifest.load(epicmanifest.default_path())
          for name in sorted(m.modules):
              mod = m.modules[name]
              if mod.example is None:
                  continue
              for fam_name, mcus in sorted(mod.supported.items()):
                  fam = m.families[fam_name]
                  key = fam.hal_dir.replace("-hal", "")
                  d = f"{name}/mcu/{key}-{name.replace('epic-','')}-mplabx"
                  print(f"{d}={','.join(mcus)}")
                  for mcu in mcus:
                      subprocess.run(
                          ["python3", "scripts/epic_build.py", "build",
                           "--module", name, "--mcu", mcu,
                           "--build-dir", f"build-manifest/{name}"],
                          check=True,
                      )
          PY
          cat pairs.txt
          echo "pairs<<EOF" >> "$GITHUB_OUTPUT"
          cat pairs.txt >> "$GITHUB_OUTPUT"
          echo "EOF" >> "$GITHUB_OUTPUT"
      - uses: actions/upload-artifact@v4
        with:
          name: emitted-scripts
          path: |
            build-manifest/
            pairs.txt

  compare:
    needs: [toolchain-image, emit]
    runs-on: ubuntu-latest
    container:
      image: ${{ needs.toolchain-image.outputs.image }}
      credentials:
        username: ${{ github.actor }}
        password: ${{ secrets.GITHUB_TOKEN }}
    steps:
      - uses: actions/checkout@v4
      - uses: actions/download-artifact@v4
        with:
          name: emitted-scripts
      - name: Compare .hex from both build paths
        shell: bash
        run: |
          chmod +x scripts/equivalence-gate.sh
          DFP_ROOT="$XC8_INSTALL_DIR/pic/packs" \
            sh scripts/equivalence-gate.sh < pairs.txt | tee gate.log
          {
            echo '### Manifest equivalence'
            echo '```'
            tail -30 gate.log
            echo '```'
          } >> "$GITHUB_STEP_SUMMARY"
```

- [ ] **Step 3: Commit and push to trigger the gate**

```bash
git add scripts/equivalence-gate.sh .github/workflows/manifest-equivalence.yml
git commit -m "test(manifest): gate the migration on byte-identical .hex output"
git push
```

- [ ] **Step 4: Read the gate result and fix every diff**

Open the `manifest-equivalence` run's `compare` job summary.

Expected on first run: some `FAIL ... (.hex differs)` lines. That is the gate working. For each one, the cause is almost always one of four things, in decreasing order of likelihood:

1. **Source order differs.** XC8 lays out psects in link order, so a reordered `.p1` list changes the `.hex`. Compare the emitted `build.sh`'s compile order against `make -C <dir> -f Makefile -f - MCU=<mcu> print-SRCS <<< 'print-%: ; @echo $($*)'`. Fix by reordering `hal_sources` in the manifest to match.
2. **A flag differs.** Diff the emitted `cflags` string against the Makefile's `CFLAGS`. Fix in `emit_build_script`.
3. **A source is missing or extra.** Fix the manifest's `hal_sources`, `conditional_sources`, or the module's `sources`.
4. **Config pragmas differ or are ordered differently.** Compare the emitted `build-manifest/<MCU>/config_<MCU>.c` against `<dir>/build/config_<MCU>.c`. TOML preserves table order, so fix by reordering the manifest's `example.config` entries.

Iterate until the summary reads `equivalence: N identical, 0 differing`, where N is every supported pair. Commit each fix separately with a message naming what disagreed, for example:

```bash
git commit -m "fix(manifest): match HAL source order to the PIC16 Makefile"
```

- [ ] **Step 5: Confirm the gate is fully green**

Expected final summary line: `equivalence: 72 identical, 0 differing` (the exact count is however many supported pairs the manifest declares; it must equal the number of pairs, with zero differing and zero unexplained SKIPs).

---

## Task 7: Wire the root Makefile and document the driver

**Files:**
- Modify: root `Makefile`
- Modify: `scripts/README.md`

**Interfaces:**
- Consumes: `epic_build.py build` from Task 5.
- Produces: `make xc8-build MODULE=<name> MCU=<mcu>` driving the new path.

- [ ] **Step 1: Read the existing target**

```bash
grep -n "xc8-build" -A 20 Makefile | head -40
```

Note the exact `docker run` invocation it uses; the replacement keeps it and changes only what runs inside.

- [ ] **Step 2: Replace the xc8-build target**

Change the `xc8-build` target's body to resolve on the host and execute in the container. `MODULE` now takes a module name (`epic-serial`), not a path to an `mcu/` directory:

```make
# Real-target build. Resolution runs on the host (needs python3), the
# emitted sh script runs in the container (which has xc8-cc and no
# python3, see docker/ci-toolchain/Dockerfile). MODULE is a manifest
# module name, e.g. epic-serial, not a path: the mcu/*-mplabx dirs it
# used to name are gone.
.PHONY: xc8-build
xc8-build:
	@test -n "$(MODULE)" || { echo "usage: make xc8-build MODULE=epic-serial MCU=16F877A" >&2; exit 1; }
	@test -n "$(MCU)" || { echo "usage: make xc8-build MODULE=epic-serial MCU=16F877A" >&2; exit 1; }
	python3 scripts/epic_build.py build --module $(MODULE) --mcu $(MCU) \
	  --dfp-dir "/opt/microchip/xc8/v4.00/pic/packs/$$(python3 -c "import sys; sys.path.insert(0,'scripts'); import epicmanifest as e; m=e.load(e.default_path()); print(m.family_of('$(MCU)').dfp)")/xc8"
	docker run --rm -v "$(PWD)":/repo -w /repo $(IMAGE) \
	  sh build/$(MCU)/build.sh
```

- [ ] **Step 3: Verify the usage errors**

```bash
make xc8-build 2>&1 | head -2
```

Expected: `usage: make xc8-build MODULE=epic-serial MCU=16F877A`.

- [ ] **Step 4: Document the driver in scripts/README.md**

Add a section to `scripts/README.md`:

```markdown
## `epic_build.py`, the real-target build driver

Reads `epic-common/manifest/modules.toml`, resolves a module's
dependencies and sources, and emits a POSIX `sh` script of `xc8-cc`
invocations. Replaces the `mcu/*-mplabx/Makefile`s.

```sh
python3 scripts/epic_build.py build --module epic-serial --mcu 16F877A --run
python3 scripts/epic_build.py matrix          # CI matrix JSON
python3 scripts/epic_build.py report --log build/16F877A/build.log
```

Without `--run` it only writes `build/<MCU>/build.sh`, which is the mode
CI uses: resolution needs python3, execution does not, and the toolchain
container has no python3 in it. Reading that script is also the fastest
way to see the exact command line for any translation unit.

An unsupported `(module, MCU)` pair fails immediately with the reason
recorded in the manifest, rather than as a wall of XC8 linker errors.
```

- [ ] **Step 5: Commit**

```bash
git add Makefile scripts/README.md
git commit -m "feat(manifest): drive make xc8-build through epic_build.py"
```

---

## Task 8: Replace the CI matrix source

**Files:**
- Modify: `.github/workflows/xc8-build.yml`
- Delete: `scripts/ci-discover-xc8-matrix.py`

**Interfaces:**
- Consumes: `epic_build.py matrix` from Task 5, whose output shape (`[{"family", "dfp", "modules"}]` with `modules` as `"name=mcu,mcu;name=mcu"`) intentionally matches what `ci-discover-xc8-matrix.py` printed, so the workflow's bash parsing is unchanged.
- Produces: `xc8-build.yml` building from the manifest.

The one shape change: `modules` entries are now module **names** (`epic-serial`), not directory paths (`epic-serial/mcu/pic16f87xa-serial-mplabx`). The build loop changes accordingly.

- [ ] **Step 1: Verify the matrix output matches the old one**

```bash
python3 scripts/epic_build.py matrix | python3 -m json.tool
python3 scripts/ci-discover-xc8-matrix.py | python3 -m json.tool
```

Expected: the same families and the same MCU lists per module, with names in place of paths. Any module present in one and absent from the other means the manifest's `supported` tables disagree with `KNOWN_BROKEN`; fix the manifest.

- [ ] **Step 2: Point the discover job at the driver**

In `.github/workflows/xc8-build.yml`, in the `discover` job, replace:

```yaml
          matrix="$(python3 scripts/ci-discover-xc8-matrix.py)"
```

with:

```yaml
          matrix="$(python3 scripts/epic_build.py matrix)"
```

- [ ] **Step 3: Emit build scripts in the discover job**

Add this step to the `discover` job, after the `discover` step, so the container has scripts to run:

```yaml
      - name: Emit a build script for every matrix pair
        run: |
          python3 - <<'PY'
          import subprocess, sys
          sys.path.insert(0, "scripts")
          import epicmanifest
          m = epicmanifest.load(epicmanifest.default_path())
          for name in sorted(m.modules):
              if m.modules[name].example is None:
                  continue
              for fam_name, mcus in m.modules[name].supported.items():
                  dfp = m.families[fam_name].dfp
                  for mcu in mcus:
                      subprocess.run(
                          ["python3", "scripts/epic_build.py", "build",
                           "--module", name, "--mcu", mcu,
                           "--build-dir", f"build/{name}",
                           "--dfp-dir", f"/opt/microchip/xc8/v4.00/pic/packs/{dfp}/xc8"],
                          check=True,
                      )
          PY
      - uses: actions/upload-artifact@v4
        with:
          name: build-scripts
          path: build/
```

- [ ] **Step 4: Change the build job's loop**

In the `build` job, add a download step before the build step:

```yaml
      - uses: actions/download-artifact@v4
        with:
          name: build-scripts
          path: build/
```

and replace the inner loop body (the `make -C` invocation and its `.hex` check) with:

```bash
              rm -rf "build/${dir}/${mcu}"/*.p1
              if EPIC_REPO_ROOT="$PWD" sh "build/${dir}/${mcu}/build.sh" \
                 && [ "$(ls build/${dir}/${mcu}-*.hex 2>/dev/null | wc -l)" -eq 1 ]; then
```

Here `dir` is the module name from the matrix string. Update the surrounding `echo` lines and `$GITHUB_STEP_SUMMARY` rows to say `Module` rather than a path; the table header becomes `| Module | MCU | Result |` (unchanged) and the rows now carry `epic-serial` instead of a directory.

Also update the `DFP_DIR` comment above the loop: the driver now bakes the DFP path into the emitted script, so the workflow no longer passes it.

- [ ] **Step 5: Delete the old discovery script**

```bash
git rm scripts/ci-discover-xc8-matrix.py
```

- [ ] **Step 6: Push and confirm xc8-build is green**

```bash
git add .github/workflows/xc8-build.yml
git commit -m "refactor(ci): build xc8-build's matrix from the manifest"
git push
```

Expected: the `xc8-build` workflow passes with the same PASS rows as before the change, module names in place of paths, and no FAIL rows. Compare the row count against the previous run on `master`; it must match.

---

## Task 9: Retarget sim-tests

**Files:**
- Modify: `.github/workflows/sim-tests.yml`

**Interfaces:**
- Consumes: the `.hex` paths the driver produces (`build/<module>/<MCU>-<example>.hex`).
- Produces: `sim-tests` running against manifest-built firmware.

- [ ] **Step 1: Find the current hex paths**

```bash
grep -n "hex\|MODULE\|make -C" .github/workflows/sim-tests.yml
```

Note every place a `.hex` path or a `make -C <mcu dir>` appears.

- [ ] **Step 2: Replace the build step**

Wherever `sim-tests.yml` builds firmware with `make -C <module>/mcu/<...>`, replace it with the two-phase pattern: resolve on the runner, execute in the container. If the workflow's build and simulate steps are in the same containerised job, emit the script in a preceding non-container job and pass it as an artifact, exactly as Task 8 does. If the build already happens on a bare runner, call:

```bash
python3 scripts/epic_build.py build --module "$MODULE" --mcu "$MCU" \
  --build-dir "build/$MODULE" \
  --dfp-dir "/opt/microchip/xc8/v4.00/pic/packs/$DFP/xc8"
```

- [ ] **Step 3: Update the hex paths handed to sim-mdb-run.sh**

Old paths were `<module>/mcu/<family>-<module>-mplabx/build/<MCU>-<name>.hex`. New paths are `build/<module>/<MCU>-<name>.hex`. Update every reference, including any in `scripts/sim-mdb-run.sh` if it constructs paths itself:

```bash
grep -n "mplabx/build\|/build/" scripts/sim-mdb-run.sh
```

- [ ] **Step 4: Push and confirm sim-tests is green**

```bash
git add .github/workflows/sim-tests.yml scripts/sim-mdb-run.sh
git commit -m "refactor(ci): point sim-tests at manifest-built firmware"
git push
```

Expected: `sim-tests` passes with the same PASS output as before. This workflow checks real register and UART values under MPLAB SIM, so a pass here is meaningful evidence the manifest build produces working firmware, not merely a linking one.

---

## Task 10: Delete the Makefiles

**Files:**
- Delete: all 29 `*/mcu/*-mplabx/Makefile`
- Delete: `epic-common/mk/epic_family.mk`
- Delete: any `*/mcu/*-mplabx/README.md` that documents `make MCU=`

**Interfaces:**
- Consumes: a fully green gate from Task 6 and green CI from Tasks 8 and 9.
- Produces: a repo with one real-target build path.

**Do not start this task until Task 6's gate reports zero differing pairs and Tasks 8 and 9 are green on `master`.** This is the irreversible step.

- [ ] **Step 1: Confirm the preconditions**

```bash
gh run list --workflow=manifest-equivalence.yml --limit 1
gh run list --workflow=xc8-build.yml --limit 1
gh run list --workflow=sim-tests.yml --limit 1
```

Expected: all three most recent runs `completed  success`. If any is not, stop and fix it first.

- [ ] **Step 2: Delete the Makefiles and the shared fragment**

```bash
git rm $(git ls-files '*/mcu/*-mplabx/Makefile')
git rm epic-common/mk/epic_family.mk
```

- [ ] **Step 3: Check what still references them**

```bash
grep -rn "epic_family.mk\|mcu/.*-mplabx" --include="*.md" --include="*.yml" --include="Makefile" . \
  | grep -v third_party | grep -v docs/superpowers
```

Every hit is a doc or workflow to fix in Task 11. Record the list; do not fix them yet if they are prose.

- [ ] **Step 4: Update the mcu README files**

Each `*/mcu/*-mplabx/README.md` documents a `make MCU=` workflow that no longer exists. For each, either delete it (if the directory is now empty) or replace its build instructions with:

```markdown
Build this example with the manifest-driven driver, from the repo root:

```sh
python3 scripts/epic_build.py build --module <module> --mcu <MCU> --run
```

The `Makefile` this directory used to hold is gone; see
`epic-common/manifest/README.md`.
```

If a directory ends up holding only a README that says "this is gone", delete the directory instead.

**One exception, do not delete it:** `pic16f87xa-hal/mcu/pic16f87xa-mplabx/` also contains an `nbproject/` directory, the only real MPLAB X project in the repo. Its `Makefile` goes with the rest, but `nbproject/` stays: a later plan promotes it into that family's reference `.X` project. Verify before committing:

```bash
test -d pic16f87xa-hal/mcu/pic16f87xa-mplabx/nbproject \
  && echo "OK: nbproject preserved" || echo "FAIL: nbproject was deleted"
```

Expected: `OK: nbproject preserved`.

- [ ] **Step 5: Confirm nothing builds via make any more**

```bash
git ls-files '*/mcu/*' | head -20
python3 scripts/epic_build.py matrix | python3 -m json.tool | head -20
```

Expected: no `Makefile` entries remain under `mcu/`, and the matrix still lists every module.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "refactor(manifest): delete the 29 mcu/*-mplabx Makefiles"
git push
```

Expected: `xc8-build` and `sim-tests` stay green. `manifest-equivalence` will now fail, because there is no old path left to compare against; that is expected and Task 11 removes it.

---

## Task 11: Remove the scaffolding and update the docs

**Files:**
- Delete: `scripts/bootstrap_manifest.py`, `scripts/equivalence-gate.sh`, `.github/workflows/manifest-equivalence.yml`
- Modify: `docs/mplabx-link-gaps-plan.md`, `docs/ci-plan.md`, `README.md`, `AGENTS.md`

**Interfaces:**
- Consumes: everything above.
- Produces: a repo whose documentation matches its build.

Repo convention: a change is not done until the docs it touches are updated. This task is not optional.

- [ ] **Step 1: Delete the migration scaffolding**

```bash
git rm scripts/bootstrap_manifest.py scripts/equivalence-gate.sh \
       .github/workflows/manifest-equivalence.yml
```

- [ ] **Step 2: Update docs/mplabx-link-gaps-plan.md**

Change its `Status:` line to note that tracking moved, and rewrite the "Next steps" section's item 3. The exclusion list is no longer in `ci-discover-xc8-matrix.py`; replace that reference with:

```markdown
3. As each module gets fixed, remove its entry from the `excluded` table
   in `epic-common/manifest/modules.toml` and add the part back to that
   module's `supported` list, so `xc8-build.yml` starts covering it
   again. Every `excluded` table being empty is this document's exit
   criterion. (Was `scripts/ci-discover-xc8-matrix.py`'s `KNOWN_BROKEN`
   set before the build manifest replaced it, see
   `epic-common/manifest/README.md`.)
```

Also fix root cause 1's **Fix** paragraph: `EPIC_SOURCES` lists no longer exist, so the fix is now "the manifest already compiles the family's full HAL set, so this class of failure cannot recur; what remains is the design question of whether a per-module dispatch is worth building."

- [ ] **Step 3: Update docs/ci-plan.md**

Replace every reference to `ci-discover-xc8-matrix.py` with `scripts/epic_build.py matrix`, and every reference to per-module `make -C` with the emit-then-run split. Add a short paragraph under the Phase 1 section recording why the split exists (no python3 in the toolchain container).

- [ ] **Step 4: Update README.md**

In the "Real hardware" section, replace:

````markdown
```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
cd epic-taskmgr/mcu/pic16f87xa-taskmgr-mplabx
make MCU=16F877A        # also 873A / 874A / 876A
```
````

with:

````markdown
```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
python3 scripts/epic_build.py build --module epic-taskmgr --mcu 16F877A --run
```
````

and change the following sentence's `Produces build/<MCU>-multi-blink.hex` to the new path, `build/16F877A-multi-blink.hex`.

In the "Development" section's Docker block, update the `xc8-build` and `mdb-test` example lines to use module names rather than `mcu/` paths.

- [ ] **Step 5: Update AGENTS.md**

Three passages are now wrong. In "Module anatomy", replace "real-target via `mcu/<family>-*-mplabx/Makefile` (`make MCU=...`)" with "real-target via `python3 scripts/epic_build.py build --module <name> --mcu <MCU> --run`". In "Build & toolchain", replace the `make MCU=...` instruction the same way. In "Development cycle", update the "Real-target build" paragraph.

Add one line to "Non-obvious things that will bite you":

```markdown
- **The toolchain container has no python3.** `epic_build.py` therefore
  resolves the manifest and emits a `sh` script rather than calling
  `xc8-cc` itself; resolution runs on the host or CI runner, execution
  runs in the container. Do not "simplify" this into a direct call.
```

- [ ] **Step 6: Verify no stale references remain**

```bash
grep -rn "epic_family.mk\|ci-discover-xc8-matrix\|mcu/.*-mplabx/Makefile\|make MCU=" \
  --include="*.md" --include="*.yml" --include="Makefile" . \
  | grep -v third_party | grep -v docs/superpowers
```

Expected: no output. Any hit is a doc still describing the old build.

- [ ] **Step 7: Verify the whole repo still works**

```bash
python3 -m unittest discover -s scripts/tests -v
python3 scripts/epic_build.py matrix | python3 -m json.tool | head
cmake -B /tmp/hosttest -S epic-taskmgr && cmake --build /tmp/hosttest && (cd /tmp/hosttest && ctest)
```

Expected: unit tests pass, matrix prints three families, host build and ctest pass (proving the CMake side was genuinely untouched).

- [ ] **Step 8: Commit**

```bash
git add -A
git commit -m "docs(manifest): update every doc that described the Makefile build"
git push
```

Expected: `host-tests`, `xc8-build`, and `sim-tests` all green, and `manifest-equivalence` gone from the workflow list.

---

## Done when

- `epic-common/manifest/modules.toml` describes all three families and all 13 modules.
- No `*/mcu/*-mplabx/Makefile` and no `epic-common/mk/epic_family.mk` remain.
- `scripts/ci-discover-xc8-matrix.py` is gone; `epic_build.py matrix` feeds `xc8-build.yml`.
- `host-tests`, `xc8-build`, and `sim-tests` are green on `master`, with `xc8-build` reporting the same set of passing pairs as before the migration.
- Every `KNOWN_BROKEN` pair is an `excluded` entry with a human-readable reason, and none has been repaired.
- `python3 -m unittest discover -s scripts/tests` passes.

## What this plan deliberately does not do

- Fix any excluded `(module, MCU)` pair.
- Redesign `pic16_irq_dispatch.c`'s strong-reference contract.
- Trim the HAL peripheral set per module.
- Generate release bundles, `epicurus.mk`, or MPLAB X projects. Those are the next plan.
