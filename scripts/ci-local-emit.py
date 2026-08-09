#!/usr/bin/env python3
"""Emit everything the local `make target-ci` replica of CI's target job
needs, on the host (this script needs python3; the toolchain container
has none): the real-target matrix and build scripts, the fixed sim
variants, and the per-family bundles. Mirrors the emit step in
.github/workflows/ci.yml so the local replica and CI agree on what gets
built; the container-side loops (ci-target-build.sh / ci-target-sim.sh /
ci-target-bundle.sh) are the same scripts CI runs.
"""
from __future__ import annotations

import json
import os
import pathlib
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import epicmanifest  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parents[1]
XC8_INSTALL_DIR = os.environ.get(
    "XC8_INSTALL_DIR",
    f"/opt/microchip/xc8/v{os.environ.get('XC8_VERSION', '4.00')}",
)

# The fixed sim-variant list, kept in sync by hand with the run_one
# calls at the bottom of scripts/ci-target-sim.sh (the same constraint
# ci.yml's emit step documents).
SIM_VARIANTS = [
    ("epic-tick", "16F877A"),
    ("epic-tick", "18F4550"),
    ("epic-pic16f193x-firmware", "16F1937"),
    ("epic-swuart", "16F877A"),
    ("epic-math", "16F877A"),
    ("epic-math", "18F4550"),
    ("pic16f87xa-hal", "16F877A"),
    ("pic18fxx5x-hal", "18F4550"),
    ("epic-pid", "18F4550"),
    ("epic-fsm", "16F877A"),
    ("epic-adcfilter", "16F877A"),
    ("epic-encoder", "16F877A"),
    ("epic-bus", "16F877A"),
    ("epic-serial", "16F877A"),
    ("epic-lcd", "16F877A"),
    ("epic-debounce", "16F877A"),
    ("epic-console", "18F4550"),
    ("epic-taskmgr", "18F4550"),
    ("epic-settings", "18F4550"),
    ("epic-modbus", "18F4550"),
]


def main() -> None:
    manifest = epicmanifest.load(epicmanifest.default_path())

    # Real-target matrix + build scripts, reusing epic_build.py matrix's
    # own filter so there is one definition of "what's in the matrix".
    matrix_json = subprocess.run(
        [sys.executable, "scripts/epic_build.py", "matrix"],
        check=True, capture_output=True, text=True, cwd=REPO,
    ).stdout
    with open(REPO / "matrix.txt", "w") as matrix_f:
        for entry in json.loads(matrix_json):
            fam_name = entry["family"]
            fam = manifest.families[fam_name]
            dfp_dir = f"{XC8_INSTALL_DIR}/pic/packs/{fam.dfp}/xc8"
            for pair in entry["modules"].split(";"):
                name, mcus = pair.split("=")
                for mcu in mcus.split(","):
                    matrix_f.write(f"{fam_name} {name} {mcu}\n")
                    subprocess.run(
                        [sys.executable, "scripts/epic_build.py", "build",
                         "--module", name, "--mcu", mcu,
                         "--build-dir", f"build/{name}",
                         "--dfp-dir", dfp_dir],
                        check=True, stdout=subprocess.DEVNULL, cwd=REPO,
                    )

    # Sim build scripts for the fixed gate list.
    for module, mcu in SIM_VARIANTS:
        fam = manifest.family_of(mcu)
        dfp_dir = f"{XC8_INSTALL_DIR}/pic/packs/{fam.dfp}/xc8"
        subprocess.run(
            [sys.executable, "scripts/epic_build.py", "build",
             "--module", module, "--mcu", mcu, "--variant", "sim",
             "--build-dir", f"build-sim/{module}",
             "--dfp-dir", dfp_dir],
            check=True, stdout=subprocess.DEVNULL, cwd=REPO,
        )

    # Bundles, one per family, for the bundle gate.
    for fam_name in sorted(manifest.families):
        subprocess.run(
            [sys.executable, "scripts/make_bundle.py",
             "--family", fam_name, "--version", "ci-test"],
            check=True, stdout=subprocess.DEVNULL, cwd=REPO,
        )

    print("emit: matrix, sim variants, and bundles ready")


if __name__ == "__main__":
    main()
