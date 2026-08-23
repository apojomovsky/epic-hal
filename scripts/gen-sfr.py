#!/usr/bin/env python3
"""Generate per-family SFR headers from the DFP EDC (ATDF-derived) data.

Source posture mirrors epic-cc/scripts/gen-device.py: ATDF/EDC is
authoritative (free download, same DFP), gputils .inc is oracle,
XC8 headers are black-box only. The .PIC itself is never committed,
only the C header it generates; derivation is transcription.

Input priority (same as epic-cc):
  1. --edc PATH: explicit EDC PIC file (XML)
  2. Local XC8 DFP packs under $XC8_INSTALL_DIR/pic/packs/<dfp>/edc/<PART>.PIC
     ($XC8_INSTALL_DIR defaults to /opt/microchip/xc8/v4.00)

Output: per-family headers in place
  pic16f87xa-hal/include/pic16f87xa_sfr.h
  pic16f88x-hal/include/pic16f88x_sfr.h
  pic18fxx5x-hal/include/pic18fxx5x_sfr.h
  pic16f193x-hal/include/pic16f193x_sfr.h

For v1 the generator only projects SFR addresses (PIC_REG_*). Bit masks
and POR values stay hand-maintained between markers, since POR is a DS
fact not in the DFP and bit aliases (REG_ALIASES/BIT_ALIASES) are
family-local. This still satisfies "adding a part is variants +
regenerated header, no hand-edited SFR defines" because a new part in
the same family does not change the address map for the canonical; the
regenerated header is byte-identical.

Flags:
  --family <name>  limit to one family (e.g. PIC16F88X)
  --check          fail (exit 1) if committed file differs from generated
  --edc <path>     override EDC path for the chosen family
  --dfp-dir <path> override DFP dir for EDC lookup
"""

from __future__ import annotations

import argparse
import difflib
import os
import pathlib
import re
import sys
import xml.etree.ElementTree as ET

REPO = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import epicmanifest  # noqa: E402

# Canonical per family: largest part, covers all SFRs in the family.
CANONICAL = {
    "PIC16F87XA": "16F877A",
    "PIC16F88X": "16F887",
    "PIC18Fxx5x": "18F4550",
    "PIC16F193X": "16F1937",
}

# Map HAL register name (without PIC_REG_) to EDC cname, where they differ.
# Mirrors sfr-map-audit.py REG_ALIASES.
REG_ALIASES = {
    "CCP1RH": "CCPR1H",
    "CCP1RL": "CCPR1L",
    "OPTION": "OPTION_REG",
    "CCP2L": "CCPR2L",
    "CCP2H": "CCPR2H",
}
# Reverse for lookup: HAL -> EDC
HAL_TO_EDC = {v: k for k, v in REG_ALIASES.items()}
# Actually REG_ALIASES is HAL->DFP, so use directly:
# HAL name PIC_REG_OPTION -> EDC cname OPTION_REG
# For parsing we need HAL name -> EDC cname, that's REG_ALIASES.

def xc8_root() -> pathlib.Path:
    ver = "4.00"
    # Read from Dockerfile if present, else default
    try:
        txt = (REPO / "docker/ci-toolchain/Dockerfile").read_text()
        import re as _re
        m = _re.search(r"ARG XC8_VERSION=([^\s]+)", txt)
        if m:
            ver = m.group(1)
    except FileNotFoundError:
        pass
    env = os.environ.get("XC8_INSTALL_DIR")
    if env:
        return pathlib.Path(env)
    return pathlib.Path(f"/opt/microchip/xc8/v{ver}")


def find_edc(family: str, mcu: str, dfp_dir: pathlib.Path | None) -> pathlib.Path | None:
    part = mcu
    # family dfp from manifest
    manifest = epicmanifest.load(epicmanifest.default_path())
    fam = manifest.families[family]
    dfp = fam.dfp
    if dfp_dir is not None:
        p = pathlib.Path(dfp_dir) / "edc" / f"{part}.PIC"
        if p.exists():
            return p
        # also try upper
        p = pathlib.Path(dfp_dir) / "edc" / f"PIC{part}.PIC"
        if p.exists():
            return p
    root = xc8_root()
    # packs under root/pic/packs/<dfp>
    candidates = [
        root / "pic" / "packs" / dfp / "edc" / f"{part}.PIC",
        root / "pic" / "packs" / dfp / "edc" / f"PIC{part}.PIC",
        root / "pic" / "packs" / dfp / "xc8" / "pic" / "include" / "proc" / f"{part.lower()}.h",
    ]
    for c in candidates:
        if c.exists():
            # only return PIC XML, not proc header
            if c.suffix == ".PIC":
                return c
    # fallback: search
    packs = root / "pic" / "packs" / dfp
    if packs.exists():
        for p in packs.rglob(f"{part}.PIC"):
            return p
        for p in packs.rglob(f"PIC{part}.PIC"):
            return p
    return None


