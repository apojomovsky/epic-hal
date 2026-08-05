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
    output is parsed. Modules with no CONFIG_SRC get no config table.

Schema (per Amendments 1 and 2): families carry a required fosc_hz;
modules carry needs_hal, sources, sources_by_family, depends_on, and
per-family example tables. needs_hal is set false when a module's
extracted SRCS shares no path with its family's HAL set (reviewed by
hand: epic-math is the exception, its library needs no HAL but its
example does, so it is hand-corrected to needs_hal=false with hal=true
on its examples). sources_by_family holds paths under a src/pic16/ or
src/pic18/ backend directory; the family-blind sources list holds the
rest. depends_on is inferred from cross-module sources in SRCS and
verified against the plan's table.

Usage:  python3 scripts/bootstrap_manifest.py > epic-common/manifest/modules.toml
"""
from __future__ import annotations

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
    """Ask GNU make for one resolved variable's value (empty if undefined)."""
    proc = subprocess.run(
        ["make", "--no-print-directory", "-C", str(directory), "-f", "Makefile",
         "-f", "-", f"MCU={mcu}", f"print-{var}"],
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
    """Run the real config-word recipe and parse what it emitted.

    Returns {} when the Makefile defines no CONFIG_SRC (no config words);
    the build driver then emits no config translation unit for that
    example, matching the Makefile, which never compiled one.

    The recipe only runs `mkdir` and `printf`, never a compiler, so it
    runs anywhere. It is generated into a private temp directory (BUILD_DIR
    override) rather than the module's own build/, which may be a
    root-owned leftover from a prior container run that this user cannot
    write to.
    """
    if not make_var(directory, mcu, "CONFIG_SRC"):
        return {}
    import tempfile
    tmp = pathlib.Path(tempfile.mkdtemp(prefix="epic-bootstrap-"))
    try:
        target = str(tmp / f"config_{mcu}.c")
        proc = subprocess.run(
            ["make", "--no-print-directory", "-C", str(directory),
             f"MCU={mcu}", f"BUILD_DIR={tmp}", target],
            capture_output=True, text=True,
        )
        generated = tmp / f"config_{mcu}.c"
        if proc.returncode != 0 or not generated.exists():
            sys.exit(f"could not generate config_{mcu}.c in {directory}:\n{proc.stderr}")
        pragmas = {}
        for line in generated.read_text().splitlines():
            if "#pragma config" not in line:
                continue
            rest = line.split("#pragma config", 1)[1]
            # A config line may carry several KEY = VALUE pairs separated
            # by commas (PIC18: "CP0 = OFF, CP1 = OFF, CP2 = OFF"). Split
            # and parse each so none is lost and no value keeps a comma.
            for pair in rest.split(","):
                m = re.match(r"\s*(\w+)\s*=\s*(\S+)\s*$", pair)
                if m:
                    pragmas[m.group(1)] = m.group(2)
        return pragmas
    finally:
        import shutil
        shutil.rmtree(tmp, ignore_errors=True)


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


def toml_inline_table(pairs: list[tuple[str, str]]) -> str:
    if not pairs:
        return "{}"
    return "{ " + ", ".join(f"{k} = {toml_str(v)}" for k, v in pairs) + " }"


def example_name_from_target(directory: pathlib.Path, mcu: str) -> str:
    """The example name: the suffix of the Makefile's TARGET after <MCU>-.

    The emitted .hex is build/<MCU>-<name>.hex, so this name makes the new
    path's output comparable to the Makefile's, like for like.
    """
    target = make_var(directory, mcu, "TARGET")
    # TARGET resolves to build/<MCU>-<name>; strip directory and MCU prefix.
    base = target.rsplit("/", 1)[-1]
    prefix = mcu + "-"
    if base.startswith(prefix):
        return base[len(prefix):]
    return base


def split_module_sources(repo_rel_paths: list[str], module_name: str):
    """Split a module's own SRCS into family-blind sources and per-family.

    Paths under src/pic16/ or src/pic18/ are per-family backends and go to
    sources_by_family; everything else under the module's own src/ stays in
    the family-blind `sources` list. Returns (sources, sources_by_family)
    where the keys of sources_by_family are the family *directory* tokens
    (pic16f87xa / pic18fxx5x), translated to family names by the caller.
    """
    sources: list[str] = []
    by_backend: dict[str, list[str]] = {}
    for s in repo_rel_paths:
        rel = s.split("/", 1)[1]  # drop the "<module>/" prefix
        if re.match(r"src/pic16/", rel):
            by_backend.setdefault("PIC16F87XA", []).append(rel)
        elif re.match(r"src/pic18/", rel):
            by_backend.setdefault("PIC18Fxx5x", []).append(rel)
        else:
            sources.append(rel)
    return sources, by_backend


def main():
    hal_dirs, module_dirs = [], []
    for d in mcu_dirs():
        (hal_dirs if d.parent.parent.name.endswith("-hal") else module_dirs).append(d)

    out = []
    out.append("# GENERATED by scripts/bootstrap_manifest.py from the")
    out.append("# mcu/*-mplabx/Makefiles it replaces. Reviewed by hand")
    out.append("# afterwards; edit this file directly from now on.")
    out.append("")

    # ---- Families ------------------------------------------------------
    # The HAL source set is what that family's own HAL Makefile compiles.
    # A source only some variants compile becomes a conditional. fosc_hz
    # is the Makefile's own FOSC_HZ default, family-uniform.
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
        fosc_hz = make_var(hal_dir, fam["variants"][0], "FOSC_HZ")

        fam_name = fam["name"]
        out.append(f"[families.{fam_name}]")
        out.append("hal_dir  = " + toml_str(fam["hal_dir"]))
        out.append("variants = " + toml_list(fam["variants"]))
        out.append("dfp      = " + toml_str(fam["dfp"]))
        out.append(f"fosc_hz  = {fosc_hz}")
        out.append(f"includes = {toml_list(includes)}")
        out.append(f"hal_sources = {toml_list(common)}")
        out.append("")
        for path, variants in sorted(conditional.items()):
            out.append(f"[[families.{fam_name}.conditional_sources]]")
            out.append(f"path     = {toml_str(path)}")
            out.append(f"variants = {toml_list(sorted(variants))}")
            out.append("")

    # ---- Modules -------------------------------------------------------
    by_module: dict[str, list[pathlib.Path]] = {}
    for d in module_dirs:
        by_module.setdefault(d.parent.parent.name, []).append(d)

    for module_name in sorted(by_module):
        module_dir = REPO / module_name
        supported: dict[str, list[str]] = {}
        sources_all: list[str] = []
        sources_by_family_all: dict[str, list[str]] = {}
        includes_all: list[str] = []
        depends: set[str] = set()
        examples: dict[str, dict] = {}
        compiles_hal = False

        for d in sorted(by_module[module_name]):
            key = family_of_dir(d)
            fam = FAMILIES[key]
            hal_dir = REPO / fam["hal_dir"] / "mcu" / f'{key}-mplabx'
            first = fam["variants"][0]

            hal_set = set()
            for mcu in fam["variants"]:
                hal_set |= {normalise(hal_dir, p)
                            for p in make_var(hal_dir, mcu, "EPIC_SOURCES").split()}

            srcs = [normalise(d, p) for p in make_var(d, first, "SRCS").split()]
            owns = [s for s in srcs if s.startswith(module_name + "/")]
            if any(s in hal_set for s in srcs):
                compiles_hal = True

            # Library sources: owned paths under the module's own src/.
            own_lib = [s for s in owns if s.split("/", 1)[1].startswith("src/")]
            fam_sources, by_backend = split_module_sources(own_lib, module_name)
            sources_all += fam_sources
            for bfam, paths in by_backend.items():
                sources_by_family_all.setdefault(bfam, []).extend(paths)

            # depends_on: any sibling module whose src/ appears in this build.
            for s in srcs:
                m = re.match(r"epic-([a-z0-9]+)/src/", s)
                if m:
                    dep = "epic-" + m.group(1)
                    if dep != module_name:
                        depends.add(dep)

            # includes: owned -I paths from INC.
            for tok in make_var(d, first, "INC").split():
                if tok.startswith("-I"):
                    n = normalise(d, tok[2:])
                    if n.startswith(module_name + "/"):
                        includes_all.append(n.split("/", 1)[1])

            # Example program (Amendment 2 two-step rule):
            #  1. APP_SOURCES, if the Makefile defines it.
            #  2. else any owned SRCS entry that is not under src/ and not
            #     contributed by the HAL or a dependency.
            app = make_var(d, first, "APP_SOURCES")
            if app:
                ex_srcs = [normalise(d, p).split("/", 1)[1] for p in app.split()]
            else:
                # Owned paths not under src/ are the example program. Paths
                # contributed by a dependency start with that dependency's
                # dir, not this module's, so they are already excluded by
                # the `owns` filter.
                ex_srcs = [s.split("/", 1)[1] for s in owns
                           if not s.split("/", 1)[1].startswith("src/")]

            ex_name = example_name_from_target(d, first)
            pragmas = config_pragmas(d, first)
            examples[fam["name"]] = {
                "name": ex_name,
                "sources": ex_srcs,
                "config": pragmas,
            }
            supported[fam["name"]] = list(fam["variants"])

        out.append(f"[modules.{module_name}]")
        out.append(f"dir        = {toml_str(module_name)}")
        out.append(f"sources    = {toml_list(sorted(set(sources_all)))}")
        out.append(f"includes   = {toml_list(sorted(set(includes_all)))}")
        known_modules = set(by_module)
        deps_sorted = sorted(d for d in depends if d in known_modules)
        out.append(f"depends_on = {toml_list(deps_sorted)}")
        # needs_hal: false when no family's build compiles any HAL source.
        # REVIEW: epic-math is hand-corrected to false with hal=true on its
        # examples (Task 3 Step 3); its library touches no HAL, its smoke
        # test does.
        if not compiles_hal:
            out.append("needs_hal = false")
        out.append("")
        if sources_by_family_all:
            out.append(f"[modules.{module_name}.sources_by_family]")
            for fam_name in sorted(sources_by_family_all):
                out.append(f"{fam_name} = {toml_list(sorted(set(sources_by_family_all[fam_name])))}")
            out.append("")
        out.append(f"[modules.{module_name}.supported]")
        for fam_name, variants in sorted(supported.items()):
            out.append(f"{fam_name} = {toml_list(variants)}")
        out.append("")
        for fam_name in sorted(examples):
            ex = examples[fam_name]
            out.append(f"[modules.{module_name}.example.{fam_name}]")
            out.append(f"name    = {toml_str(ex['name'])}")
            out.append("sources = " + toml_list(sorted(set(ex["sources"]))))
            if ex["config"]:
                out.append(f"config  = {toml_inline_table(sorted(ex['config'].items()))}")
            out.append("")

    print("\n".join(out))


if __name__ == "__main__":
    main()
