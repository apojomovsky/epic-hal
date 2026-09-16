#!/usr/bin/env python3
"""SFR-map audit: cross-check every SFR address/bit constant in the three
HALs' sfr.h files against the DFP proc headers (the ground truth; this
mechanizes the PIE2 misread-memory-map bug class). Runs in CI's target job
and `make audit`; host-side python3, DFP headers read via `docker run cat`
(the toolchain container has no python3). Exit 0 = no mismatches. Env:
EPIC_TOOLCHAIN_IMAGE, EPIC_XC8_ROOT.
"""

from __future__ import annotations

import os
import re
import subprocess
import sys

IMAGE = os.environ.get("EPIC_TOOLCHAIN_IMAGE", "epic-hal-toolchain:local")
PACKS = f"{os.environ.get('EPIC_XC8_ROOT', '/opt/microchip/xc8/v4.00')}/pic/packs"

# family -> (hal sfr.h path, [(mcu, dfp pack, proc header)])
FAMILIES = {
    "pic16f87xa-hal": (
        "pic16f87xa-hal/include/pic16f87xa_sfr.h",
        [
            ("16F873", "Microchip.PIC16Fxxx_DFP", "pic16f873.h"),
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
    "pic18f1320-hal": (
        "pic18f1320-hal/include/pic18f1320_sfr.h",
        [
            ("18F1320", "Microchip.PIC18Fxxxx_DFP", "pic18f1320.h"),
        ],
    ),
    "pic18f2520-hal": (
        "pic18f2520-hal/include/pic18f2520_sfr.h",
        [
            ("18F2520", "Microchip.PIC18Fxxxx_DFP", "pic18f2520.h"),
        ],
    ),
    "pic18f6520-hal": (
        "pic18f6520-hal/include/pic18f6520_sfr.h",
        [
            ("18F6520", "Microchip.PIC18Fxxxx_DFP", "pic18f6520.h"),
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
    "pic16f628a-hal": (
        "pic16f628a-hal/include/pic16f628a_sfr.h",
        [
            ("16F627", "Microchip.PIC16Fxxx_DFP", "pic16f627.h"),
            ("16F627A", "Microchip.PIC16Fxxx_DFP", "pic16f627a.h"),
            ("16LF627A", "Microchip.PIC16Fxxx_DFP", "pic16lf627a.h"),
            ("16F628", "Microchip.PIC16Fxxx_DFP", "pic16f628.h"),
            ("16LF628A", "Microchip.PIC16Fxxx_DFP", "pic16lf628a.h"),
            ("16F648A", "Microchip.PIC16Fxxx_DFP", "pic16f648a.h"),
            ("16F628A", "Microchip.PIC16Fxxx_DFP", "pic16f628a.h"),
        ],
    ),
    "pic16f83_84-hal": (
        "pic16f83_84-hal/include/pic16f83_84_sfr.h",
        [
            ("16F83", "Microchip.PIC16Fxxx_DFP", "pic16f83.h"),
            ("16F84", "Microchip.PIC16Fxxx_DFP", "pic16f84.h"),
            ("16F84A", "Microchip.PIC16Fxxx_DFP", "pic16f84a.h"),
        ],
    ),
    "pic16f63x_67x_68x-hal": (
        "pic16f63x_67x_68x-hal/include/pic16f63x_67x_68x_sfr.h",
        [
            ("16F631", "Microchip.PIC16Fxxx_DFP", "pic16f631.h"),
            ("16F630", "Microchip.PIC16Fxxx_DFP", "pic16f630.h"),
            ("16F676", "Microchip.PIC16Fxxx_DFP", "pic16f676.h"),
            ("16F639", "Microchip.PIC16Fxxx_DFP", "pic16f639.h"),
            ("16F684", "Microchip.PIC16Fxxx_DFP", "pic16f684.h"),
            ("16F685", "Microchip.PIC16Fxxx_DFP", "pic16f685.h"),
            ("16F688", "Microchip.PIC16Fxxx_DFP", "pic16f688.h"),
            ("16F689", "Microchip.PIC16Fxxx_DFP", "pic16f689.h"),
            ("16F677", "Microchip.PIC16Fxxx_DFP", "pic16f677.h"),
        ],
    ),
    "pic16f88x-hal": (
        "pic16f88x-hal/include/pic16f88x_sfr.h",
        [
            ("16F882", "Microchip.PIC16Fxxx_DFP", "pic16f882.h"),
            ("16F883", "Microchip.PIC16Fxxx_DFP", "pic16f883.h"),
            ("16F884", "Microchip.PIC16Fxxx_DFP", "pic16f884.h"),
            ("16F886", "Microchip.PIC16Fxxx_DFP", "pic16f886.h"),
            ("16F887", "Microchip.PIC16Fxxx_DFP", "pic16f887.h"),
        ],
    ),
    "pic16f7x-hal": (
        "pic16f7x-hal/include/pic16f7x_sfr.h",
        [
            ("16F72", "Microchip.PIC16Fxxx_DFP", "pic16f72.h"),
            ("16F73", "Microchip.PIC16Fxxx_DFP", "pic16f73.h"),
            ("16F74", "Microchip.PIC16Fxxx_DFP", "pic16f74.h"),
            ("16F76", "Microchip.PIC16Fxxx_DFP", "pic16f76.h"),
            ("16F77", "Microchip.PIC16Fxxx_DFP", "pic16f77.h"),
            ("16F737", "Microchip.PIC16Fxxx_DFP", "pic16f737.h"),
            ("16F747", "Microchip.PIC16Fxxx_DFP", "pic16f747.h"),
            ("16F767", "Microchip.PIC16Fxxx_DFP", "pic16f767.h"),
            ("16F777", "Microchip.PIC16Fxxx_DFP", "pic16f777.h"),
        ],
    ),
    "pic16f5x-hal": (
        "pic16f5x-hal/include/pic16f5x_sfr.h",
        [
            ("16F54", "Microchip.PIC16Fxxx_DFP", "pic16f54.h"),
            ("16F57", "Microchip.PIC16Fxxx_DFP", "pic16f57.h"),
            ("16F59", "Microchip.PIC16Fxxx_DFP", "pic16f59.h"),
            ("16F505", "Microchip.PIC16Fxxx_DFP", "pic16f505.h"),
            ("16F506", "Microchip.PIC16Fxxx_DFP", "pic16f506.h"),
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
    # 63x/67x/68x: the HAL-side Bank-0 WDTCON alias names the DFP's
    # WDTCON, so its address is checked instead of skipped.
    "WDTCON_BANK0": "WDTCON",
}
BIT_ALIASES = {
    # 87XA STATUS: the HAL's short names vs the DFP's long ones.
    ("STATUS", "C"): ("STATUS", "CARRY"),
    ("STATUS", "DC"): ("STATUS", "DC"),
    ("STATUS", "Z"): ("STATUS", "ZERO"),
    # OPTION_REG: the DFP spells the register name in full.
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
    # 87XA/88X OPTION_REG: the HAL spells the pull-up bit RBPU (the
    # datasheet name); the DFP spells it nRBPU.
    ("OPTION", "RBPU"): ("OPTION_REG", "nRBPU"),
    # 63x/67x/68x OPTION_REG: the pull-up bit covers both ports
    # (RABPU); the DFP spells it nRABPU.
    ("OPTION", "RABPU"): ("OPTION_REG", "nRABPU"),
    # 193X ADCON0: the HAL spells the GO/DONE bit GO_NDONE; the 87XA
    # and PIC18 spell it GO_DONE (the DFP defines both GO_DONE and the
    # n-prefixed alias at the same position).
    ("ADCON0_GO", "NDONE"): ("ADCON0", "GO"),
    ("ADCON0_GO", "DONE"): ("ADCON0", "GO_DONE"),
    # 628A T1CON: the HAL uses the datasheet name T1SYNC; the DFP
    # spells it nT1SYNC (1 = do not synchronize).
    ("T1CON", "T1SYNC"): ("T1CON", "nT1SYNC"),
    # 88X CCP: the HAL anchors the mode/duty/config bits on CCP1/CCP2
    # (PIC_CCP1_*); the DFP anchors the same bit names on CCP1CON/
    # CCP2CON. Positions still checked after the alias.
    ("CCP1", "CCP1M0"): ("CCP1CON", "CCP1M0"),
    ("CCP1", "CCP1M1"): ("CCP1CON", "CCP1M1"),
    ("CCP1", "CCP1M2"): ("CCP1CON", "CCP1M2"),
    ("CCP1", "CCP1M3"): ("CCP1CON", "CCP1M3"),
    ("CCP1", "DC1B0"): ("CCP1CON", "DC1B0"),
    ("CCP1", "DC1B1"): ("CCP1CON", "DC1B1"),
    ("CCP1", "P1M0"): ("CCP1CON", "P1M0"),
    ("CCP1", "P1M1"): ("CCP1CON", "P1M1"),
    ("CCP2", "CCP2M0"): ("CCP2CON", "CCP2M0"),
    ("CCP2", "CCP2M1"): ("CCP2CON", "CCP2M1"),
    ("CCP2", "CCP2M2"): ("CCP2CON", "CCP2M2"),
    ("CCP2", "CCP2M3"): ("CCP2CON", "CCP2M3"),
    ("CCP2", "DC2B0"): ("CCP2CON", "DC2B0"),
    ("CCP2", "DC2B1"): ("CCP2CON", "DC2B1"),
}

# Registers present on every shape but at a different address on
# some parts (bank variants, not absences): key mcu -> {HAL name:
# expected DFP address}, checked against the DFP (a wrong override
# fails loudly). The 63x 2-bank EEPROM/VRCON/ANSEL/WDTCON homes
# below; the runtime uses literal-token bank macros there, so the
# HAL constants stay canonical.
BANK_VARIANT_ADDRS = {
    "16F630": {"EEDATA": 0x9A, "EEADR": 0x9B, "EECON1": 0x9C,
               "EECON2": 0x9D, "VRCON": 0x99},
    "16F639": {"EEDATA": 0x9A, "EEADR": 0x9B, "EECON1": 0x9C,
               "EECON2": 0x9D, "VRCON": 0x99, "WDTCON": 0x18},
    "16F676": {"EEDATA": 0x9A, "EEADR": 0x9B, "EECON1": 0x9C,
               "EECON2": 0x9D, "VRCON": 0x99, "ANSEL": 0x91},
    "16F684": {"EEDATA": 0x9A, "EEADR": 0x9B, "EECON1": 0x9C,
               "EECON2": 0x9D, "VRCON": 0x99, "WDTCON": 0x18,
               "ANSEL": 0x91},
    "16F688": {"EEDATA": 0x9A, "EEADR": 0x9B, "EECON1": 0x9C,
               "EECON2": 0x9D, "VRCON": 0x99, "WDTCON": 0x18,
               "ANSEL": 0x91},
    # The 14-pin ADC parts keep ANSEL in Bank 1 (0x91, the
    # ANSEL_BANK1 alias's premise); 639/684/688 keep WDTCON in Bank
    # 0 (0x18, the WDTCON_BANK0 alias's premise, used by the shared
    # WDT driver).

}
# Registers and bits that are legitimately absent from a part's DFP
# header: family-conditional SFRs on the smaller parts (the HAL defines
# the constants unconditionally and guards the usage). key: mcu -> set
# of register names to skip entirely.
CONDITIONAL_REGS = {
    "16F873": {"PORTD", "PORTE", "TRISD", "TRISE", "CMCON", "CVRCON"},
    "16F873A": {"PORTD", "PORTE", "TRISD", "TRISE", "PIE1", "PIR1", "PIR2"},
    "16F876A": {"PORTD", "PORTE", "TRISD", "TRISE", "PIE1", "PIR1", "PIR2"},
}
# On the 28-pin parts the whole PSP interrupt path is absent; the
# PIE1/PIR1/PIR2 registers still exist, only their PSP bits are
# conditional. The non-A 873 additionally lacks the comparator/VREF
# registers and the PIE2/PIR2 comparator bits (DPF headers).
CONDITIONAL_BITS = {
    "16F873": {("PIE1", "PSPIE"), ("PIR1", "PSPIF"),
               ("PIE2", "CMIE"), ("PIR2", "CMIF"),
               ("ADCON1", "ADCS2")},
    "16F873A": {("PIE1", "PSPIE"), ("PIR1", "PSPIF")},
    "16F876A": {("PIE1", "PSPIE"), ("PIR1", "PSPIF")},
    # 18F2455/2550 (28-pin, no SPP): the SPP registers and the SPP
    # interrupt bits are absent from those parts' DFP headers.
    "18F2455": {("IPR1", "SPPIP"), ("PIE1", "SPPIE"), ("PIR1", "SPPIF")},
    "18F2550": {("IPR1", "SPPIP"), ("PIE1", "SPPIE"), ("PIR1", "SPPIF")},
    # 88X 28-pin parts (882/883/886): ANSEL ANS5/ANS6/ANS7 are
    # unimplemented on the 11-channel ADC (present on 884/887).
    "16F882": {("ANSEL", "ANS5"), ("ANSEL", "ANS6"), ("ANSEL", "ANS7")},
    "16F883": {("ANSEL", "ANS5"), ("ANSEL", "ANS6"), ("ANSEL", "ANS7")},
    "16F886": {("ANSEL", "ANS5"), ("ANSEL", "ANS6"), ("ANSEL", "ANS7")},
    # 63x/67x/68x 4-bank parts: the 2-bank RAIF/RAIE/RAPU spellings
    # and the PIR1 EEPROM pair exist only on the 2-bank shapes;
    # ANS2/ANS3 exist only where ANSEL is fully implemented.
    "16F631": {("INTCON", "RAIF"), ("INTCON", "RAIE"), ("OPTION", "RAPU"),
               ("PIR1", "EEIF"), ("PIE1", "EEIE"),
               ("ANSEL", "ANS2"), ("ANSEL", "ANS3")},
    "16F677": {("INTCON", "RAIF"), ("INTCON", "RAIE"), ("OPTION", "RAPU"),
               ("PIR1", "EEIF"), ("PIE1", "EEIE")},
    # 63x/67x/68x 16F630: the DFP spells the PORTA-change pair
    # RAIF/RAIE (no RABIF/RABIE); RAPU is a HAL-only name for the
    # pull-up bit. The VRCON comparator-reference trio exists only
    # on the dual-comparator shapes. The 1K dice additionally lack
    # the SBOREN/ULPWUE PCON bits and the T1GINV gate-invert bit.
    "16F630": {("INTCON", "RABIF"), ("INTCON", "RABIE"),
               ("OPTION", "RAPU"),
               ("PCON", "SBOREN"), ("PCON", "ULPWUE"),
               ("T1CON", "T1GINV"),
               ("VRCON", "C1VREN"), ("VRCON", "C2VREN"),
               ("VRCON", "VP6EN")},
    "16F639": {("INTCON", "RABIF"), ("INTCON", "RABIE"),
               ("OPTION", "RAPU"),
               ("VRCON", "C1VREN"), ("VRCON", "C2VREN"),
               ("VRCON", "VP6EN")},
    "16F676": {("INTCON", "RABIF"), ("INTCON", "RABIE"),
               ("OPTION", "RAPU"),
               ("PCON", "SBOREN"), ("PCON", "ULPWUE"),
               ("T1CON", "T1GINV"),
               ("VRCON", "C1VREN"), ("VRCON", "C2VREN"),
               ("VRCON", "VP6EN")},
    "16F684": {("INTCON", "RABIF"), ("INTCON", "RABIE"),
               ("OPTION", "RAPU"),
               ("VRCON", "C1VREN"), ("VRCON", "C2VREN"),
               ("VRCON", "VP6EN")},
    "16F685": {("INTCON", "RAIF"), ("INTCON", "RAIE"), ("OPTION", "RAPU"),
               ("PIR1", "EEIF"), ("PIE1", "EEIE")},
    "16F688": {("INTCON", "RABIF"), ("INTCON", "RABIE"),
               ("OPTION", "RAPU"), ("PCON", "SBOREN"),
               ("VRCON", "C1VREN"), ("VRCON", "C2VREN"),
               ("VRCON", "VP6EN")},
    "16F689": {("INTCON", "RAIF"), ("INTCON", "RAIE"), ("OPTION", "RAPU"),
               ("PIR1", "EEIF"), ("PIE1", "EEIE")},

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
    # 88X 28-pin parts (882/883/886): no PORTD; the HAL defines
    # PORTD/TRISD unconditionally and guards the usage.
    "16F882": {"PORTD", "TRISD"},
    "16F883": {"PORTD", "TRISD"},
    "16F886": {"PORTD", "TRISD"},
    # 63x/67x/68x 16F631: no ANSELH (677-only); the HAL defines it
    # unconditionally and gates the usage on FAMILY_HAS_ANSELH.
    "16F631": {"ANSELH", "ANSEL_BANK1", "WDTCON_BANK0"},
    "16F677": {"ANSEL_BANK1", "WDTCON_BANK0"},
    # 63x/67x/68x 16F630: no PORTB, no dual comparators, no PIE2/PIR2,
    # no SRCON, no OSCCON/OSCTUNE/WDTCON, no ANSEL; ANSEL_BANK1 is the
    # HAL-side alias pattern above. EEPROM/VRCON bank addresses ride
    # BANK_VARIANT_ADDRS, not this list.
    "16F630": {"ANSEL", "ANSELH", "ANSEL_BANK1", "CM1CON0", "CM2CON0",
               "CM2CON1", "IOCB", "OSCCON", "OSCTUNE", "PIE2", "PIR2",
               "PORTB", "SRCON", "TRISB", "WDTCON", "WPUB", "WDTCON_BANK0"},
    "16F639": {"ANSEL", "ANSELH", "ANSEL_BANK1", "CM1CON0", "CM2CON0",
               "CM2CON1", "IOCB", "PIE2", "PIR2", "PORTB", "SRCON",
               "TRISB", "WPUA", "WPUB"},
    "16F676": {"ANSELH", "ANSEL_BANK1", "CM1CON0", "CM2CON0", "CM2CON1",
               "IOCB", "OSCCON", "OSCTUNE", "PIE2", "PIR2", "PORTB",
               "SRCON", "TRISB", "WDTCON", "WPUB", "WDTCON_BANK0"},
    "16F684": {"ANSELH", "ANSEL_BANK1", "CM1CON0", "CM2CON0", "CM2CON1",
               "IOCB", "PIE2", "PIR2", "PORTB", "SRCON", "TRISB",
               "WPUB"},
    "16F685": {"ANSEL_BANK1", "WDTCON_BANK0"},
    "16F688": {"ANSELH", "ANSEL_BANK1", "CM1CON0", "CM2CON0", "CM2CON1",
               "IOCB", "PIE2", "PIR2", "PORTB", "SRCON", "TRISB",
               "WPUB"},
    "16F689": {"ANSEL_BANK1", "WDTCON_BANK0"},
    # PIC16F7x family (DFP-verified, exact per-part absence). 16F72
    # (28-pin, no USART/CCP2/PM bank): the missing registers are
    # CCP2CON/CCPR2H/CCPR2L/PIE2/PIR2/PMADR/PMDATA and the USART/PSP/
    # CMP bit rows. The DS30498 parts (16F737/747/767/777) use the
    # 10-bit ADRESH/ADRESL pair; the DS30325 parts (16F72-77) use the
    # single 8-bit ADRES. PortD/E and their TRIS rows exist only where
    # the DFP declares them.
    "16F72":  {"CCP2CON", "CCPR2H", "CCPR2L", "PIE2", "PIR2",
               "PMADR", "PMDATA", "ADRESH", "ADRESL",
               "PORTD", "PORTE", "TRISD", "TRISE",
               "TXSTA", "SPBRG", "RCSTA", "TXREG", "RCREG"},
    "16F73":  {"ADRESH", "ADRESL", "PORTD", "PORTE", "TRISD", "TRISE"},
    "16F74":  {"ADRESH", "ADRESL"},
    "16F76":  {"ADRESH", "ADRESL", "PORTD", "PORTE", "TRISD", "TRISE"},
    "16F77":  {"ADRESH", "ADRESL"},
    "16F737": {"ADRES", "PORTD", "TRISD", "TRISE"},
    "16F747": {"ADRES"},
    "16F767": {"ADRES", "PORTD", "TRISD", "TRISE"},
    "16F777": {"ADRES"},

    # PIC16F5x: sfr.h carries every port register and OSCCAL,
    # #if-guarded per part (pic16f5x_hal.h); parse_hal reads the raw
    # text, so each absent die register is listed (DFP "equ" rows):
    # PORTC on 57+, PORTD/E on 59, OSCCAL instead of PORTA on 505/506,
    # comparator/ADC files on 506. TRIS/OPTION are control-space.
    "16F54":  {"PORTC", "PORTD", "PORTE", "OSCCAL"},
    "16F57":  {"PORTD", "PORTE", "OSCCAL"},
    "16F59":  {"OSCCAL"},
    "16F505": {"PORTA", "PORTD", "PORTE"},
    "16F506": {"PORTA", "PORTD", "PORTE"},

})

# PIC16F7x bit rows absent from the smaller/older parts' DFP headers:
# the HAL defines the bit unconditionally and gates usage on the family
# macros. 16F72 has no USART/CCP2/PM (its PIE1/PIE2/PIR1 flag rows for
# those sources are absent); the 28-pin parts have no PSP bit; several
# DS30325 parts carry no ADCON1 ADCS2/ADFM (single 8-bit ADC) and no
# RCSTA ADDEN (no auto-address-detect). Enumerated exactly as the DFP
# reports missing for each MCU.
CONDITIONAL_BITS.update({
    "16F72": {("ADCON1", "ADCS2"), ("ADCON1", "ADFM"),
              ("PIE1", "PSPIE"), ("PIE1", "RCIE"), ("PIE1", "TXIE"),
              ("PIE2", "CCP2IE"), ("PIR1", "PSPIF"),
              ("PIR1", "RCIF"), ("PIR1", "TXIF"), ("PIR2", "CCP2IF")},
    "16F73": {("ADCON1", "ADCS2"), ("ADCON1", "ADFM"),
              ("PIE1", "PSPIE"), ("PIR1", "PSPIF"), ("RCSTA", "ADDEN")},
    "16F74": {("ADCON1", "ADCS2"), ("ADCON1", "ADFM"),
              ("RCSTA", "ADDEN")},
    "16F76": {("ADCON1", "ADCS2"), ("ADCON1", "ADFM"),
              ("PIE1", "PSPIE"), ("PIR1", "PSPIF"), ("RCSTA", "ADDEN")},
    "16F77": {("ADCON1", "ADCS2"), ("ADCON1", "ADFM"),
              ("RCSTA", "ADDEN")},
})

# PIC16F5x 20-pin parts: the DFP STATUS row carries PA0 only - the
# 505/506 have no program-page bits (their 2-bit FSR bank select does
# not ride STATUS; DS41319 sections 3.0/4.0). PA1/PA2 exist on the
# 18/28/40-pin parts (16F54/57/59).
CONDITIONAL_BITS.update({
    "16F505": {("STATUS", "PA1"), ("STATUS", "PA2")},
    "16F506": {("STATUS", "PA1"), ("STATUS", "PA2")},
})

# Bits the DFP does not define but the datasheet documents:
# STATUS<PD>/<TO> (POR-only flags) have no _POSN macros; the 87XA
# OPTION_REG RBPU and the 193X SRCON1 SRQEN/SRNQEN are documented in
# the datasheet but absent from the DFP's bit macros.
DFP_MISSING_OK = {
    ("STATUS", "PD"), ("STATUS", "TO"),
    ("OPTION", "RBPU"), ("OPTION_REG", "RBPU"),
    ("SRCON1", "SRQEN"), ("SRCON1", "SRNQEN"),
    # 63x/67x/68x 2-bank RABPU: the pull-up global is documented in
    # DS40300/DS41262 but the 2-bank DFP headers carry no _POSN for
    # it (the 4-bank headers spell it nRABPU, covered by the alias).
    ("OPTION", "RABPU"),
    # PIC16F5x OPTION: control-space (written with `option`, no SFR
    # address; the DFP declares `extern volatile __control unsigned
    # char OPTION` and no _POSN macros). The HAL's bits (PS/PSA/T0SE/
    # T0CS) are datasheet facts (DS41213D Register 9-1).
    ("OPTION", "PS0"), ("OPTION", "PS1"), ("OPTION", "PS2"),
    ("OPTION", "PSA"), ("OPTION", "T0SE"), ("OPTION", "T0CS"),
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
    import argparse
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--family", choices=("PIC16F87XA", "PIC18Fxx5x",
                                         "PIC16F193X", "PIC16F88X", "PIC16F628A",
                                         "PIC16F83_84",
                                         "PIC16F63x_67x_68x", "PIC18F1320",
                                         "PIC18F2520",
                                         "PIC18F6520", "PIC16F7x",
                                         "PIC16F5x"), default=None,
                    help="only this manifest family (the sharded CI jobs)")
    args = ap.parse_args()
    hal_label = {"PIC16F87XA": "pic16f87xa-hal",
                 "PIC18Fxx5x": "pic18fxx5x-hal",
                 "PIC16F193X": "pic16f193x-hal",
                 "PIC16F88X": "pic16f88x-hal",
                 "PIC16F628A": "pic16f628a-hal",
                 "PIC16F83_84": "pic16f83_84-hal",
                 "PIC16F63x_67x_68x": "pic16f63x_67x_68x-hal",
                 "PIC18F1320": "pic18f1320-hal",
                 "PIC18F2520": "pic18f2520-hal",
                 "PIC18F6520": "pic18f6520-hal",
                 "PIC16F7x": "pic16f7x-hal",
                 "PIC16F5x": "pic16f5x-hal"}[args.family] \
        if args.family else None
    bad = 0
    for family, (sfr_path, mcus) in FAMILIES.items():
        if hal_label is not None and family != hal_label:
            continue
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
                expected = BANK_VARIANT_ADDRS.get(mcu, {}).get(name, addr)
                if dfp_name not in dfp_regs:
                    issues.append(f"  register {name}: HAL 0x{addr:02X}, "
                                  f"no DFP register {dfp_name} on {mcu}")
                elif dfp_regs[dfp_name] != expected:
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
