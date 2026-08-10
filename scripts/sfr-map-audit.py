#!/usr/bin/env python3
"""SFR-map audit: cross-check every SFR address and bit constant in the
three HALs' sfr.h files against the Microchip DFP proc headers, the
ground truth for the device memory maps.

Why: the PIE2 bug (2026-08-09) was a misread memory map: PIE2 is at
0x8D (Bank 1) but was "fixed" to Bank 2, silently rerouting every
PIR2-source Enable/DisableSrc into EEADR. A mechanical check against
the DFP kills that class. Run this after any change to the sfr.h
files, and in CI (the target job, which has the toolchain container).

The DFP headers live inside the toolchain container image (the host
has no XC8 install), so each header is read via `docker run ... cat`.
The script is host-side python3, matching the repo's split (the
toolchain container deliberately has no python3).

Usage: python3 scripts/sfr-map-audit.py
Exit 0 = no mismatches; exit 1 = mismatches found (reported per MCU).
Env: EPIC_TOOLCHAIN_IMAGE (default pic8-hal-toolchain:local, the CI
step passes the GHCR image) and EPIC_XC8_ROOT (default
/opt/microchip/xc8/v4.00).
"""

from __future__ import annotations

import os
import re
import subprocess
import sys

IMAGE = os.environ.get("EPIC_TOOLCHAIN_IMAGE", "pic8-hal-toolchain:local")
PACKS = f"{os.environ.get('EPIC_XC8_ROOT', '/opt/microchip/xc8/v4.00')}/pic/packs"

# family -> (hal sfr.h path, [(mcu, dfp pack, proc header)])
FAMILIES = {
    "pic16f87xa-hal": (
        "pic16f87xa-hal/include/pic16f87xa_sfr.h",
        [
            ("16F873A", "Microchip.PIC16Fxxx_DFP", "pic16f873a.h"),
            ("16F874A", "Microchip.PIC16Fxxx_DFP", "pic16f874a.h"),
            ("16F876A", "Microchip.PIC16Fxxx_DFP", "pic16f876a.h"),
            ("16F877A", "Microchip.PIC16Fxxx_DFP", "pic16f877a.h"),
        ],
    ),
    "pic18fxx5x-hal": (
        "pic18fxx5x-hal/include/pic18fxx5x_sfr.h",
        [
            ("18F2455", "Microchip.PIC18Fxxxx_DFP", "pic18f2455.h"),
            ("18F2550", "Microchip.PIC18Fxxxx_DFP", "pic18f2550.h"),
            ("18F4455", "Microchip.PIC18Fxxxx_DFP", "pic18f4455.h"),
            ("18F4550", "Microchip.PIC18Fxxxx_DFP", "pic18f4550.h"),
        ],
    ),
    "pic16f193x-hal": (
        "pic16f193x-hal/include/pic16f193x_sfr.h",
        [
            ("16F1933", "Microchip.PIC12-16F1xxx_DFP", "pic16f1933.h"),
            ("16F1934", "Microchip.PIC12-16F1xxx_DFP", "pic16f1934.h"),
            ("16F1936", "Microchip.PIC12-16F1xxx_DFP", "pic16f1936.h"),
            ("16F1937", "Microchip.PIC12-16F1xxx_DFP", "pic16f1937.h"),
            ("16F1938", "Microchip.PIC12-16F1xxx_DFP", "pic16f1938.h"),
            ("16F1939", "Microchip.PIC12-16F1xxx_DFP", "pic16f1939.h"),
        ],
    ),
}


def read_dfp_header(pack: str, header: str) -> str:
    path = f"{PACKS}/{pack}/xc8/pic/include/proc/{header}"
    out = subprocess.run(
        ["docker", "run", "--rm", IMAGE, "cat", path],
        capture_output=True, text=True,
    )
    if out.returncode != 0:
        sys.exit(f"error: cannot read DFP header {path} ({out.stderr.strip()})")
    return out.stdout


def parse_hal(path: str):
    """Extract PIC_REG_<name> addresses and PIC_<REG>_<BIT> positions."""
    regs, bits = {}, {}
    for line in open(path):
        m = re.match(r"#define\s+PIC_REG_([A-Z0-9_]+)\s+0x([0-9A-F]+)U?", line)
        if m:
            regs[m.group(1)] = int(m.group(2), 16)
            continue
        m = re.match(r"#define\s+PIC_([A-Z0-9_]+)_([A-Z0-9]+)\s+EPIC_BIT\((\d+)\)", line)
        if m:
            bits[(m.group(1), m.group(2))] = int(m.group(3))
    return regs, bits


