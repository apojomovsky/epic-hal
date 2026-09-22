#!/usr/bin/env python3
"""Epic-cc slice audit: every family whose manifest declares an
``epiccc_sources`` slice must (a) carry the ``include/epiccc/`` platform
header its epic-cc include path resolves to, and (b) have an epiccc-gate
leg in ci.yml, so a declared slice that cannot compile fails CI instead
of waiting for a ticket to trip over it (epic-hal#257). Pure text, no
toolchain: reads the manifest, the family trees and ci.yml. Runs in CI's
family-check audit step (``--family``) and ``make audit`` (repo-wide).
Exit 0 = no gaps.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
import epicmanifest as manifest_lib  # noqa: E402

CI_WORKFLOW = REPO / ".github" / "workflows" / "ci.yml"

# MODULE=<hal dir> MCU=<part> legs of the epiccc-gate job. The scan is
# scoped to that job's block: a `make epiccc-build` elsewhere in ci.yml
# (a reused emit step, say) is not a watched gate leg.
LEG_RE = re.compile(r"make epiccc-build MODULE=(\S+) MCU=(\S+)")
JOB_RE = re.compile(r"^  ([a-z0-9-]+):\s*$", re.M)


def ci_leg_mcus() -> set[str]:
    text = CI_WORKFLOW.read_text(encoding="utf-8")
    start = text.index("  epiccc-gate:")
    nxt = JOB_RE.search(text, start + len("  epiccc-gate:"))
    block = text[start:nxt.start()] if nxt else text[start:]
    return {m.group(2) for m in LEG_RE.finditer(block)}


def audit_family(fam, gaps: list[str]) -> None:
    if not fam.epiccc_sources:
        return
    hal = pathlib.Path(fam.hal_dir)
    # The platform header's name is family convention, not contract: the
    # PIC14-side families mirror their target header (<family>_platform.h),
    # the 18Fxx5x ships a bare pic18_platform.h. Existence of the swapped
    # include dir's header is the contract; the build resolves the name.
    epiccc_dir = hal / "include" / "epiccc"
    headers = list(epiccc_dir.glob("*.h")) if epiccc_dir.is_dir() else []
    if not headers:
        gaps.append(
            f"families.{fam.name} declares an epiccc_sources slice but {epiccc_dir} "
            "carries no platform header: the epic-cc include path swaps include/target "
            "for include/epiccc and the family umbrella include fails at the platform "
            "header (epic-hal#257)"
        )
    # A leg watches the family when its MCU is one of the family's
    # variants, whatever module the leg names (the 193X slice builds
    # through the epic-pic16f193x-firmware module, not a pic16f193x-hal
    # MODULE).
    if not set(fam.variants) & ci_leg_mcus():
        gaps.append(
            f"families.{fam.name} declares an epiccc_sources slice but ci.yml's epiccc-gate "
            f"job has no leg on any of its variants ({', '.join(fam.variants)}): "
            "the slice's compile is unwatched (epic-hal#257)"
        )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--family", default=None,
                        help="audit one family (family-check passes the job's family)")
    args = parser.parse_args()

    m = manifest_lib.load(manifest_lib.default_path())
    gaps: list[str] = []
    for fam in m.families.values():
        if args.family and fam.name != args.family:
            continue
        audit_family(fam, gaps)

    for gap in gaps:
        print(f"epiccc-slice-audit: {gap}", file=sys.stderr)
    if gaps:
        return 1
    print("epiccc-slice-audit: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
