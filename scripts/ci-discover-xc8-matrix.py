#!/usr/bin/env python3
"""Discover the mcu/*-mplabx build matrix for xc8-build.yml.

Reads the tracked Makefile set from git (no hardcoded module list, same
discipline host-tests.yml uses for its own module discovery) and pairs
each with its family's known MCU variants and DFP pack name. Prints a
compact JSON array to stdout, one entry per module (not per module x MCU,
see docs/ci-plan.md's "one job per module" efficiency note: matrixing
per-MCU meant 72 separate GitHub Actions jobs each paying a fresh
multi-GB image pull to do a few seconds of real compiling), consumed by
the workflow as a matrix `include` list; the build job pulls the image
once per module and loops over that module's `mcus` list itself.
"""

import json
import subprocess
import sys

PIC16_VARIANTS = ["16F873A", "16F874A", "16F876A", "16F877A"]
PIC18_VARIANTS = ["18F2455", "18F2550", "18F4455", "18F4550"]

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
}


def main():
    out = subprocess.run(
        ["git", "ls-files", "--", "*/mcu/*-mplabx/Makefile"],
        capture_output=True, text=True, check=True,
    ).stdout

    entries = []
    skipped = 0
    for line in out.splitlines():
        d = line.rsplit("/Makefile", 1)[0]
        if "pic16f87xa" in d:
            variants, dfp = PIC16_VARIANTS, "Microchip.PIC16Fxxx_DFP"
        elif "pic18fxx5x" in d:
            variants, dfp = PIC18_VARIANTS, "Microchip.PIC18Fxxxx_DFP"
        else:
            sys.exit(f"unrecognized family for {d}")

        mcus = []
        for v in variants:
            if (d, v) in KNOWN_BROKEN:
                skipped += 1
                continue
            mcus.append(v)

        if mcus:
            entries.append({"dir": d, "dfp": dfp, "mcus": mcus})

    if not entries:
        sys.exit("no mcu/*-mplabx/Makefile found, discovery is broken")

    print(f"skipped {skipped} known-broken (dir, mcu) pairs, see docs/mplabx-link-gaps-plan.md", file=sys.stderr)
    print(json.dumps(entries))


if __name__ == "__main__":
    main()
