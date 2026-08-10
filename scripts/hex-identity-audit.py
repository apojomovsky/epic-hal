#!/usr/bin/env python3
"""Hex-rebuild identity audit: every matrix .hex must be byte-identical
across a rebuild, so codegen or layout drift shows up as a reviewable
diff instead of a flaky gate.

Each (module, mcu) matrix pair is built twice into separate build dirs
(build-ident/<module>/<mcu>/a and .../b, both gitignored via build*/)
from freshly emitted build scripts, and the two .hex files are compared
by sha256. Any mismatch or build failure fails the audit with exit 1.

Determinism probe (2026-08-11, XC8 v4.00 in pic8-hal-toolchain:local):
epic-tick 16F877A emitted into build/a and build/b and built twice in
the container; both runs produced the identical .hex
(sha256 c09a3b9a74c8d92b85e9ee454cd00ccfaba54bad131cba2bb734d654b42ad498).
Same sources, same flags, same bytes: XC8 v4.00 is deterministic, so
the whole-matrix comparison below is meaningful.

Run: python3 scripts/hex-identity-audit.py
Needs: the toolchain image (default pic8-hal-toolchain:local, override
with EPIC_TOOLCHAIN_IMAGE) and the XC8 root inside it (default
/opt/microchip/xc8/v4.00, override with EPIC_XC8_ROOT).
"""

from __future__ import annotations

import hashlib
import json
import os
import pathlib
import shutil
import subprocess
import sys

IMAGE = os.environ.get("EPIC_TOOLCHAIN_IMAGE", "pic8-hal-toolchain:local")
XC8_ROOT = os.environ.get("EPIC_XC8_ROOT", "/opt/microchip/xc8/v4.00")

REPO = pathlib.Path(__file__).resolve().parent.parent

OUT_ROOT = REPO / "build-ident"


def emit_build(module: str, mcu: str, dfp_dir: str, rel_dir: str) -> None:
    """Emit one build script (with its config TU) via the build driver.

    rel_dir is repo-root-relative (e.g. build-ident/<module>/<mcu>/a),
    matching how CI and the probe invoke the driver; the emitted script
    and the hex both land under it.
    """
    subprocess.run(
        ["python3", "scripts/epic_build.py", "build",
         "--module", module, "--mcu", mcu,
         "--build-dir", rel_dir, "--dfp-dir", dfp_dir],
        check=True, stdout=subprocess.DEVNULL,
        cwd=REPO,
    )


def run_build(module: str, mcu: str, rel_dir: str) -> str:
    """Run one emitted build script in the container; return its output.

    The script path must stay repo-relative: the container only knows
    /repo, so an absolute host path cannot be opened inside it.
    """
    rel_script = f"{rel_dir}/{mcu}/build.sh"
    cmd = [
        "docker", "run", "--rm",
        "-v", f"{REPO}:/repo", "-w", "/repo",
        IMAGE, "sh", "-c",
        f"cd /repo && EPIC_REPO_ROOT=/repo sh {rel_script}",
    ]
    out = subprocess.run(cmd, capture_output=True, text=True)
    return out.stdout + out.stderr


def sha256_of(dir_hex: pathlib.Path) -> str:
    return hashlib.sha256(dir_hex.read_bytes()).hexdigest()


def main() -> int:
    import argparse
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--family", choices=("PIC16F87XA", "PIC18Fxx5x",
                                         "PIC16F193X"), default=None,
                    help="only this manifest family (the sharded CI jobs)")
    args = ap.parse_args()
    matrix = json.loads(subprocess.run(
        ["python3", "scripts/epic_build.py", "matrix"],
        check=True, capture_output=True, text=True, cwd=REPO,
    ).stdout)

    bad = 0
    audited = 0
    for entry in matrix:
        if args.family is not None and entry["family"] != args.family:
            continue
        dfp_dir = f"{XC8_ROOT}/pic/packs/{entry['dfp']}/xc8"
        for pair in entry["modules"].split(";"):
            module, mcus = pair.split("=")
            for mcu in mcus.split(","):
                rel_root = f"build-ident/{module}/{mcu}"
                for side in ("a", "b"):
                    side_dir = OUT_ROOT / module / mcu / side
                    shutil.rmtree(side_dir, ignore_errors=True)
                    emit_build(module, mcu, dfp_dir, f"{rel_root}/{side}")
                outputs = {}
                ok = True
                for side in ("a", "b"):
                    side_dir = OUT_ROOT / module / mcu / side
                    output = run_build(module, mcu, f"{rel_root}/{side}")
                    hexes = list(side_dir.glob("*.hex"))
                    if len(hexes) != 1:
                        print(f"{module} {mcu}: build side '{side}' produced "
                              f"{len(hexes)} .hex files, expected 1")
                        print(output.strip()[-2000:])
                        ok = False
                        break
                    outputs[side] = sha256_of(hexes[0])
                if not ok:
                    print(f"hex-identity build failure: {module} ({mcu})")
                    bad += 1
                    continue
                audited += 1
                if outputs["a"] != outputs["b"]:
                    print(f"hex-identity mismatch: {module} ({mcu})\n"
                          f"  a: {outputs['a']}\n  b: {outputs['b']}")
                    bad += 1

    if bad:
        print(f"hex-identity audit: {bad} (module, MCU) pair(s) with a "
              "non-identical or failed rebuild")
        return 1
    print(f"hex-identity audit: {audited} matrix hexes byte-identical "
          "across a rebuild")
    return 0


if __name__ == "__main__":
    sys.exit(main())
