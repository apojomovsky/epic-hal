#!/bin/sh
# tools/gpsim_selftest.sh -- Tier 2: optional gpsim regression for the PIC16
# inline-asm backend.
#
# gpsim (FOSS, supports the PIC16F87x family this repo targets) can load the
# real compiled hex, poke input registers, step, and read output registers
# headlessly. This script is the plan's optional/best-effort Tier 2: it
# feature-detects gpsim and, if present, would script it against
# tests/golden_vectors.h so the PIC16 inline asm itself (not just the host
# reference) is proven against the same vectors Tier 1 uses.
#
# gpsim is NOT installed in this environment and there is no passwordless
# sudo to install it, so this script skips cleanly (exit 0) with a message
# when gpsim is absent -- no build or CI step depends on it being present.
#
# Run:  tools/gpsim_selftest.sh
#
# If you have gpsim installed, flesh out the body below to drive it against
# the PIC16 self-test hex (mcu/pic16f87xa-math-mplabx/build/*.hex) and the
# golden vectors.

set -e

if ! command -v gpsim >/dev/null 2>&1; then
    echo "gpsim_selftest: SKIP (gpsim not installed; Tier 2 is optional/best-effort)."
    echo "  PIC16 asm correctness rests on Tier 1 (exhaustive host tests) + the asm"
    echo "  build + the hand-traces. See docs/ARCHITECTURE.md 'Testing tiers'."
    exit 0
fi

echo "gpsim_selftest: gpsim present -- driving PIC16 asm against golden_vectors.h..."
# TODO: drive gpsim headlessly against the PIC16 self-test hex, replaying
# golden_vectors.h and comparing the asm output to the expected values.
# gpsim does not support PIC18, so this is PIC16-only.
echo "gpsim_selftest: not yet implemented for this environment (gpsim absent)."
exit 0