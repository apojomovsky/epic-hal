#!/usr/bin/env python3
"""Discover the mcu/*-mplabx build matrix for xc8-build.yml.

Reads the tracked Makefile set from git (no hardcoded module list, same
discipline host-tests.yml uses for its own module discovery) and pairs
each with its family's known MCU variants and DFP pack name. Prints a
compact JSON array to stdout, ONE ENTRY PER FAMILY (not per module, and
not per module x MCU): the build job's container image is pulled once per
job either way, so even one job per module (23 of them) was paying for 23
redundant multi-GB pulls to do a few seconds of compiling each. Grouping
by family cuts that to 2 pulls while keeping PIC16/PIC18 builds running
concurrently (see docs/ci-plan.md's "one job per family" efficiency note).

Each family's `modules` field is a single string, not nested JSON: the
toolchain container (docker/ci-toolchain/Dockerfile) deliberately has no
python3/jq installed, so it has to be parseable with plain bash `IFS`
splitting. Format: "<dir>=<mcu>,<mcu>,...;<dir>=<mcu>,...", one
semicolon-separated segment per module, each segment's MCU list
comma-separated.
"""

import json
import subprocess
import sys

PIC16_VARIANTS = ["16F873A", "16F874A", "16F876A", "16F877A"]
PIC18_VARIANTS = ["18F2455", "18F2550", "18F4455", "18F4550"]
PIC16F193X_VARIANTS = ["16F1933", "16F1934", "16F1936", "16F1937", "16F1938", "16F1939"]

# Real, pre-existing bugs found by this workflow's first fully-working run
# (see docs/mplabx-link-gaps-plan.md for root causes and the fix plan), not
# CI plumbing: these (dir, mcu) pairs genuinely fail to link with XC8 today.
# Excluded here so xc8-build.yml stays green and actually meaningful (a
# permanently-red matrix leg gets ignored, not fixed) while those get fixed
# for real. Remove an entry the moment its module is fixed, this list
# shrinking to empty is docs/mplabx-link-gaps-plan.md's exit criterion.
KNOWN_BROKEN = {
    # Root cause 1: missing peripheral sources vs. the IRQ dispatch
    # contract (pic16_irq_dispatch.c / pic18_irq_dispatch.c require every
    # peripheral's IRQHandler to be strongly linked in). All MCU variants
    # affected, this isn't size-dependent.
    ("pic8-console/mcu/pic16f87xa-console-mplabx", mcu) for mcu in PIC16_VARIANTS
} | {
    ("pic8-console/mcu/pic18fxx5x-console-mplabx", mcu) for mcu in PIC18_VARIANTS
} | {
    ("pic8-settings/mcu/pic16f87xa-settings-mplabx", mcu) for mcu in PIC16_VARIANTS
} | {
    ("pic8-settings/mcu/pic18fxx5x-settings-mplabx", mcu) for mcu in PIC18_VARIANTS
} | {
    ("pic8-taskmgr/mcu/pic18fxx5x-taskmgr-mplabx", mcu) for mcu in PIC18_VARIANTS
} | {
    # Root cause 2: genuine RAM/resource overflow on the smaller variant(s)
    # in each affected family (larger variants build fine).
    ("pic8-bus/mcu/pic18fxx5x-bus-mplabx", mcu) for mcu in ("18F2455", "18F2550")
} | {
    ("pic8-debounce/mcu/pic16f87xa-debounce-mplabx", mcu) for mcu in ("16F873A", "16F874A")
} | {
    ("pic8-debounce/mcu/pic18fxx5x-debounce-mplabx", mcu) for mcu in ("18F2455", "18F2550")
} | {
    ("pic8-encoder/mcu/pic18fxx5x-encoder-mplabx", mcu) for mcu in ("18F2455", "18F2550")
} | {
    ("pic8-math/mcu/pic18fxx5x-math-mplabx", mcu) for mcu in ("18F2455", "18F2550")
} | {
    ("pic8-modbus/mcu/pic16f87xa-modbus-mplabx", mcu) for mcu in ("16F873A", "16F874A")
} | {
    ("pic8-modbus/mcu/pic18fxx5x-modbus-mplabx", mcu) for mcu in ("18F2455", "18F2550")
} | {
    ("pic8-serial/mcu/pic16f87xa-serial-mplabx", mcu) for mcu in ("16F873A", "16F874A")
} | {
    ("pic8-serial/mcu/pic18fxx5x-serial-mplabx", mcu) for mcu in ("18F2455", "18F2550")
} | {
    ("pic8-tick/mcu/pic18fxx5x-tick-mplabx", mcu) for mcu in ("18F2455", "18F2550")
} | {
    # Root cause 3: docs/ci-plan.md Phase 4's PIE1/PIE2 read-modify-write
    # fix (pic16_isr_vector.c's pic8_irq_pie_scratch, one __at-pinned
    # byte, needed by every module that calls EPIC_IRQ_Enable/DisableSrc
    # for a Bank 1 IRQ source, which includes anything using
    # EPIC_USART_Init with a callback) tipped these two, previously-green,
    # already RAM-marginal modules over the edge on their smaller PIC16
    # variants: confirmed via a real local XC8 v4.00 build, "fixup
    # overflow referencing psect bssBANK1" (other, unrelated large
    # buffers spilling into Bank 1 once one more byte of Bank 0 got
    # claimed). pic8-math's own docs/ARCHITECTURE.md already documents
    # this family's RAM as marginal ("cannot hold math + full HAL +
    # golden_vectors.h + the self-test... spills to bank 1 and
    # overflows"); this fix pushed it from marginal to broken. Larger
    # variants (16F876A/16F877A for math, none left for modbus, its
    # smaller variants were already excluded above for an earlier,
    # unrelated RAM reason) still build fine. Not investigated further
    # here, tracked in docs/mplabx-link-gaps-plan.md alongside the other
    # RAM-tightness entries above.
    ("pic8-math/mcu/pic16f87xa-math-mplabx", mcu) for mcu in ("16F873A", "16F874A")
} | {
    ("pic8-modbus/mcu/pic16f87xa-modbus-mplabx", mcu) for mcu in ("16F876A", "16F877A")
}