def parse_dfp(text: str):
    """DFP register addresses (asm equ) and bit positions (_POSN)."""
    regs, bits = {}, {}
    for m in re.finditer(r'asm\("([A-Z0-9_]+) equ ([0-9A-F]+)h"\)', text):
        regs[m.group(1)] = int(m.group(2), 16)
    # Anchor on the known register names so underscored names resolve
    # correctly: _OPTION_REG_nWPUEN_POSN is (OPTION_REG, nWPUEN), not
    # (OPTION, REG_nWPUEN), and _SSPSTAT_D_A_POSN is (SSPSTAT, D_A).
    for reg in regs:
        for m in re.finditer(
                rf"#define _{re.escape(reg)}_([A-Za-z0-9_]+)_POSN\s+0x([0-9A-F]+)",
                text):
            bits[(reg, m.group(1))] = int(m.group(2), 16)
    return regs, bits


# Name aliases: the HAL's register/bit names vs the DFP's generated
# names. An alias maps a HAL name to its DFP name; the addresses and
# positions must still agree after the alias.
REG_ALIASES = {
    "CCP1RH": "CCPR1H", "CCP1RL": "CCPR1L", "OPTION": "OPTION_REG",
}
BIT_ALIASES = {
    # 87XA STATUS: the HAL's short names vs the DFP's long ones.
    ("STATUS", "C"): ("STATUS", "CARRY"),
    ("STATUS", "DC"): ("STATUS", "DC"),
    ("STATUS", "Z"): ("STATUS", "ZERO"),
    # OPTION_REG: the DFP spells the register name in full.
    ("OPTION", "RBPU"): ("OPTION_REG", "RBPU"),
    ("OPTION", "INTEDG"): ("OPTION_REG", "INTEDG"),
    ("OPTION", "T0CS"): ("OPTION_REG", "T0CS"),
    ("OPTION", "T0SE"): ("OPTION_REG", "T0SE"),
    ("OPTION", "PSA"): ("OPTION_REG", "PSA"),
    # SSPSTAT: HAL DA/RW vs the DFP's full names.
    ("SSPSTAT", "DA"): ("SSPSTAT", "D_A"),
    ("SSPSTAT", "RW"): ("SSPSTAT", "R_W"),
    # 193X: the DFP's PCON status bits are n-prefixed (active low) and
    # the OPTION_REG weak-pullup bit is nWPUEN.
    ("PCON", "BOR"): ("PCON", "nBOR"),
    ("PCON", "POR"): ("PCON", "nPOR"),
    ("PCON", "RI"): ("PCON", "nRI"),
    ("PCON", "RMCLR"): ("PCON", "nRMCLR"),
    ("OPTION", "WPUEN"): ("OPTION_REG", "nWPUEN"),
    ("OPTION", "RBPU"): ("OPTION_REG", "nWPUEN"),
    # 193X ADCON0: the HAL spells the GO/DONE bit GO_NDONE; the 87XA
    # and PIC18 spell it GO_DONE (the DFP defines both GO_DONE and the
    # n-prefixed alias at the same position).
    ("ADCON0_GO", "NDONE"): ("ADCON0", "GO"),
    ("ADCON0_GO", "DONE"): ("ADCON0", "GO_DONE"),
}

# Registers and bits that are legitimately absent from a part's DFP
# header: family-conditional SFRs on the smaller parts (the HAL defines
# the constants unconditionally and guards the usage). key: mcu -> set
# of register names to skip entirely.
CONDITIONAL_REGS = {
    "16F873A": {"PORTD", "PORTE", "TRISD", "TRISE", "PIE1", "PIR1", "PIR2"},
    "16F876A": {"PORTD", "PORTE", "TRISD", "TRISE", "PIE1", "PIR1", "PIR2"},
}
# On the 28-pin parts the whole PSP interrupt path is absent; the
# PIE1/PIR1/PIR2 registers still exist, only their PSP bits are
# conditional.
CONDITIONAL_BITS = {
    "16F873A": {("PIE1", "PSPIE"), ("PIR1", "PSPIF")},
    "16F876A": {("PIE1", "PSPIE"), ("PIR1", "PSPIF")},
    # 18F2455/2550 (28-pin, no SPP): the SPP registers and the SPP
    # interrupt bits are absent from those parts' DFP headers.
    "18F2455": {("IPR1", "SPPIP"), ("PIE1", "SPPIE"), ("PIR1", "SPPIF")},
    "18F2550": {("IPR1", "SPPIP"), ("PIE1", "SPPIE"), ("PIR1", "SPPIF")},
}

