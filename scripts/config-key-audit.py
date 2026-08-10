#!/usr/bin/env python3
"""Config-key audit: every manifest example's #pragma config keys and
values must be valid XC8 config settings for every MCU that example's
family supports.

XC8 keeps its config-option database inside the compiler binary (no
file ground truth to parse), and it validates pragmas at LINK time, not
compile time (`xc8-cc -c` accepts any pragma silently; the (1363)
"unknown configuration setting/register" diagnostic only fires when the
linker resolves the config words). So this audit links each example's
generated config translation unit together with a trivial main and
fails on (1363), the exact check the real build performs, minus
compiling and linking the module's sources. The same diagnostic also
catches invalid VALUES for otherwise-valid keys.

Both variants are audited: the real-target config and the sim config
(the MPLAB SIM diagnostic builds link their own config TU too).

Run: python3 scripts/config-key-audit.py
Needs: the toolchain image (default pic8-hal-toolchain:local, override
with EPIC_TOOLCHAIN_IMAGE) and the XC8 root inside it (default
/opt/microchip/xc8/v4.00, override with EPIC_XC8_ROOT).
"""

from __future__ import annotations

import concurrent.futures
import os
import pathlib
import re
import subprocess
import sys

IMAGE = os.environ.get("EPIC_TOOLCHAIN_IMAGE", "pic8-hal-toolchain:local")
XC8_ROOT = os.environ.get("EPIC_XC8_ROOT", "/opt/microchip/xc8/v4.00")
PACKS = f"{XC8_ROOT}/pic/packs"

REPO = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
import epicmanifest as manifest_lib  # noqa: E402
import epic_build  # noqa: E402

OUT_ROOT = REPO / "build-sim" / "audit-config"

# XC8's link-time config diagnostic carries the offending key and the
# offending value; the numeric id 1363 is stable across XC8 releases.
UNKNOWN_KEY_RE = re.compile(r"\(1363\) unknown configuration "
                            r"setting/register \(([^=\s]+)\s*=")

MAIN_TU = "#include <xc.h>\nvoid main(void) { for (;;) {} }\n"


def link_config_tu(mcu: str, dfp_pack: str, rel_path: str) -> str:
    """Link one config TU (plus a trivial main) in the container."""
    main_rel = "build-sim/audit-config/_audit_main.c"
    (REPO / main_rel).write_text(MAIN_TU)
    cmd = [
        "docker", "run", "--rm",
        "-v", f"{REPO}:/repo", "-w", "/repo",
        IMAGE, "sh", "-c",
        f"xc8-cc -mdfp={PACKS}/{dfp_pack}/xc8 -mcpu={mcu.lower()} "
        f"{rel_path} {main_rel} -o /tmp/audit-config.hex",
    ]
    out = subprocess.run(cmd, capture_output=True, text=True)
    return out.stdout + out.stderr


def main() -> int:
    import argparse
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--family", choices=("PIC16F87XA", "PIC18Fxx5x",
                                         "PIC16F193X"), default=None,
                    help="only this manifest family (the sharded CI jobs)")
    args = ap.parse_args()
    m = manifest_lib.load(manifest_lib.default_path())
    out_root = OUT_ROOT
    out_root.mkdir(parents=True, exist_ok=True)

    jobs = []
    for module_name in sorted(m.modules):
        module = m.modules[module_name]
        for family_name, mcus in sorted(module.supported.items()):
            if args.family is not None and family_name != args.family:
                continue
            for mcu in sorted(mcus):
                for variant in ("target", "sim"):
                    if variant == "sim" and m.family_of(mcu).name != family_name:
                        continue
                    sim = m.sim_variant_for(module_name, family_name)
                    if variant == "sim" and sim is None:
                        continue
                    src = epic_build.emit_config_source(
                        m, module_name, mcu, variant=variant)
                    if src is None:
                        continue
                    rel = pathlib.Path("build-sim") / "audit-config" / (
                        f"{module_name}__{variant}__{mcu}.c")
                    (REPO / rel).write_text(src)
                    fam = m.family_of(mcu)
                    jobs.append((module_name, mcu, variant, fam.dfp, str(rel)))

    # The links are independent docker runs; thread them (the runner's
    # cores) so the ~150 links do not serialize on docker's startup.
    workers = min(4, len(jobs))
    bad = 0
    with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as ex:
        results = ex.map(
            lambda j: (j[0], j[1], j[2], link_config_tu(j[1], j[3], j[4])),
            jobs)
        for module_name, mcu, variant, output in results:
            hits = UNKNOWN_KEY_RE.findall(output)
            if "1363" in output or "error" in output.lower():
                if hits:
                    print(f"config-key mismatch in {module_name} "
                          f"({mcu}, {variant}): "
                          + ", ".join(f"'{k}'" for k in hits))
                else:
                    print(f"config link failed in {module_name} "
                          f"({mcu}, {variant}):\n{output.strip()}")
                bad += 1

    if bad:
        print(f"config-key audit: {bad} example(s) with invalid config keys")
        return 1
    print(f"config-key audit: {len(jobs)} config TU(s) link clean for every "
          "supported MCU")
    return 0


if __name__ == "__main__":
    sys.exit(main())