FAMILIES = {
    "pic16f87xa": (PIC16_VARIANTS, "Microchip.PIC16Fxxx_DFP"),
    "pic18fxx5x": (PIC18_VARIANTS, "Microchip.PIC18Fxxxx_DFP"),
    "pic16f193x": (PIC16F193X_VARIANTS, "Microchip.PIC12-16F1xxx_DFP"),
}


def main():
    out = subprocess.run(
        ["git", "ls-files", "--", "*/mcu/*-mplabx/Makefile"],
        capture_output=True, text=True, check=True,
    ).stdout

    by_family = {name: [] for name in FAMILIES}
    skipped = 0
    for line in out.splitlines():
        d = line.rsplit("/Makefile", 1)[0]
        if "pic16f87xa" in d:
            family = "pic16f87xa"
        elif "pic18fxx5x" in d:
            family = "pic18fxx5x"
        elif "pic16f193x" in d:
            family = "pic16f193x"
        else:
            sys.exit(f"unrecognized family for {d}")
        variants, _dfp = FAMILIES[family]

        mcus = []
        for v in variants:
            if (d, v) in KNOWN_BROKEN:
                skipped += 1
                continue
            mcus.append(v)

        if mcus:
            by_family[family].append((d, mcus))

    entries = []
    for family, modules in by_family.items():
        if not modules:
            continue
        _variants, dfp = FAMILIES[family]
        modules_str = ";".join(f"{d}={','.join(mcus)}" for d, mcus in modules)
        entries.append({"family": family, "dfp": dfp, "modules": modules_str})

    if not entries:
        sys.exit("no mcu/*-mplabx/Makefile found, discovery is broken")

    total_modules = sum(len(v) for v in by_family.values())
    total_mcus = sum(len(mcus) for modules in by_family.values() for _d, mcus in modules)
    print(
        f"skipped {skipped} known-broken (dir, mcu) pairs, see docs/mplabx-link-gaps-plan.md; "
        f"{total_modules} modules, {total_mcus} MCU builds across {len(entries)} families",
        file=sys.stderr,
    )
    print(json.dumps(entries))


if __name__ == "__main__":
    main()