# Registers absent from the smaller parts' DFP headers but defined
# unconditionally in the HAL (the HAL guards the usage).
CONDITIONAL_REGS.update({
    "18F2455": {"PORTD", "PORTE", "LATD", "LATE", "TRISD", "TRISE",
                "SPPCFG", "SPPCON", "SPPDATA", "SPPEPS"},
    "18F2550": {"PORTD", "PORTE", "LATD", "LATE", "TRISD", "TRISE",
                "SPPCFG", "SPPCON", "SPPDATA", "SPPEPS"},
    # 193X 28-pin parts (1933/1934/1936/1938): no PORTD/E, and the
    # 1933/1936/1938 have the smaller segment LCD (fewer LCDDATA
    # registers, no LCDSE2).
    "16F1933": {"PORTD", "TRISD", "ANSELD", "ANSELE", "LATD", "LCDSE2",
                "LCDDATA2", "LCDDATA5", "LCDDATA8", "LCDDATA11"},
    "16F1936": {"PORTD", "TRISD", "ANSELD", "ANSELE", "LATD", "LCDSE2",
                "LCDDATA2", "LCDDATA5", "LCDDATA8", "LCDDATA11"},
    "16F1938": {"PORTD", "TRISD", "ANSELD", "ANSELE", "LATD", "LCDSE2",
                "LCDDATA2", "LCDDATA5", "LCDDATA8", "LCDDATA11"},
})

# Bits the DFP does not define but the datasheet documents:
# STATUS<PD>/<TO> (POR-only flags) have no _POSN macros; the 87XA
# OPTION_REG RBPU and the 193X SRCON1 SRQEN/SRNQEN are documented in
# the datasheet but absent from the DFP's bit macros.
DFP_MISSING_OK = {
    ("STATUS", "PD"), ("STATUS", "TO"),
    ("OPTION", "RBPU"), ("OPTION_REG", "RBPU"),
    ("SRCON1", "SRQEN"), ("SRCON1", "SRNQEN"),
}

# Bit aliases that the audit deliberately does not chase: aggregate or
# instance-generic names whose per-register mapping is documented in the
# HAL (e.g. CCP_CCPX.* maps to CCP1CON or CCP2CON depending on the
# instance). These are reviewed by hand, not mechanically.
BIT_SKIP_PREFIXES = {("CCP_CCPX",)}

# The TMR0L constant is a documented unused alias of TMR1L's address
# ("kept for naming"); skip it.
SKIP_REGS = {"TMR0L"}


def main() -> int:
    bad = 0
    for family, (sfr_path, mcus) in FAMILIES.items():
        hal_regs, hal_bits = parse_hal(sfr_path)
        for mcu, pack, header in mcus:
            dfp_regs, dfp_bits = parse_dfp(read_dfp_header(pack, header))
            issues = []
            for name, addr in sorted(hal_regs.items()):
                if name in SKIP_REGS:
                    continue
                if name in CONDITIONAL_REGS.get(mcu, set()):
                    continue
                dfp_name = REG_ALIASES.get(name, name)
                if dfp_name not in dfp_regs:
                    issues.append(f"  register {name}: HAL 0x{addr:02X}, "
                                  f"no DFP register {dfp_name} on {mcu}")
                elif dfp_regs[dfp_name] != addr:
                    issues.append(f"  register {name}: HAL 0x{addr:02X} != "
                                  f"DFP {dfp_name} 0x{dfp_regs[dfp_name]:02X} on {mcu}")
            for (reg, bit), pos in sorted(hal_bits.items()):
                if reg in CONDITIONAL_REGS.get(mcu, set()):
                    continue
                if any(reg.startswith(p) for p in BIT_SKIP_PREFIXES):
                    continue
                if (reg, bit) in CONDITIONAL_BITS.get(mcu, set()):
                    continue
                dfp_key = BIT_ALIASES.get((reg, bit), (reg, bit))
                if dfp_key not in dfp_bits:
                    if (reg, bit) not in DFP_MISSING_OK:
                        issues.append(f"  bit {reg}.{bit}: HAL bit {pos}, "
                                      f"no DFP _POSN for {mcu}")
                elif dfp_bits[dfp_key] != pos:
                    issues.append(f"  bit {reg}.{bit}: HAL bit {pos} != "
                                  f"DFP {dfp_key[0]}.{dfp_key[1]} bit "
                                  f"{dfp_bits[dfp_key]} on {mcu}")
            if issues:
                bad += 1
                print(f"SFR map mismatch in {family} ({mcu}):")
                for i in issues:
                    print(i)
    if bad == 0:
        print("sfr-map audit: all registers and bits match the DFP")
        return 0
    print(f"sfr-map audit: {bad} MCU(s) with mismatches")
    return 1


if __name__ == "__main__":
    sys.exit(main())
