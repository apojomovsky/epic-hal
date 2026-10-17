#!/usr/bin/env python3
"""Emit everything the local `make target-ci` replica of CI's target job
needs, on the host (python3; the toolchain container has none): the
real-target matrix and build scripts, the fixed sim variants, and the
per-family bundles. Mirrors ci.yml's emit step so the local replica and CI
agree on what the container-side loops (ci-target-*.sh) build.
"""
from __future__ import annotations

import json
import os
import pathlib
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "build"))

import epicmanifest  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parents[2]
XC8_INSTALL_DIR = os.environ.get(
    "XC8_INSTALL_DIR",
    f"/opt/microchip/xc8/v{os.environ.get('XC8_VERSION', '4.00')}",
)

# The fixed sim-variant list, kept in sync by hand with the run_one
# calls at the bottom of scripts/ci/ci-target-sim.sh (the same constraint
# ci.yml's emit step documents).
SIM_VARIANTS = [
    ("lib/tick", "16F877A"),
    ("lib/tick", "18F4550"),
    ("hal/pic14e/16f193x", "16F1939"),
    ("hal/pic14e/16f1508", "16F1509"),
    ("hal/pic14e/16f188xx", "16F18877"),
    ("lib/tick", "16F1939"),
    ("lib/fsm", "16F1939"),
    ("lib/swuart", "16F877A"),
    ("lib/math", "16F877A"),
    ("lib/math", "18F4550"),
    ("hal/pic14/16f87xa", "16F877A"),
    ("hal/pic14/16f7x", "16F77"),
    ("hal/pic14/16f818_819", "16F819"),
    ("hal/pic18/18fxx5x", "18F4550"),
    ("hal/pic18/18f2520", "18F2320"),
    ("hal/pic18/18f2520", "18F2520"),
    ("hal/pic18/18f6520", "18F6520"),
    ("lib/pid", "18F4550"),
    ("lib/fsm", "16F877A"),
    ("lib/adcfilter", "16F877A"),
    ("lib/encoder", "16F877A"),
    ("lib/bus", "16F877A"),
              ("lib/mcp23x17", "16F877A"),
    ("lib/serial", "16F877A"),
    ("lib/lcd", "16F877A"),
    ("lib/debounce", "16F877A"),
    ("lib/console", "18F4550"),
    ("lib/taskmgr", "18F4550"),
    ("lib/settings", "18F4550"),
    ("lib/modbus", "18F4550"),
    ("tests/epic-combo-uart-ssp", "16F877A"),
    ("tests/epic-combo-multitimer", "16F877A"),
    ("tests/epic-combo-adc-uart", "16F877A"),
    ("tests/epic-combo-rb-uart", "16F877A"),
    ("tests/epic-combo-eeprom-isr", "18F4550"),
    ("tests/epic-combo-tick-serial", "16F877A"),
    ("tests/epic-combo-tick-settings", "18F4550"),
    ("tests/epic-combo-taskmgr-serial", "18F4550"),
    ("tests/epic-combo-encoder-tick", "16F877A"),
    ("tests/epic-combo-lcd-tick", "18F4550"),
    ("tests/epic-combo-swuart-tick", "16F877A"),
    ("tests/epic-combo-rx-loopback", "16F877A"),
    ("tests/epic-combo-modbus-full", "18F4550"),
    ("hal/pic14/16f88x", "16F887"),
    ("hal/pic12/16f5x", "16F54"),
    ("hal/pic12/16f5x", "16F59"),
    ("hal/pic12/16f5x", "16F57"),
    ("hal/pic12/16f5x", "16F506"),
    ("hal/pic12/16f5x", "16F505"),
    ("lib/tick", "16F887"),
    ("lib/tick", "16F648A"),
    ("lib/swuart", "16F887"),
    ("lib/math", "16F887"),
    ("lib/fsm", "16F887"),
    ("lib/adcfilter", "16F887"),
    ("lib/encoder", "16F887"),
    ("lib/bus", "16F887"),
    ("lib/mcp23x17", "16F887"),
    ("lib/serial", "16F887"),
    ("lib/debounce", "16F887"),
    ("hal/pic14/16f628a", "16F628A"),
    ("hal/pic14/16f83_84", "16F84A"),
    ("hal/pic14/16f63x_67x_68x", "16F677"),
    ("hal/pic18/18f1320", "18F1320"),
]


def main() -> None:
    manifest = epicmanifest.load(epicmanifest.default_path())

    # Real-target matrix + build scripts, reusing epic_build.py matrix's
    # own filter so there is one definition of "what's in the matrix".
    matrix_json = subprocess.run(
        [sys.executable, "scripts/build/epic_build.py", "matrix"],
        check=True, capture_output=True, text=True, cwd=REPO,
    ).stdout
    with open(REPO / "matrix.txt", "w") as matrix_f:
        for entry in json.loads(matrix_json):
            fam_name = entry["family"]
            fam = manifest.families[fam_name]
            dfp_dir = f"{XC8_INSTALL_DIR}/pic/packs/{epicmanifest.dfp_name(fam)}/xc8"
            for pair in entry["modules"].split(";"):
                name, mcus = pair.split("=")
                for mcu in mcus.split(","):
                    matrix_f.write(f"{fam_name} {name} {mcu}\n")
                    subprocess.run(
                        [sys.executable, "scripts/build/epic_build.py", "build",
                         "--module", name, "--mcu", mcu,
                         "--build-dir", f"build/{name}",
                         "--dfp-dir", dfp_dir],
                        check=True, stdout=subprocess.DEVNULL, cwd=REPO,
                    )

    # Sim build scripts for the fixed gate list.
    for module, mcu in SIM_VARIANTS:
        fam = manifest.family_of(mcu)
        dfp_dir = f"{XC8_INSTALL_DIR}/pic/packs/{epicmanifest.dfp_name(fam)}/xc8"
        subprocess.run(
            [sys.executable, "scripts/build/epic_build.py", "build",
             "--module", module, "--mcu", mcu, "--variant", "sim",
             "--build-dir", f"build-sim/{module}",
             "--dfp-dir", dfp_dir],
            check=True, stdout=subprocess.DEVNULL, cwd=REPO,
        )

    # Bundles, one per family, for the bundle gate.
    for fam_name in sorted(manifest.families):
        subprocess.run(
            [sys.executable, "scripts/build/make_bundle.py",
             "--family", fam_name, "--version", "ci-test"],
            check=True, stdout=subprocess.DEVNULL, cwd=REPO,
        )

    print("emit: matrix, sim variants, and bundles ready")


if __name__ == "__main__":
    main()
