#!/usr/bin/env python3
"""Statics audit: flag file-scope mutable statics that are IRQ-shared and
neither single-byte nor __at-pinned (the banked families' best-fit GPR
risk set; PIC16F87XA/PIC16F193X only). IRQ-shared = referenced from an
interrupt-context function (dispatch/ISR names or HAL callback slots) and
elsewhere; multi-byte = arrays, HAL handle typedefs, or >1-byte types.
Exit 1 on unpinned IRQ-shared multi-byte statics. Runs in CI and `make audit`.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
import epicmanifest as manifest_lib  # noqa: E402

BANKED_FAMILIES = ("PIC16F87XA", "PIC16F193X")

HANDLE_TYPEDEF_RE = re.compile(r"HandleTypeDef|struct\b|\bunion\b")
# Placement pins: XC8's raw __at or the platform-header EPIC_PLACE
# wrapper (the host build compiles the same source with EPIC_PLACE a
# no-op, so only the real-target spelling is a pin).
PIN_RE = re.compile(r"(?:__at|EPIC_PLACE)\s*\(\s*(0x[0-9A-Fa-f]+|\d+)")
IRQ_NAME_RE = re.compile(r"(IRQHandler|_isr\b|Isr|_IRQ|_irq)")
# HAL callback-slot assignments: h.XxxCallback = fn or .EventCallback = fn
CALLBACK_ASSIGN_RE = re.compile(r"(?:\.\w*[Cc]allback|EventCallback)\s*=\s*"
                                r"([a-zA-Z_]\w*)")
SIZE_HINTS = {  # type token -> bytes
    "uint8_t": 1, "int8_t": 1, "bool": 1, "char": 1, "uint8": 1, "int8": 1,
    "uint16_t": 2, "int16_t": 2, "uint16": 2, "int16": 2,
    "uint32_t": 4, "int32_t": 4, "uint32": 4, "int32": 4, "float": 4,
    "double": 8,
}

# Explicitly verified exceptions: (file, static) pairs that are
# IRQ-shared and multi-byte but deliberately unpinned, with the reason.
# Each is verified by disassembly or the documented codegen mechanism
# (the reasons are itemized below):
# - The 193X objects: XC8 v4.00 derefs pointers through FSR1 indirect
#   (`movwf fsr1l; clrf fsr1h; moviw`), which reaches any bank, and
#   direct array reads get auto-banksel. Verified 2026-08-11 by
#   disassembly of the 16F1937 timer0 ISR; matches the documented
#   Finding 1 (runtime addresses compile to FSR1:INDF1).
# - The 87XA g_ccp_callbacks: the CCP ISRs read the array DIRECTLY
#   (auto-banksel), never through a pointer; the PR #19 disassembly
#   shows the direct read + null check + functab call, no IRP/FSR.
# - The module objects (epic-serial's ring buffers, epic-tick's tick
#   counter): the ISR path touches them by direct symbol access
#   (auto-banksel), never through a pointer, so any bank works; the
#   193X FSR1 mechanism covers the pointer derefs. Same verified
#   mechanism as the CCP callbacks.
ALLOWLIST = {
    ("pic16f193x-hal/src/peripherals/pic16f193x_ccp.c", "g_handle"):
        "193X FSR1-indirect derefs, bank-agnostic (verified)",
    ("pic16f193x-hal/src/peripherals/pic16f193x_timer0.c", "g_t0_storage"):
        "193X FSR1-indirect derefs, bank-agnostic (verified)",
    ("pic16f193x-hal/src/peripherals/pic16f193x_timer246.c", "g_handle"):
        "193X FSR1-indirect derefs, bank-agnostic (verified)",
    ("pic16f87xa-hal/src/peripherals/pic16f87xa_ccp.c", "g_ccp_callbacks"):
        "87XA CCP ISR reads the array directly, auto-banksel (verified)",
    ("epic-serial/src/epic_serial.c", "g_tx_buf"):
        "direct symbol access, auto-banksel (verified)",
    ("epic-tick/src/epic_tick.c", "g_tick_ms"):
        "direct symbol access, auto-banksel (verified)",
}


def module_sources(m, module: str, family: str) -> list[str]:
    """Repo-root-relative sources one build compiles for (module, family).

    Module sources are stored module-relative (src/foo.c); the HAL
    family sources are already repo-root-relative.
    """
    out = []
    for dep in m.resolve_deps(module):
        mod = m.modules[dep]
        out += [f"{mod.dir}/{s}" for s in mod.sources]
        out += [f"{mod.dir}/{s}"
                for s in mod.sources_by_family.get(family, [])]
    return list(dict.fromkeys(out))


def brace_depths(text: str) -> list[int]:
    """Cumulative brace depth before each line."""
    depths = []
    depth = 0
    for line in text.splitlines():
        depths.append(depth)
        depth += line.count("{") - line.count("}")
    return depths


def split_top_commas(s: str) -> list[str]:
    depth = 0
    parts, cur = [], ""
    for ch in s:
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        if ch == "," and depth == 0:
            parts.append(cur)
            cur = ""
        else:
            cur += ch
    parts.append(cur)
    return parts


def parse_static_decl(line: str):
    """(names, type_str, has_array, pinned) or None for a static decl.

    Handles plain, array, function-pointer, and multi-declarator
    forms; the initializer is stripped before declarator splitting.
    """
    s = line.strip()
    if not s.startswith("static "):
        return None
    s = re.sub(r"^static\s+", "", s)
    # The __at pin suffix is placement syntax, not part of the
    # declarator; strip it so the object name is still extracted.
    s = re.sub(r"__at\s*\([^)]*\)", "", s)
    # A const object is immutable and out of scope; only a `const T *`
    # pointer stays in scope (the pointer itself is written, e.g.
    # `g_t0_handle = &storage`). Const arrays and const scalars are
    # read-only and skipped.
    if s.startswith("const ") and "*" not in s:
        return None
    if s.startswith(("typedef ", "volatile void", "void ")) \
            and not re.search(r"\(\s*\*", s):
        # functions (and typedefs) are not objects; a leading 'void'
        # with a parenthesized '*name' declarator is a function-pointer
        # variable and is kept.
        if not re.search(r"\(\s*\*", s):
            return None
    depth = 0
    eq = None
    for i, ch in enumerate(s):
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        elif ch == "=" and depth == 0:
            eq = i
            break
    if eq is not None:
        s = s[:eq]
    s = s.rstrip().rstrip(";").rstrip()
    if not s:
        return None
    parts = split_top_commas(s)
    # A leading 'void' or return type with a parenthesized declarator
    # is a function-pointer variable; a bare 'void name(...)' is a
    # function declaration and is excluded.
    if "(" in s and not re.search(r"\(\s*\*", s):
        return None
    names, type_words = [], []
    for part in parts:
        pm = re.search(r"\(\s*\*\s*([a-zA-Z_]\w*)\s*(?:\[\s*\d*\s*\])?\s*\)",
                       part)
        if pm:
            names.append(pm.group(1))
            continue
        # The declarator name is the last identifier before any
        # array-size expression: g_tx_buf[SZ] names g_tx_buf, not SZ.
        part_no_array = re.sub(r"\[[^\]]*\]", "", part)
        ids = re.findall(r"[a-zA-Z_]\w*", part_no_array)
        if ids:
            names.append(ids[-1])
    # Type = the declaration minus the first declarator's name.
    first = parts[0]
    first_name = names[0] if names else ""
    if first_name:
        type_str = first.replace(first_name, "", 1)
    else:
        type_str = first
    has_array = bool(re.search(r"\[\s*[^\]]+\]", s))
    pinned = bool(PIN_RE.search(line))
    return names, type_str.strip(), has_array, pinned


def is_multi(type_str: str, has_array: bool) -> bool:
    if has_array:
        return True  # even an array of pointers is multi-byte
    if "*" in type_str:
        return False  # a single pointer is 1 byte on PIC16
    if HANDLE_TYPEDEF_RE.search(type_str):
        return True
    words = [w for w in type_str.split() if w]
    if not words:
        return False
    return SIZE_HINTS.get(words[-1], 1) > 1


def split_funcs(text: str):
    """(name, start_line, end_line) tuples by a crude brace balance."""
    funcs = []
    depth = 0
    cur_name = None
    cur_start = 0
    opened = False
    for i, line in enumerate(text.splitlines()):
        if depth == 0 and not opened and "(" in line and ")" in line \
                and "{" not in line.split("(")[0]:
            name_m = re.match(r"[\w\s\*]*?\b([a-zA-Z_]\w*)\s*\(", line)
            if name_m:
                cur_name = name_m.group(1)
                cur_start = i
        if "{" in line and cur_name and depth == 0:
            opened = True
        depth += line.count("{") - line.count("}")
        if depth == 0 and opened:
            funcs.append((cur_name, cur_start, i))
            cur_name = None
            opened = False
    return funcs


def scan_file(path: pathlib.Path):
    """[(names, type_str, has_array, pinned, line_no, decl_line, shared)]"""
    text = path.read_text(errors="replace")
    depths = brace_depths(text)
    funcs = split_funcs(text)
    irq_text = "\n".join(
        "\n".join(text.splitlines()[s:e + 1]) for n, s, e in funcs
        if IRQ_NAME_RE.search(n))
    callbacks = set(CALLBACK_ASSIGN_RE.findall(text))
    callback_text = "\n".join(
        "\n".join(text.splitlines()[s:e + 1]) for n, s, e in funcs
        if n in callbacks)
    main_text = "\n".join(
        "\n".join(text.splitlines()[s:e + 1]) for n, s, e in funcs
        if not IRQ_NAME_RE.search(n) and n not in callbacks)
    irq_names = set(re.findall(r"\b([a-zA-Z_]\w*)\b", irq_text)) | \
        set(re.findall(r"\b([a-zA-Z_]\w*)\b", callback_text))
    main_names = set(re.findall(r"\b([a-zA-Z_]\w*)\b", main_text))

    out = []
    for i, line in enumerate(text.splitlines(), 1):
        if depths[i - 1] != 0:
            continue
        parsed = parse_static_decl(line)
        if parsed is None:
            continue
        names, type_str, has_array, pinned = parsed
        shared = any(n in irq_names and n in main_names for n in names)
        multi = is_multi(type_str, has_array)
        out.append((names, type_str, has_array, pinned, i, line.strip(),
                    shared, multi))
    # Pointer-pointee propagation: a multi-byte storage object whose
    # name never appears in the ISR is still read from ISR context
    # through a same-file pointer (e.g. the ISR derefs `g_t0_handle`
    # to reach `g_t0_storage`). If any IRQ-shared pointer exists in
    # the file, every multi-byte static in it is treated as shared;
    # conservative, matching section H's worst-case list.
    shared_ptrs = {names[0] for names, t, a, p, ln, d, s, m in out
                   if s and "*" in t}
    if shared_ptrs:
        out = [(names, t, a, p, ln, d, True if m else s, m)
               for names, t, a, p, ln, d, s, m in out]
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--list", action="store_true",
                    help="list every file-scope mutable static")
    args = ap.parse_args()

    m = manifest_lib.load(manifest_lib.default_path())
    files: dict[pathlib.Path, str] = {}  # path -> label
    for fam_name in BANKED_FAMILIES:
        fam = m.families[fam_name]
        for src in fam.hal_sources:
            p = REPO / src
            if p.exists() and p.suffix == ".c":
                files[p] = fam.hal_dir
        # The HARNESS=sim variant swaps the family harness source for
        # the sim-target harness (the mdb gates build these); its
        # statics are in the same banked GPR as the rest.
        for p in sorted((REPO / fam.hal_dir / "src").glob("core/*harness_sim_target.c")):
            files[p] = fam.hal_dir
    for module in sorted(m.modules):
        fam = next((f for f in BANKED_FAMILIES
                    if f in m.modules[module].supported), None)
        if fam is None:
            continue
        for src in module_sources(m, module, fam):
            p = REPO / src
            if p.exists() and p.suffix == ".c" and p not in files:
                files[p] = module

    bad = 0
    for path, label in sorted(files.items()):
        for names, type_str, has_array, pinned, line_no, decl, shared, multi \
                in scan_file(path):
            if not (shared and multi):
                continue
            key = (str(path.relative_to(REPO)), names[0])
            if pinned or key in ALLOWLIST:
                continue
            print(f"unpinned IRQ-shared multi-byte static: {label}: "
                  f"{path.relative_to(REPO)}:{line_no}: {decl}")
            bad = 1
        if args.list:
            for names, type_str, has_array, pinned, line_no, decl, shared, \
                    multi in scan_file(path):
                mark = "I" if shared else " "
                print(f"{mark}{'M' if multi else '1'} {label}: "
                      f"{path.relative_to(REPO)}:{line_no}: "
                      f"{'pin ' if pinned else '    '}{decl}")

    if bad:
        print("statics audit: unpinned IRQ-shared multi-byte statics found")
        return 1
    print("statics audit: all IRQ-shared multi-byte statics are pinned")
    return 0


if __name__ == "__main__":
    sys.exit(main())
