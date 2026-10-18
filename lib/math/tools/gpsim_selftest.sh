#!/bin/sh
# tools/gpsim_selftest.sh: Tier 2 gpsim regression for the PIC16 asm backend.
#
# Loads the PIC16 self-test hex under gpsim, captures the firmware USART
# report, and exits nonzero on any golden-vector mismatch. The firmware
# (tests/target_selftest.c) replays golden_vectors.h on the simulated core
# and prints per-vector FAIL lines plus a PASS=/FAIL=/RESULT summary, so
# the verdict here is a strict parse of that report, never gpsim's own
# exit status. gpsim is PIC16-only and absent from the toolchain image, so
# a missing gpsim skips cleanly (exit 0); a present gpsim with no hex to
# load is an error (exit 1), so this gate can never pass by skipping work.
#
# Run: tools/gpsim_selftest.sh [hex]
# Env: GPSIM_HEX (same as [hex]), GPSIM_PROCESSOR (default p16f877a),
#   GPSIM_STEPS (default 5000000), GPSIM_CMDFILE (custom .stc, skips the
#   generated USART-tap script below).
#
# Driving core follows the gpsim manual CLI: -i (CLI only), -p (processor),
# -c (command file). The generated script taps firmware TX with the
# documented usart module (module/node/attach), bounds the run with step,
# then quits. gpsim was not installable where this was written, so the tap
# is manual-faithful but sim-unverified; any capture failure lands in the
# strict verdict below as a nonzero exit, never a silent pass.

set -e

MODDIR=$(dirname "$0")/..
: "${GPSIM_PROCESSOR:=p16f877a}"
: "${GPSIM_STEPS:=5000000}"

if ! command -v gpsim >/dev/null 2>&1; then
    echo "gpsim_selftest: SKIP (gpsim not installed; Tier 2 is optional/best-effort)."
    echo "  PIC16 asm correctness rests on Tier 1 (exhaustive host tests) + the asm"
    echo "  build + the hand-traces. See docs/ARCHITECTURE.md 'Testing tiers'."
    exit 0
fi

HEX=${1:-${GPSIM_HEX:-}}
if [ -z "$HEX" ]; then
    HEX=$(find "$MODDIR" -name '*.hex' -print 2>/dev/null | sort | head -n 1 || true)
fi
if [ -z "$HEX" ] || [ ! -f "$HEX" ]; then
    echo "gpsim_selftest: FAIL (gpsim present but no PIC16 self-test hex found)."
    echo "  Build one (see docs/ARCHITECTURE.md 'Testing tiers') and pass its path:"
    echo "  tools/gpsim_selftest.sh <hex>, or GPSIM_HEX=<hex>."
    exit 1
fi

TMPD=$(mktemp -d)
trap 'rm -rf "$TMPD"' EXIT INT TERM
STC=${GPSIM_CMDFILE:-"$TMPD/tap.stc"}
if [ -z "${GPSIM_CMDFILE:-}" ]; then
    # USART tap per the gpsim manual attach example: firmware TX (RC6 on
    # the 16F877A) into a usart module instance. step bounds the run so the
    # script always terminates on its own; override via GPSIM_CMDFILE.
    cat > "$STC" <<EOF
module load usart U1
node uart_tx
attach uart_tx portc6 U1.RXPIN
step $GPSIM_STEPS
quit
EOF
fi

echo "gpsim_selftest: driving $HEX under gpsim ($GPSIM_PROCESSOR)..."
if command -v timeout >/dev/null 2>&1; then
    timeout 600 gpsim -i -p "$GPSIM_PROCESSOR" -c "$STC" "$HEX" > "$TMPD/xcript" 2>&1 || true
else
    gpsim -i -p "$GPSIM_PROCESSOR" -c "$STC" "$HEX" > "$TMPD/xcript" 2>&1 || true
fi

T=$TMPD/xcript
BAD=0
if ! grep -q 'epic-math target self-test' "$T"; then
    echo "gpsim_selftest: FAIL (no firmware banner in gpsim output; tap captured nothing)."
    head -n 20 "$T"
    BAD=1
fi
if grep -q 'FAIL .* got=' "$T"; then
    echo "gpsim_selftest: FAIL (vector mismatches):"
    grep 'FAIL .* got=' "$T"
    BAD=1
fi
# FAIL= must read 0 and PASS= must be positive, so a truncated log that
# happens to carry the banner still cannot pass.
NF=$(grep 'FAIL=[0-9]' "$T" | head -n 1 | sed -n 's/^.*FAIL=\([0-9][0-9]*\).*$/\1/p' || true)
NP=$(grep 'PASS=[0-9]' "$T" | head -n 1 | sed -n 's/^.*PASS=\([0-9][0-9]*\).*$/\1/p' || true)
case "${NF:-missing}" in
    0) ;;
    *) echo "gpsim_selftest: FAIL (summary FAIL count is '${NF:-missing}', want 0)."; BAD=1;;
esac
case "${NP:-missing}" in
    *[!0-9]*|0) echo "gpsim_selftest: FAIL (summary PASS count is '${NP:-missing}')."; BAD=1;;
esac
if ! grep -q 'RESULT: ALL PASS' "$T"; then
    echo "gpsim_selftest: FAIL (missing 'RESULT: ALL PASS')."
    BAD=1
fi
if [ "$BAD" -eq 0 ]; then
    echo "gpsim_selftest: PASS (PASS=$NP FAIL=0 under gpsim)."
    exit 0
fi
exit 1