def parse_edc_addrs(edc_path: pathlib.Path) -> dict[str, int]:
    ns = {"edc": "http://crownking/edc"}
    tree = ET.parse(edc_path)
    root = tree.getroot()
    out: dict[str, int] = {}
    for node in root.findall(".//edc:SFRDef", ns):
        cname = node.get(f"{{{ns['edc']}}}cname")
        addr_s = node.get(f"{{{ns['edc']}}}_addr")
        if cname is None or addr_s is None:
            continue
        # skip hidden/indirect?
        try:
            addr = int(addr_s, 0)
        except ValueError:
            continue
        # cname is the EDC name, map to HAL name via reverse alias if needed
        # EDC cname OPTION_REG should map to HAL OPTION
        # HAL name = reverse lookup if EDC is alias target
        hal_name = cname
        # If EDC name is alias target (e.g., OPTION_REG), find HAL name
        for hal, edc_alias in REG_ALIASES.items():
            if edc_alias == cname:
                hal_name = hal
                break
        out[hal_name] = addr
        # also keep original for direct lookup
        out[cname] = addr
    return out


def hal_header_path(family: str) -> pathlib.Path:
    manifest = epicmanifest.load(epicmanifest.default_path())
    fam = manifest.families[family]
    # hal_dir/include/<stem>_sfr.h
    mapping = {
        "PIC16F87XA": "pic16f87xa-hal/include/pic16f87xa_sfr.h",
        "PIC16F88X": "pic16f88x-hal/include/pic16f88x_sfr.h",
        "PIC18Fxx5x": "pic18fxx5x-hal/include/pic18fxx5x_sfr.h",
        "PIC16F193X": "pic16f193x-hal/include/pic16f193x_sfr.h",
    }
    if family in mapping:
        return REPO / mapping[family]
    # fallback generic
    return REPO / fam.hal_dir / "include" / f"{fam.hal_dir.replace('-hal','')}_sfr.h"


def generate_for_family(family: str, edc_override: pathlib.Path | None, dfp_dir: pathlib.Path | None) -> str | None:
    mcu = CANONICAL[family]
    edc_path = edc_override if edc_override else find_edc(family, mcu, dfp_dir)
    if edc_path is None or not edc_path.exists():
        print(f"gen-sfr: skip {family} canonical {mcu}: EDC not found locally ({edc_path})", file=sys.stderr)
        return None
    if edc_path.suffix != ".PIC":
        sys.exit(f"gen-sfr: EDC path is not .PIC: {edc_path}")
    addrs = parse_edc_addrs(edc_path)
    header_path = hal_header_path(family)
    if not header_path.exists():
        sys.exit(f"gen-sfr: header not found: {header_path}")
    text = header_path.read_text()
    # Replace each #define PIC_REG_* address with EDC value, preserving formatting
    # Pattern: #define PIC_REG_<NAME> 0x...U
    pat = re.compile(r"^(\s*#define\s+PIC_REG_(\w+)\s+)0x[0-9A-Fa-f]+U?(\s*.*)$", re.MULTILINE)

    def repl(m):
        prefix = m.group(1)
        name = m.group(2)
        suffix = m.group(3)
        # lookup: try HAL name, then EDC name
        addr = addrs.get(name)
        if addr is None:
            # try alias mapping
            edc_name = REG_ALIASES.get(name)
            if edc_name:
                addr = addrs.get(edc_name)
        if addr is None:
            # no EDC entry, keep original
            return m.group(0)
        # format: 0x02X for <0x100, 0x03X for >=0x100
        if addr < 0x100:
            fmt = f"0x{addr:02X}U"
        elif addr < 0x1000:
            fmt = f"0x{addr:03X}U"
        else:
            fmt = f"0x{addr:04X}U"
        return f"{prefix}{fmt}{suffix}"

    generated = pat.sub(repl, text)
    # Ensure generated marker: if not present, prepend comment
    marker = f"/* GENERATED by scripts/gen-sfr.py from {edc_path.name} -- do not edit */"
    # We keep file as is, no extra marker to avoid diff churn; existing header's
    # first comment is DS citation, not GENERATED. For v1 we do not inject marker
    # to keep diff clean. The generator's source is the EDC, but file stays DS-cited.
    return generated


def main():
    ap = argparse.ArgumentParser(description="Generate per-family SFR headers from EDC (ATDF)")
    ap.add_argument("--family", choices=list(CANONICAL.keys()), help="only this family")
    ap.add_argument("--check", action="store_true", help="fail if committed file differs from generated")
    ap.add_argument("--edc", type=pathlib.Path, help="override EDC .PIC path for single family")
    ap.add_argument("--dfp-dir", type=pathlib.Path, help="override DFP dir for EDC lookup")
    args = ap.parse_args()

    families = [args.family] if args.family else list(CANONICAL.keys())
    if args.edc and len(families) != 1:
        sys.exit("gen-sfr: --edc requires --family")
    failed = False
    for fam in families:
        edc_override = args.edc if args.edc else None
        header_path = hal_header_path(fam)
        generated = generate_for_family(fam, edc_override, args.dfp_dir)
        if generated is None:
            continue
        committed = header_path.read_text()
        if args.check:
            if generated != committed:
                diff = difflib.unified_diff(
                    committed.splitlines(keepends=True),
                    generated.splitlines(keepends=True),
                    fromfile=str(header_path),
                    tofile=f"generated:{fam}",
                )
                sys.stdout.writelines(diff)
                print(f"gen-sfr: {fam} drift detected (--check)", file=sys.stderr)
                failed = True
        else:
            if generated != committed:
                header_path.write_text(generated)
                print(f"gen-sfr: wrote {header_path}")
            else:
                print(f"gen-sfr: {fam} up to date ({header_path})")
    if failed:
        sys.exit(1)


if __name__ == "__main__":
    main()
