#!/usr/bin/env python3
"""Discover the (mcu/*-mplabx dir, MCU variant) matrix for xc8-build.yml.

Reads the tracked Makefile set from git (no hardcoded module list, same
discipline host-tests.yml uses for its own module discovery) and pairs
each with its family's known MCU variants. Prints a compact JSON array to
stdout, consumed by the workflow as a GitHub Actions matrix `include` list.
"""

import json
import subprocess
import sys

PIC16_VARIANTS = ["16F873A", "16F874A", "16F876A", "16F877A"]
PIC18_VARIANTS = ["18F2455", "18F2550", "18F4455", "18F4550"]


def main():
    out = subprocess.run(
        ["git", "ls-files", "--", "*/mcu/*-mplabx/Makefile"],
        capture_output=True, text=True, check=True,
    ).stdout

    entries = []
    for line in out.splitlines():
        d = line.rsplit("/Makefile", 1)[0]
        if "pic16f87xa" in d:
            variants = PIC16_VARIANTS
        elif "pic18fxx5x" in d:
            variants = PIC18_VARIANTS
        else:
            sys.exit(f"unrecognized family for {d}")
        for v in variants:
            entries.append({"dir": d, "mcu": v})

    if not entries:
        sys.exit("no mcu/*-mplabx/Makefile found, discovery is broken")

    print(json.dumps(entries))


if __name__ == "__main__":
    main()
