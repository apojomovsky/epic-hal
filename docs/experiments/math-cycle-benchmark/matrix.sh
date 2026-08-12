#!/bin/bash
# Builds and measures the cycle benchmark across families and -O levels.
# The firmware (bench.c) times each op with TMR1 over N=100 iterations and
# prints "<op> <hexdelta>" over UART; per-op cycles = (delta - loop_empty)/N.
#
# Run inside the XC8 toolchain container, from the repo root:
#   docker run --rm -v "$PWD":/work -w /work pic8-hal-toolchain:local \
#     sh docs/experiments/math-cycle-benchmark/matrix.sh
# Output per build is a "### <name>" header followed by the raw <op> <hexdelta>
# lines captured from the simulator's UART. See README.md in this directory.
set -u
cd "$(dirname "$0")/../../.."         # repo root inside the container
DFP87=/opt/microchip/xc8/v4.00/pic/packs/Microchip.PIC16Fxxx_DFP/xc8
DFP18=/opt/microchip/xc8/v4.00/pic/packs/Microchip.PIC18Fxxxx_DFP/xc8
BENCH=docs/experiments/math-cycle-benchmark/bench.c
OUT=build-sim/bench-cycles
mkdir -p "$OUT"
OPS="loop_empty add_native add_epic sub_native sub_epic mul8_native mul8_epic"
OPS="$OPS mul16_native mul16_epic div16_native div16_epic"

run_measure() { # device outfile hex
  rm -f "$2"
  cat > "$OUT/cap.mdb" <<EOF
device $1
set uart1io.uartioenabled true
set uart1io.output file
set uart1io.outputfile $PWD/$2
hwtool SIM
program $PWD/$3
run
wait 4000
halt
quit
EOF
  mdb.sh "$OUT/cap.mdb" > /dev/null 2>&1
}

measure() { # name device hex
  local out="$OUT/out-$(basename "$3").txt"
  run_measure "$2" "$out" "$3"
  echo "### $1"
  grep -E "^($(echo $OPS | tr ' ' '|')) " "$out" | tail -12
}

build() { # mcu dfp opt native(0|1) extra_srcs out  (extra_srcs may be empty)
  local mcu="$1" dfp="$2" opt="$3" native="$4" extra="$5" out="$6"
  local nf=""; [ "$native" = "1" ] && nf="-DBENCH_NATIVE_ONLY"
  # shellcheck disable=SC2086
  xc8-cc -mcpu=$mcu -DPIC$mcu -O$opt -std=c99 -mdfp=$dfp \
    -Iepic-math/include -Iepic-common/include \
    $nf $extra $BENCH -o "$OUT/$out" -ginhx32 2>/dev/null
}

# PIC16F877A: no hardware multiplier; both sides use software math.
for opt in 2 3; do
  build 16F877A "$DFP87" "$opt" 0 "epic-math/src/pic16/*.c epic-math/src/common/*.c" "87-full-o$opt.hex"
  measure "87XA full -O$opt" PIC16F877A "$OUT/87-full-o$opt.hex"
done
for opt in 0 1 2 3; do
  build 16F877A "$DFP87" "$opt" 1 "" "87-native-o$opt.hex"
  measure "87XA native -O$opt" PIC16F877A "$OUT/87-native-o$opt.hex"
done
# PIC18F4550: single-cycle MULWF; XC8 inlines 8x8 and 16x16->16 products.
for opt in 2 3; do
  build 18F4550 "$DFP18" "$opt" 0 "epic-math/src/pic18/*.c epic-math/src/common/*.c" "18-full-o$opt.hex"
  measure "18F4550 full -O$opt" PIC18F4550 "$OUT/18-full-o$opt.hex"
done
for opt in 0 1 2 3; do
  build 18F4550 "$DFP18" "$opt" 1 "" "18-native-o$opt.hex"
  measure "18F4550 native -O$opt" PIC18F4550 "$OUT/18-native-o$opt.hex"
done
