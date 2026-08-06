# Bundle Generator Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Generate a self-contained, per-family source bundle that an outside project can drop in and build against with bare XC8, and prove it is self-contained by building it outside the repo.

**Architecture:** `scripts/bundlegen.py` reads the manifest through `epicmanifest.py` and emits a bundle tree: the family's HAL, `epic-common`, every module supported on that family, and five generated files (`epicurus.mk`, `epicurus-sources.json`, `SUPPORT.md`, `MPLABX.md`, `QUICKSTART.md`). Module dependencies are flattened at generation time, so the emitted `epicurus.mk` is a table of precomputed variables plus a guard, with no recursive make logic.

**Tech Stack:** Python 3.11+ (stdlib only), GNU make (the emitted fragment's consumer), MPLAB XC8 v4.00, Docker, GitHub Actions.

**Design spec:** `docs/superpowers/specs/2026-08-05-distribution-design.md`, phase 3. Read it before starting.

**Prerequisite:** `docs/superpowers/plans/2026-08-05-manifest-and-build-driver.md` must be complete. This plan imports `scripts/epicmanifest.py` and assumes `epic-common/manifest/modules.toml` exists and validates.

## Global Constraints

- **No em-dashes.** Not in docs, not in commit messages, not in code comments. A pre-commit hook rejects them.
- **Python 3.11 minimum**, stdlib only. Tests use `unittest`.
- **The toolchain container has no python3.** Bundle generation runs on a host or CI runner; only the emitted `sh`/`make` artifacts run in the container. Do not modify `docker/ci-toolchain/Dockerfile`.
- **Nothing generated is committed.** Bundles are build outputs. Add `bundles/` to `.gitignore`.
- **A bundle must be self-contained.** No path inside a bundle may reference anything outside it.
- **Conventional Commits**, trailing newline, no trailing whitespace.
- **Spec naming note:** the spec calls the generator `scripts/make-bundle.sh`. It is Python here, `scripts/make_bundle.py`, because it reads the manifest through `epicmanifest.py`. Same role, different language.

## File Structure

| Path | Responsibility |
|---|---|
| `scripts/bundlegen.py` | All generation logic, importable and testable. Knows the bundle layout and the format of each generated file. |
| `scripts/make_bundle.py` | Thin CLI over `bundlegen`. Copies the tree, writes the generated files. |
| `scripts/tests/test_bundlegen.py` | Unit tests for every emitted artifact. |
| `.github/workflows/bundle-gate.yml` | Builds each bundle from a scratch copy outside the repo. |
| `.gitignore` | Add `bundles/`. |

---

## Task 1: Bundle content resolution

**Files:**
- Create: `scripts/bundlegen.py`
- Create: `scripts/tests/test_bundlegen.py`

**Interfaces:**
- Consumes: `epicmanifest.load`, `Manifest.families`, `Manifest.modules`, `Manifest.resolve_deps`, `Manifest.sources_for`, `Manifest.includes_for`, `Manifest.family_of`, `Module.supported`, `Module.excluded`, `Module.example` from the previous plan.
- Produces:
  - `modules_for_family(manifest, family_name) -> list[str]`, sorted, every module with at least one supported part in that family.
  - `files_for_family(manifest, family_name) -> list[str]`, sorted repo-root-relative paths of every file the bundle must copy.
  - `class BundleError(Exception)`

- [ ] **Step 1: Write the failing tests**

Create `scripts/tests/test_bundlegen.py`:

```python
"""Unit tests for scripts/bundlegen.py."""
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

import bundlegen  # noqa: E402
import epicmanifest  # noqa: E402

MANIFEST = """
[families.PIC16F87XA]
hal_dir  = "pic16f87xa-hal"
variants = ["16F873A", "16F877A"]
dfp      = "Microchip.PIC16Fxxx_DFP"
includes = ["pic16f87xa-hal/include/target", "pic16f87xa-hal/include",
            "epic-common/include"]
hal_sources = ["pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c"]

[families.PIC18Fxx5x]
hal_dir  = "pic18fxx5x-hal"
variants = ["18F4550"]
dfp      = "Microchip.PIC18Fxxxx_DFP"
includes = ["pic18fxx5x-hal/include/target", "epic-common/include"]
hal_sources = ["pic18fxx5x-hal/src/peripherals/pic18fxx5x_gpio.c"]

[modules.epic-tick]
dir        = "epic-tick"
sources    = ["src/epic_tick.c"]
includes   = ["include"]
depends_on = []

[modules.epic-tick.supported]
PIC16F87XA = ["16F873A", "16F877A"]
PIC18Fxx5x = ["18F4550"]

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
"16F873A" = "RAM: 32-byte g_rx_buf does not fit"

[modules.epic-usb]
dir        = "epic-usb"
sources    = ["src/epic_usb.c"]
includes   = ["include"]
depends_on = []

[modules.epic-usb.supported]
PIC18Fxx5x = ["18F4550"]
"""


def load():
    tmp = tempfile.NamedTemporaryFile("w", suffix=".toml", delete=False)
    tmp.write(MANIFEST)
    tmp.close()
    return epicmanifest.load(pathlib.Path(tmp.name))


class TestModuleSelection(unittest.TestCase):
    def setUp(self):
        self.m = load()

    def test_includes_modules_supported_on_the_family(self):
        self.assertEqual(
            bundlegen.modules_for_family(self.m, "PIC16F87XA"),
            ["epic-serial", "epic-tick"],
        )

    def test_excludes_modules_with_no_supported_part(self):
        self.assertNotIn(
            "epic-usb", bundlegen.modules_for_family(self.m, "PIC16F87XA")
        )

    def test_other_family_gets_its_own_module_set(self):
        self.assertEqual(
            bundlegen.modules_for_family(self.m, "PIC18Fxx5x"),
            ["epic-tick", "epic-usb"],
        )

    def test_unknown_family_raises(self):
        with self.assertRaises(bundlegen.BundleError):
            bundlegen.modules_for_family(self.m, "PIC99XXXX")


class TestFileSelection(unittest.TestCase):
    def setUp(self):
        self.files = bundlegen.files_for_family(load(), "PIC16F87XA")

    def test_includes_family_hal_sources(self):
        self.assertIn("pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c", self.files)

    def test_includes_module_sources(self):
        self.assertIn("epic-serial/src/epic_serial.c", self.files)
        self.assertIn("epic-tick/src/epic_tick.c", self.files)

    def test_includes_example_sources(self):
        self.assertIn("epic-tick/examples/example_tick.c", self.files)

    def test_excludes_other_families_hal(self):
        self.assertFalse(any(f.startswith("pic18fxx5x-hal/") for f in self.files))

    def test_excludes_modules_not_supported_here(self):
        self.assertFalse(any(f.startswith("epic-usb/") for f in self.files))

    def test_is_sorted_and_deduplicated(self):
        self.assertEqual(self.files, sorted(set(self.files)))


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `python3 -m unittest scripts.tests.test_bundlegen -v` from the repo root, or `python3 -m unittest discover -s scripts/tests -v`.
Expected: FAIL, `ModuleNotFoundError: No module named 'bundlegen'`.

- [ ] **Step 3: Implement resolution**

Create `scripts/bundlegen.py`:

```python
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
    """Every module with at least one supported part in this family."""
    _family(manifest, family_name)
    return sorted(
        name for name, mod in manifest.modules.items()
        if mod.supported.get(family_name)
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
        if mod.example is not None:
            files |= {f"{mod.dir}/{s}" for s in mod.example.sources}

    return sorted(files)
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `python3 -m unittest discover -s scripts/tests -v`
Expected: PASS, 10 new tests (4 in `TestModuleSelection`, 6 in `TestFileSelection`) on top of the previous plan's 38.

- [ ] **Step 5: Commit**

```bash
git add scripts/bundlegen.py scripts/tests/test_bundlegen.py
git commit -m "feat(bundle): resolve per-family bundle contents from the manifest"
```

---

## Task 2: Emit `epicurus.mk`

**Files:**
- Modify: `scripts/bundlegen.py`
- Modify: `scripts/tests/test_bundlegen.py`

**Interfaces:**
- Consumes: `modules_for_family` from Task 1.
- Produces: `emit_epicurus_mk(manifest, family_name, version) -> str`.

The consumer contract, which the generated file must honour exactly:

```make
EPICURUS_DIR := third_party/epicurus
EPICURUS_MCU := 16F877A
EPICURUS_MODULES := serial tick
include $(EPICURUS_DIR)/epicurus.mk

SRCS := main.c $(EPICURUS_SRCS)
CFLAGS += $(EPICURUS_CFLAGS)
```

Module names in `EPICURUS_MODULES` are given **without** the `epic-` prefix, because that is what a consumer will reach for. The generated file maps both spellings.

Three requirements the tests pin down:

1. `EPICURUS_SRCS` paths are prefixed with `$(EPICURUS_DIR)`, so they resolve from the consumer's own working directory.
2. Dependencies are already flattened: naming `modbus` yields serial and tick sources without the consumer listing them.
3. An unsupported `(module, MCU)` is a hard `$(error ...)` carrying the manifest's reason.

- [ ] **Step 1: Write the failing tests**

Append to `scripts/tests/test_bundlegen.py`, before the `if __name__` block:

```python
class TestEpicurusMk(unittest.TestCase):
    def setUp(self):
        self.mk = bundlegen.emit_epicurus_mk(load(), "PIC16F87XA", "v0.1.0")

    def test_declares_the_family_and_version(self):
        self.assertIn("PIC16F87XA", self.mk)
        self.assertIn("v0.1.0", self.mk)

    def test_maps_short_module_names_to_full_ones(self):
        self.assertIn("EPICURUS_MODULE_serial := epic-serial", self.mk)
        self.assertIn("EPICURUS_MODULE_tick := epic-tick", self.mk)

    def test_flattens_dependencies_at_generation_time(self):
        self.assertIn(
            "EPICURUS_RESOLVED_epic-serial := epic-tick epic-serial", self.mk
        )

    def test_lists_supported_parts_per_module(self):
        self.assertIn(
            "EPICURUS_SUPPORTED_epic-serial := 16F877A", self.mk
        )
        self.assertIn(
            "EPICURUS_SUPPORTED_epic-tick := 16F873A 16F877A", self.mk
        )

    def test_carries_the_exclusion_reason(self):
        self.assertIn(
            "EPICURUS_WHYNOT_epic-serial_16F873A := "
            "RAM: 32-byte g_rx_buf does not fit",
            self.mk,
        )

    def test_hal_sources_are_prefixed_with_the_bundle_dir(self):
        self.assertIn(
            "$(EPICURUS_DIR)/pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c",
            self.mk,
        )

    def test_module_sources_are_prefixed_with_the_bundle_dir(self):
        self.assertIn("$(EPICURUS_DIR)/epic-serial/src/epic_serial.c", self.mk)

    def test_includes_are_prefixed_and_ordered(self):
        self.assertIn(
            "-I$(EPICURUS_DIR)/pic16f87xa-hal/include/target "
            "-I$(EPICURUS_DIR)/pic16f87xa-hal/include",
            self.mk,
        )

    def test_errors_on_an_unset_mcu(self):
        self.assertIn("EPICURUS_MCU is not set", self.mk)

    def test_errors_on_an_unsupported_pair(self):
        self.assertIn("$(error", self.mk)
        self.assertIn("is not supported on", self.mk)

    def test_defines_the_part_macro_and_dfp(self):
        self.assertIn("-DPIC$(EPICURUS_MCU)", self.mk)
        self.assertIn("Microchip.PIC16Fxxx_DFP", self.mk)
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `python3 -m unittest discover -s scripts/tests -v`
Expected: FAIL, `AttributeError: module 'bundlegen' has no attribute 'emit_epicurus_mk'`.

- [ ] **Step 3: Implement the emitter**

Append to `scripts/bundlegen.py`:

```python
def emit_epicurus_mk(manifest, family_name: str, version: str) -> str:
    """The consumer-facing make fragment.

    Data plus a guard, no build rules: the consumer's own Makefile owns
    the rules, and this file must not assume anything about them beyond
    the ability to `include` it. Dependency resolution is already done
    (see EPICURUS_RESOLVED_*), so nothing here recurses.
    """
    fam = _family(manifest, family_name)
    modules = modules_for_family(manifest, family_name)

    out = [
        "# GENERATED by scripts/bundlegen.py. Do not edit.",
        f"# Epicurus {version}, {family_name} family.",
        "#",
        "# Usage from your own Makefile:",
        "#",
        "#   EPICURUS_DIR := third_party/epicurus",
        "#   EPICURUS_MCU := 16F877A",
        "#   EPICURUS_MODULES := serial tick",
        "#   include $(EPICURUS_DIR)/epicurus.mk",
        "#",
        "#   SRCS := main.c $(EPICURUS_SRCS)",
        "#   CFLAGS += $(EPICURUS_CFLAGS)",
        "#",
        "# Sets EPICURUS_SRCS, EPICURUS_INCLUDES, and EPICURUS_CFLAGS.",
        "# Module names are given without the epic- prefix.",
        "",
        f"EPICURUS_VERSION := {version}",
        f"EPICURUS_FAMILY  := {family_name}",
        f"EPICURUS_DFP     := {fam.dfp}",
        f"EPICURUS_VARIANTS := {' '.join(fam.variants)}",
        "",
        "ifeq ($(strip $(EPICURUS_DIR)),)",
        "  $(error EPICURUS_DIR is not set: point it at this bundle's root)",
        "endif",
        "ifeq ($(strip $(EPICURUS_MCU)),)",
        "  $(error EPICURUS_MCU is not set: choose one of $(EPICURUS_VARIANTS))",
        "endif",
        "",
        "# ---- module name mapping -------------------------------------",
    ]

    for name in modules:
        short = name.removeprefix("epic-")
        out.append(f"EPICURUS_MODULE_{short} := {name}")
    out.append("")

    out.append("# ---- per-module data -----------------------------------------")
    for name in modules:
        mod = manifest.modules[name]
        resolved = " ".join(manifest.resolve_deps(name))
        supported = " ".join(mod.supported.get(family_name, []))
        out.append(f"EPICURUS_RESOLVED_{name} := {resolved}")
        out.append(f"EPICURUS_SUPPORTED_{name} := {supported}")
        for mcu, reason in sorted(mod.excluded.items()):
            if mcu in fam.variants:
                out.append(f"EPICURUS_WHYNOT_{name}_{mcu} := {reason}")
        srcs = " ".join(f"$(EPICURUS_DIR)/{mod.dir}/{s}" for s in mod.sources)
        incs = " ".join(f"$(EPICURUS_DIR)/{mod.dir}/{i}" for i in mod.includes)
        out.append(f"EPICURUS_SRCS_{name} := {srcs}")
        out.append(f"EPICURUS_INCS_{name} := {incs}")
        out.append("")

    hal = " \\\n  ".join(f"$(EPICURUS_DIR)/{s}" for s in fam.hal_sources)
    out += [
        "# ---- family HAL ----------------------------------------------",
        "# The full peripheral set, not trimmed per module: the IRQ",
        "# dispatch takes strong references to every peripheral handler,",
        "# so a partial set will not link.",
        f"EPICURUS_HAL_SRCS := \\\n  {hal}",
        "",
    ]
    for cond in fam.conditional_sources:
        out.append(f"ifneq ($(filter $(EPICURUS_MCU),{' '.join(cond.variants)}),)")
        out.append(f"  EPICURUS_HAL_SRCS += $(EPICURUS_DIR)/{cond.path}")
        out.append("endif")
    out.append("")

    fam_incs = " ".join(f"-I$(EPICURUS_DIR)/{i}" for i in fam.includes)
    out += [
        "# Include order is significant: include/target must precede",
        "# include so the platform header resolves to the real-target",
        "# version, not the host memory-backed one.",
        f"EPICURUS_FAMILY_INCLUDES := {fam_incs}",
        "",
        "# ---- resolution ----------------------------------------------",
        "EPICURUS_SELECTED := $(foreach m,$(EPICURUS_MODULES),"
        "$(if $(EPICURUS_MODULE_$(m)),$(EPICURUS_MODULE_$(m)),"
        "$(error unknown Epicurus module '$(m)'; see SUPPORT.md)))",
        "",
        "EPICURUS_ALL := $(sort $(foreach m,$(EPICURUS_SELECTED),"
        "$(EPICURUS_RESOLVED_$(m))))",
        "",
        "# Every selected module must declare this part as supported.",
        "$(foreach m,$(EPICURUS_ALL),\\",
        "  $(if $(filter $(EPICURUS_MCU),$(EPICURUS_SUPPORTED_$(m))),,\\",
        "    $(error $(m) is not supported on $(EPICURUS_MCU)"
        "$(if $(EPICURUS_WHYNOT_$(m)_$(EPICURUS_MCU)),"
        " ($(EPICURUS_WHYNOT_$(m)_$(EPICURUS_MCU))),). "
        "Supported: $(if $(EPICURUS_SUPPORTED_$(m)),"
        "$(EPICURUS_SUPPORTED_$(m)),none). See SUPPORT.md.)))",
        "",
        "EPICURUS_SRCS := $(EPICURUS_HAL_SRCS) "
        "$(foreach m,$(EPICURUS_ALL),$(EPICURUS_SRCS_$(m)))",
        "EPICURUS_INCLUDES := $(EPICURUS_FAMILY_INCLUDES) "
        "$(foreach m,$(EPICURUS_ALL),-I$(EPICURUS_INCS_$(m)))",
        "",
        "EPICURUS_CFLAGS := $(EPICURUS_INCLUDES) -DPIC$(EPICURUS_MCU)",
        "",
    ]
    return "\n".join(out) + "\n"
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `python3 -m unittest discover -s scripts/tests -v`
Expected: PASS, 11 new tests.

- [ ] **Step 5: Commit**

```bash
git add scripts/bundlegen.py scripts/tests/test_bundlegen.py
git commit -m "feat(bundle): emit the consumer-facing epicurus.mk fragment"
```

---

## Task 3: Emit the JSON source list and SUPPORT.md

**Files:**
- Modify: `scripts/bundlegen.py`
- Modify: `scripts/tests/test_bundlegen.py`

**Interfaces:**
- Consumes: Tasks 1 and 2.
- Produces:
  - `emit_sources_json(manifest, family_name, version) -> str`
  - `emit_support_md(manifest, family_name, version) -> str`

The JSON is what makes the non-make personas work: MPLAB X users cannot `include` a `.mk`, and neither can someone driving XC8 from CMake or a shell script. It is also what `MPLABX.md` is generated from in Task 4, so those instructions cannot rot when a source moves.

Shape:

```json
{
  "version": "v0.1.0",
  "family": "PIC16F87XA",
  "dfp": "Microchip.PIC16Fxxx_DFP",
  "variants": ["16F873A", "16F877A"],
  "hal_sources": ["pic16f87xa-hal/src/..."],
  "conditional_sources": [{"path": "...", "variants": ["16F877A"]}],
  "family_includes": ["pic16f87xa-hal/include/target", "..."],
  "modules": {
    "epic-serial": {
      "resolved": ["epic-tick", "epic-serial"],
      "sources": ["epic-serial/src/epic_serial.c"],
      "includes": ["epic-serial/include"],
      "supported": ["16F877A"],
      "excluded": {"16F873A": "RAM: ..."}
    }
  }
}
```

All paths bundle-root-relative.

- [ ] **Step 1: Write the failing tests**

Append to `scripts/tests/test_bundlegen.py`:

```python
import json  # noqa: E402  (place with the other imports at the top)


class TestSourcesJson(unittest.TestCase):
    def setUp(self):
        self.doc = json.loads(
            bundlegen.emit_sources_json(load(), "PIC16F87XA", "v0.1.0")
        )

    def test_carries_version_family_and_dfp(self):
        self.assertEqual(self.doc["version"], "v0.1.0")
        self.assertEqual(self.doc["family"], "PIC16F87XA")
        self.assertEqual(self.doc["dfp"], "Microchip.PIC16Fxxx_DFP")

    def test_lists_hal_and_conditional_sources(self):
        self.assertIn(
            "pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c",
            self.doc["hal_sources"],
        )
        self.assertIsInstance(self.doc["conditional_sources"], list)

    def test_module_entry_is_complete(self):
        entry = self.doc["modules"]["epic-serial"]
        self.assertEqual(entry["resolved"], ["epic-tick", "epic-serial"])
        self.assertEqual(entry["sources"], ["epic-serial/src/epic_serial.c"])
        self.assertEqual(entry["includes"], ["epic-serial/include"])
        self.assertEqual(entry["supported"], ["16F877A"])
        self.assertEqual(
            entry["excluded"]["16F873A"], "RAM: 32-byte g_rx_buf does not fit"
        )

    def test_omits_modules_from_other_families(self):
        self.assertNotIn("epic-usb", self.doc["modules"])

    def test_paths_are_bundle_relative(self):
        for path in self.doc["hal_sources"]:
            self.assertFalse(path.startswith("/"))
            self.assertNotIn("..", path)


class TestSupportMd(unittest.TestCase):
    def setUp(self):
        self.md = bundlegen.emit_support_md(load(), "PIC16F87XA", "v0.1.0")

    def test_has_a_row_per_module(self):
        self.assertIn("epic-serial", self.md)
        self.assertIn("epic-tick", self.md)

    def test_marks_supported_and_unsupported_parts(self):
        self.assertIn("yes", self.md)
        self.assertIn("no", self.md)

    def test_lists_every_exclusion_reason(self):
        self.assertIn("RAM: 32-byte g_rx_buf does not fit", self.md)

    def test_names_the_variants_as_columns(self):
        self.assertIn("16F873A", self.md)
        self.assertIn("16F877A", self.md)

    def test_has_no_em_dash(self):
        self.assertNotIn(chr(0x2014), self.md)  # em-dash, repo convention
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `python3 -m unittest discover -s scripts/tests -v`
Expected: FAIL, `AttributeError: module 'bundlegen' has no attribute 'emit_sources_json'`.

- [ ] **Step 3: Implement both emitters**

Append to `scripts/bundlegen.py` (and add `import json` at the top of the file):

```python
def emit_sources_json(manifest, family_name: str, version: str) -> str:
    """The same resolved data as epicurus.mk, for non-make consumers.

    MPLAB X users cannot include a .mk, and neither can someone driving
    XC8 from CMake or a shell script. MPLABX.md is generated from this,
    so its instructions cannot go stale when a source file moves.
    """
    fam = _family(manifest, family_name)
    modules = {}
    for name in modules_for_family(manifest, family_name):
        mod = manifest.modules[name]
        modules[name] = {
            "resolved": manifest.resolve_deps(name),
            "sources": [f"{mod.dir}/{s}" for s in mod.sources],
            "includes": [f"{mod.dir}/{i}" for i in mod.includes],
            "supported": mod.supported.get(family_name, []),
            "excluded": {
                mcu: reason for mcu, reason in sorted(mod.excluded.items())
                if mcu in fam.variants
            },
            "example": None if mod.example is None else {
                "name": mod.example.name,
                "sources": [f"{mod.dir}/{s}" for s in mod.example.sources],
                "config": mod.example.config.get(family_name, {}),
            },
        }

    doc = {
        "version": version,
        "family": family_name,
        "dfp": fam.dfp,
        "variants": fam.variants,
        "hal_sources": fam.hal_sources,
        "conditional_sources": [
            {"path": c.path, "variants": c.variants}
            for c in fam.conditional_sources
        ],
        "family_includes": fam.includes,
        "modules": modules,
    }
    return json.dumps(doc, indent=2, sort_keys=True) + "\n"


def emit_support_md(manifest, family_name: str, version: str) -> str:
    """The per-module, per-part support table, with reasons."""
    fam = _family(manifest, family_name)
    modules = modules_for_family(manifest, family_name)

    out = [
        f"# Supported parts, Epicurus {version} ({family_name})",
        "",
        "Generated from `epic-common/manifest/modules.toml`. A `no` here",
        "is a combination that genuinely does not build, not one that is",
        "merely untested: asking for it fails immediately with the reason",
        "rather than as a wall of XC8 linker errors.",
        "",
        "| Module | " + " | ".join(fam.variants) + " |",
        "|---" * (len(fam.variants) + 1) + "|",
    ]
    for name in modules:
        mod = manifest.modules[name]
        supported = mod.supported.get(family_name, [])
        cells = ["yes" if v in supported else "no" for v in fam.variants]
        out.append(f"| `{name}` | " + " | ".join(cells) + " |")

    reasons = [
        (name, mcu, reason)
        for name in modules
        for mcu, reason in sorted(manifest.modules[name].excluded.items())
        if mcu in fam.variants
    ]
    if reasons:
        out += ["", "## Why not", "", "| Module | Part | Reason |", "|---|---|---|"]
        out += [f"| `{n}` | {m} | {r} |" for n, m, r in reasons]

    out += [
        "",
        "## Selecting modules",
        "",
        "Dependencies resolve automatically: naming `modbus` pulls in",
        "`serial` and `tick`. List only what you use directly.",
        "",
        "```make",
        "EPICURUS_MODULES := serial tick",
        "```",
        "",
    ]
    return "\n".join(out) + "\n"
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `python3 -m unittest discover -s scripts/tests -v`
Expected: PASS, 10 new tests.

- [ ] **Step 5: Commit**

```bash
git add scripts/bundlegen.py scripts/tests/test_bundlegen.py
git commit -m "feat(bundle): emit epicurus-sources.json and SUPPORT.md"
```

---

## Task 4: Emit QUICKSTART.md and MPLABX.md

**Files:**
- Modify: `scripts/bundlegen.py`
- Modify: `scripts/tests/test_bundlegen.py`

**Interfaces:**
- Consumes: Tasks 1 to 3.
- Produces:
  - `emit_quickstart_md(manifest, family_name, version) -> str`
  - `emit_mplabx_md(manifest, family_name, version) -> str`

Both are generated rather than written by hand so their file lists and include paths cannot drift from the manifest. `MPLABX.md` in particular exists because MPLAB X users have no way to consume `epicurus.mk`; the folders and include paths it names come straight from the same resolved data.

- [ ] **Step 1: Write the failing tests**

Append to `scripts/tests/test_bundlegen.py`:

```python
class TestQuickstart(unittest.TestCase):
    def setUp(self):
        self.md = bundlegen.emit_quickstart_md(load(), "PIC16F87XA", "v0.1.0")

    def test_shows_a_complete_consumer_makefile(self):
        self.assertIn("EPICURUS_DIR :=", self.md)
        self.assertIn("EPICURUS_MCU :=", self.md)
        self.assertIn("EPICURUS_MODULES :=", self.md)
        self.assertIn("include $(EPICURUS_DIR)/epicurus.mk", self.md)

    def test_names_a_real_part_from_this_family(self):
        self.assertIn("16F877A", self.md)

    def test_names_a_real_module_from_this_family(self):
        self.assertIn("tick", self.md)

    def test_mentions_the_dfp_flag(self):
        self.assertIn("-mdfp", self.md)

    def test_has_no_em_dash(self):
        self.assertNotIn(chr(0x2014), self.md)  # em-dash, repo convention


class TestMplabxMd(unittest.TestCase):
    def setUp(self):
        self.md = bundlegen.emit_mplabx_md(load(), "PIC16F87XA", "v0.1.0")

    def test_lists_the_source_folders_to_add(self):
        self.assertIn("pic16f87xa-hal/src", self.md)
        self.assertIn("epic-serial/src", self.md)

    def test_lists_the_include_paths_in_order(self):
        self.assertIn("pic16f87xa-hal/include/target", self.md)
        idx_target = self.md.index("pic16f87xa-hal/include/target")
        idx_plain = self.md.index("epic-common/include")
        self.assertLess(idx_target, idx_plain)

    def test_names_the_dfp_pack(self):
        self.assertIn("Microchip.PIC16Fxxx_DFP", self.md)

    def test_points_at_the_reference_project(self):
        self.assertIn("examples/epicurus-demo.X", self.md)

    def test_has_no_em_dash(self):
        self.assertNotIn(chr(0x2014), self.md)  # em-dash, repo convention
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `python3 -m unittest discover -s scripts/tests -v`
Expected: FAIL, `AttributeError: module 'bundlegen' has no attribute 'emit_quickstart_md'`.

- [ ] **Step 3: Implement both emitters**

Append to `scripts/bundlegen.py`:

```python
def _sample_module(manifest, family_name, mcu):
    """A module that actually builds on `mcu`, for use in examples."""
    for name in modules_for_family(manifest, family_name):
        if mcu in manifest.modules[name].supported.get(family_name, []):
            return name.removeprefix("epic-")
    raise BundleError(f"no module supports {mcu}; cannot write a quickstart")


def emit_quickstart_md(manifest, family_name: str, version: str) -> str:
    fam = _family(manifest, family_name)
    mcu = fam.variants[-1]
    module = _sample_module(manifest, family_name, mcu)

    return "\n".join([
        f"# Quick start, Epicurus {version} ({family_name})",
        "",
        "You need MPLAB XC8 (`xc8-cc`) on your PATH and GNU make. No",
        "MPLAB X, no licence, no IDE.",
        "",
        "## 1. Put the bundle in your project",
        "",
        "```sh",
        "mkdir -p third_party",
        f"tar xzf epicurus-{fam.hal_dir.replace('-hal', '')}-{version}.tar.gz \\",
        "  -C third_party",
        "mv third_party/epicurus-* third_party/epicurus",
        "```",
        "",
        "## 2. Write your Makefile",
        "",
        "```make",
        "EPICURUS_DIR := third_party/epicurus",
        f"EPICURUS_MCU := {mcu}",
        f"EPICURUS_MODULES := {module}",
        "include $(EPICURUS_DIR)/epicurus.mk",
        "",
        "DFP := /opt/microchip/xc8/v4.00/pic/packs/$(EPICURUS_DFP)/xc8",
        "",
        "SRCS := main.c $(EPICURUS_SRCS)",
        "CFLAGS := -mdfp=$(DFP) -mcpu=$(shell echo $(EPICURUS_MCU) | tr A-Z a-z) \\",
        "          -O2 -std=c99 -Wall -Wextra $(EPICURUS_CFLAGS) -DFOSC_HZ=20000000",
        "",
        "app.hex: $(SRCS)",
        "\txc8-cc $(CFLAGS) $^ -o $@ -ginhx32",
        "```",
        "",
        "## 3. Build",
        "",
        "```sh",
        "make",
        "```",
        "",
        "Program `app.hex` with MPLAB X, MPLAB IPE, or any programmer.",
        "",
        "## Notes",
        "",
        "- Module names drop the `epic-` prefix in `EPICURUS_MODULES`.",
        "- Dependencies resolve automatically: `modbus` pulls in `serial`",
        "  and `tick`.",
        "- Asking for a module on a part it does not fit fails immediately",
        "  with the reason. See `SUPPORT.md` for the full table.",
        f"- Supported parts in this bundle: {', '.join(fam.variants)}.",
        "- You still supply your own `#pragma config` words. The reference",
        "  project under `examples/` has a working set to copy.",
        "",
    ]) + "\n"


def emit_mplabx_md(manifest, family_name: str, version: str) -> str:
    """Instructions for MPLAB X and the MPLAB extension for VS Code.

    Generated from the same resolved data epicurus.mk uses, because an
    MPLAB X user cannot include a .mk and a hand-written list of folders
    would go stale the first time a source moved.
    """
    fam = _family(manifest, family_name)
    modules = modules_for_family(manifest, family_name)

    src_dirs = sorted({str(pathlib.PurePosixPath(s).parent) for s in fam.hal_sources})
    for name in modules:
        mod = manifest.modules[name]
        src_dirs += sorted({
            f"{mod.dir}/{pathlib.PurePosixPath(s).parent}" for s in mod.sources
        })
    src_dirs = sorted(set(src_dirs))

    out = [
        f"# Using Epicurus {version} from MPLAB X ({family_name})",
        "",
        "Two ways in. The reference project is the fast one.",
        "",
        "## Option 1: open the reference project",
        "",
        "```",
        "examples/epicurus-demo.X",
        "```",
        "",
        "Open it in MPLAB X (File > Open Project), pick your part under",
        "Project Properties, and Build. Confirm it produces a `.hex`, then",
        "either build your application inside it or copy its settings into",
        "your own project using Option 2.",
        "",
        "This also works in the MPLAB extension for VS Code, which opens",
        "the same `.X` project format.",
        "",
        "## Option 2: add Epicurus to an existing project",
        "",
        "### Add the sources",
        "",
        "Right-click `Source Files` > `Add Existing Items from Folders...`,",
        "then add each of these from this bundle:",
        "",
    ]
    out += [f"- `{d}`" for d in src_dirs]
    out += [
        "",
        "Add only the module folders you actually use. The HAL folders are",
        "not optional: the interrupt dispatch takes strong references to",
        "every peripheral handler, so a partial set will not link.",
        "",
        "### Set the include paths",
        "",
        "Project Properties > XC8 Compiler > Include directories, in this",
        "order (the order matters: `include/target` must come first so the",
        "platform header resolves to the real-target version rather than",
        "the host one):",
        "",
    ]
    out += [f"{i + 1}. `{inc}`" for i, inc in enumerate(fam.includes)]
    for name in modules:
        mod = manifest.modules[name]
        for inc in mod.includes:
            out.append(f"   plus `{mod.dir}/{inc}` if you use `{name}`")
    out += [
        "",
        "### Set the device pack",
        "",
        f"This family needs the `{fam.dfp}` pack. Install it through MPLAB",
        "X's Tools > Packs manager if the part does not appear in the",
        "device list.",
        "",
        "### Define the part macro",
        "",
        "Project Properties > XC8 Compiler > Preprocessor macros, add",
        "`PIC<part>`, for example `PIC" + fam.variants[-1] + "`.",
        "",
        "## Which parts work",
        "",
        "See `SUPPORT.md`. MPLAB X will not warn you about an unsupported",
        "combination the way `epicurus.mk` does; it will fail at link time",
        "with an XC8 memory error instead.",
        "",
    ]
    return "\n".join(out) + "\n"
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `python3 -m unittest discover -s scripts/tests -v`
Expected: PASS, 10 new tests.

- [ ] **Step 5: Commit**

```bash
git add scripts/bundlegen.py scripts/tests/test_bundlegen.py
git commit -m "feat(bundle): emit QUICKSTART.md and MPLABX.md"
```

---

## Task 5: The bundle CLI

**Files:**
- Create: `scripts/make_bundle.py`
- Modify: `.gitignore`

**Interfaces:**
- Consumes: every emitter from Tasks 1 to 4.
- Produces: `python3 scripts/make_bundle.py --family PIC16F87XA --version v0.1.0 [--out-dir bundles]` writing `bundles/epicurus-<slug>-<version>/` and a `.tar.gz` beside it.

The copy is a directory walk, not a file-by-file copy of `files_for_family`: headers, `README.md`, `docs/`, and `MANUAL.md` all belong in a bundle, and enumerating them in the manifest would be busywork. `files_for_family` stays the authority for what `epicurus.mk` names.

- [ ] **Step 1: Write the CLI**

Create `scripts/make_bundle.py`:

```python
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
SKIP_DIRS = {"build", "build18", "mcu", "__pycache__", ".git", "third_party"}


def _slug(family_name: str, manifest) -> str:
    return manifest.families[family_name].hal_dir.removesuffix("-hal")


def _copy_tree(src: pathlib.Path, dst: pathlib.Path) -> None:
    """Copy a module or HAL directory, minus build output and mcu/."""
    for path in sorted(src.rglob("*")):
        if any(part in SKIP_DIRS for part in path.relative_to(src).parts):
            continue
        if path.is_dir():
            continue
        if path.suffix not in {".c", ".h", ".md", ".txt"} and path.name not in DOC_NAMES:
            continue
        target = dst / path.relative_to(src)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)


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
        bundlegen.emit_quickstart_md(manifest, args.family, args.version))
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
        tarball = root.with_suffix(".tar.gz")
        with tarfile.open(tarball, "w:gz") as tf:
            tf.add(root, arcname=root.name)
        print(f"tarball: {tarball.relative_to(REPO)}")


if __name__ == "__main__":
    main()
```

- [ ] **Step 2: Ignore build output**

Add to `.gitignore`:

```gitignore
# Release bundles are build output, never committed.
bundles/
```

- [ ] **Step 3: Generate all three bundles**

```bash
for fam in PIC16F87XA PIC18Fxx5x PIC16F193X; do
  python3 scripts/make_bundle.py --family "$fam" --version v0.1.0
done
```

Expected: three `bundle:` lines and three `tarball:` lines. The PIC16F193X bundle will report `0 modules`, which is correct for now: its higher-level modules are not wired up yet, so it is a HAL-only bundle. If the missing-files check fires, the manifest names a source the copy filter dropped; widen `_copy_tree`'s suffix set rather than loosening the check.

- [ ] **Step 4: Inspect a bundle by hand**

```bash
ls bundles/epicurus-pic16f87xa-v0.1.0/
cat bundles/epicurus-pic16f87xa-v0.1.0/SUPPORT.md
head -40 bundles/epicurus-pic16f87xa-v0.1.0/epicurus.mk
grep -rn "\.\./" bundles/epicurus-pic16f87xa-v0.1.0/epicurus.mk || echo "OK: no escaping paths"
```

Expected: the layout from the spec, a support table matching the manifest, and `OK: no escaping paths`.

- [ ] **Step 5: Commit**

```bash
git add scripts/make_bundle.py .gitignore
git commit -m "feat(bundle): add the bundle assembly CLI"
```

---

## Task 6: The isolation gate

**Files:**
- Create: `.github/workflows/bundle-gate.yml`

**Interfaces:**
- Consumes: `make_bundle.py` from Task 5.
- Produces: CI proof that each bundle builds from a scratch directory outside the repo.

**This gate is the point of the whole plan.** Building a bundle in place would let a missing file quietly resolve back through the repo's sibling layout, which is exactly the failure packaging introduces. The gate copies each bundle somewhere with no repo above it and builds there.

- [ ] **Step 1: Write the workflow**

Create `.github/workflows/bundle-gate.yml`:

```yaml
name: bundle-gate

# Proves each generated bundle is genuinely self-contained: it is copied
# to a scratch directory OUTSIDE the repo checkout and built there. In
# place, a file missing from the bundle could still resolve through the
# repo's own sibling layout and the build would pass while the bundle
# was broken for every real consumer.
#
# Bundles are generated on the runner (python3) and built in the
# toolchain container (xc8-cc, no python3).

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

  generate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Generate every bundle
        run: |
          python3 - <<'PY' > families.txt
          import sys
          sys.path.insert(0, "scripts")
          import epicmanifest
          m = epicmanifest.load(epicmanifest.default_path())
          for name in sorted(m.families):
              print(name)
          PY
          while read -r fam; do
            python3 scripts/make_bundle.py --family "$fam" --version ci-test
          done < families.txt
          ls -la bundles/
      - uses: actions/upload-artifact@v4
        with:
          name: bundles
          path: bundles/

  build-isolated:
    needs: [toolchain-image, generate]
    runs-on: ubuntu-latest
    container:
      image: ${{ needs.toolchain-image.outputs.image }}
      credentials:
        username: ${{ github.actor }}
        password: ${{ secrets.GITHUB_TOKEN }}
    steps:
      - uses: actions/download-artifact@v4
        with:
          name: bundles
          path: /tmp/bundles
      - name: Build each bundle from outside any repo checkout
        shell: bash
        run: |
          # /isolated deliberately has no repo above it: a path escaping
          # the bundle cannot accidentally resolve.
          mkdir -p /isolated && cd /isolated
          fail=0
          {
            echo "| Bundle | Module | Part | Result |"
            echo "|---|---|---|---|"
          } >> "$GITHUB_STEP_SUMMARY"

          for tarball in /tmp/bundles/*.tar.gz; do
            name="$(basename "$tarball" .tar.gz)"
            rm -rf "/isolated/$name"
            tar xzf "$tarball" -C /isolated
            cd "/isolated/$name"

            # Pick the first supported (module, part) pair straight out
            # of the bundle's own SUPPORT.md table, so this tests what a
            # consumer would actually be told to do.
            module=""
            part=""
            while IFS= read -r row; do
              case "$row" in
                "| \`epic-"*)
                  m="$(echo "$row" | sed 's/^| `\([^`]*\)`.*/\1/' | sed 's/^epic-//')"
                  col=0
                  IFS='|' read -ra cells <<< "$row"
                  for cell in "${cells[@]:2}"; do
                    trimmed="$(echo "$cell" | tr -d ' ')"
                    if [ "$trimmed" = "yes" ]; then
                      part="$(sed -n 's/^| Module | //p' SUPPORT.md \
                        | head -1 | tr -d ' ' | cut -d'|' -f$((col + 1)))"
                      module="$m"
                      break
                    fi
                    col=$((col + 1))
                  done
                  ;;
              esac
              [ -n "$module" ] && break
            done < SUPPORT.md

            if [ -z "$module" ]; then
              echo "| $name | (none) | (none) | SKIP: HAL-only bundle |" \
                >> "$GITHUB_STEP_SUMMARY"
              cd /isolated
              continue
            fi

            dfp_name="$(grep -m1 '^EPICURUS_DFP' epicurus.mk | awk '{print $3}')"
            cat > Makefile <<EOF
          EPICURUS_DIR := .
          EPICURUS_MCU := $part
          EPICURUS_MODULES := $module
          include \$(EPICURUS_DIR)/epicurus.mk
          DFP := $XC8_INSTALL_DIR/pic/packs/$dfp_name/xc8
          all:
          	xc8-cc -mdfp=\$(DFP) -mcpu=\$(shell echo \$(EPICURUS_MCU) | tr A-Z a-z) \\
          	  -O2 -std=c99 -Wall -Wextra \$(EPICURUS_CFLAGS) -DFOSC_HZ=20000000 \\
          	  \$(EPICURUS_SRCS) main.c -o app.hex -ginhx32
          EOF
            printf 'void main(void) { for (;;) { } }\n' > main.c

            if make >build.log 2>&1 && [ -f app.hex ]; then
              echo "| $name | $module | $part | PASS |" >> "$GITHUB_STEP_SUMMARY"
            else
              echo "| $name | $module | $part | FAIL |" >> "$GITHUB_STEP_SUMMARY"
              echo "::group::$name build log"; cat build.log; echo "::endgroup::"
              fail=1
            fi
            cd /isolated
          done
          exit "$fail"
```

- [ ] **Step 2: Commit and push**

```bash
git add .github/workflows/bundle-gate.yml
git commit -m "test(bundle): gate bundles on building outside the repo"
git push
```

- [ ] **Step 3: Read the gate result and fix what it finds**

Expected first-run failures and their causes:

- **`No such file or directory` on a header.** The bundle is missing headers because `_copy_tree`'s suffix filter dropped them. Widen it.
- **`undefined symbol "_TIMER1_IRQHandler"`.** `EPICURUS_HAL_SRCS` is not the family's full peripheral set. Compare it against the manifest's `hal_sources`.
- **`epicurus.mk:NN: *** ... is not supported`.** The part chosen from `SUPPORT.md` disagrees with `epicurus.mk`'s table. Both come from the manifest, so this means one of the two emitters is reading `supported` wrongly.
- **A `..` in a path.** A source escaped the bundle. Fix `_copy_tree` or the manifest path convention.

Iterate until every non-HAL-only bundle reports PASS.

- [ ] **Step 4: Confirm the summary**

Expected: a table with one PASS row per bundle that has modules, and one `SKIP: HAL-only bundle` row for PIC16F193X.

---

## Task 7: Documentation

**Files:**
- Modify: `README.md`, `scripts/README.md`, `docs/superpowers/specs/2026-08-05-distribution-design.md`

**Interfaces:**
- Consumes: everything above.
- Produces: docs matching the build.

- [ ] **Step 1: Add a consuming section to README.md**

Insert after the "Quick start" section:

````markdown
### Using Epicurus in your own project

Grab the bundle for your family from
[Releases](https://github.com/apojomovsky/epicurus/releases), unpack it,
and point one variable at it:

```make
EPICURUS_DIR := third_party/epicurus
EPICURUS_MCU := 16F877A
EPICURUS_MODULES := serial tick
include $(EPICURUS_DIR)/epicurus.mk

SRCS := main.c $(EPICURUS_SRCS)
CFLAGS += $(EPICURUS_CFLAGS)
```

Dependencies resolve automatically, and asking for a module on a part it
does not fit fails immediately with the reason rather than as a wall of
XC8 linker errors. Each bundle carries its own `QUICKSTART.md`,
`SUPPORT.md`, and `MPLABX.md`, plus a reference MPLAB X project under
`examples/`.

MPLAB X and the MPLAB extension for VS Code are supported too: open
`examples/epicurus-demo.X`, or follow `MPLABX.md` to add Epicurus to an
existing project.
````

- [ ] **Step 2: Document the generator in scripts/README.md**

```markdown
## `make_bundle.py`, the release bundle generator

Assembles a self-contained, per-family source tree from the manifest,
plus the generated consumer files (`epicurus.mk`,
`epicurus-sources.json`, `SUPPORT.md`, `QUICKSTART.md`, `MPLABX.md`).

```sh
python3 scripts/make_bundle.py --family PIC16F87XA --version v0.1.0
```

Output lands in `bundles/`, which is gitignored: bundles are build
output, attached to a GitHub Release, never committed.

`bundlegen.py` holds the generation logic and is where every emitted
file's format lives. `.github/workflows/bundle-gate.yml` proves a bundle
is self-contained by building it from a scratch directory outside the
repo.
```

- [ ] **Step 3: Mark the spec's phase 3 done**

In `docs/superpowers/specs/2026-08-05-distribution-design.md`, update the `Status:` line to record that phases 1 to 3 are implemented, and note in the Sequencing section that phase 3's exit criterion is met.

- [ ] **Step 4: Verify**

```bash
python3 -m unittest discover -s scripts/tests -v
grep -rnP '\x{2014}' README.md scripts/README.md && echo "FAIL: em-dash" || echo "OK"
```

Expected: all tests pass, `OK`.

- [ ] **Step 5: Commit**

```bash
git add README.md scripts/README.md docs/superpowers/specs/
git commit -m "docs(bundle): document consuming Epicurus from a release bundle"
git push
```

---

## Done when

- `python3 scripts/make_bundle.py --family <F> --version <V>` produces a bundle and tarball for all three families.
- Each bundle contains `epicurus.mk`, `epicurus-sources.json`, `SUPPORT.md`, `QUICKSTART.md`, `MPLABX.md`, `VERSION`, and `LICENSE`.
- No path in any bundle escapes the bundle.
- `bundle-gate` is green: every bundle with modules builds from `/isolated`, outside any repo checkout.
- `python3 -m unittest discover -s scripts/tests` passes.
- `bundles/` is gitignored and nothing generated is committed.

## What this plan deliberately does not do

- Create the reference `.X` projects. `MPLABX.md` already points at `examples/epicurus-demo.X`; that directory arrives in the next plan, which is why `bundle-gate` does not check for it.
- Publish anything. No tags, no GitHub Release, no `release-bundles.yml`.
- Fix any excluded `(module, MCU)` pair.
- Wire the higher-level modules to PIC16F193X, which is why its bundle is HAL-only.
