#!/usr/bin/env bash
# Run a module's pre-emitted HARNESS=sim build under MPLAB SIM (mdb) and check
# the EPIC_HARNESS_RESULT marker. Shared by ci.yml's target job, family-check.yml,
# and scripts/sim-test-local.sh. Container-only (xc8-cc + mdb.sh, no python3): the
# pre-emitted build-sim/<module>/<mcu>/build.sh from `epic_build.py build --variant sim`.
#
# Usage: sim-mdb-run.sh <family> <mcu> <device> <module> [wait_ms] [mode] [extra_mdb] [eeprom_writes]
#   wait_ms=2000 wall-clock (SIM slower than real time); mode=uart|gpio|toggle
#   (gpio: PORTA bit 0, pic16f193x only; toggle: see below);
#   extra_mdb: commands before `quit`;
#   eeprom_writes=0: SIM never completes a CPU-executed EEPROM write; each cycle
#   halts, clears WR, replays the EECON2 unlock, re-asserts WR. Keep >= the
#   scenario's count; extra cycles can re-run stateful firmware.

set -euo pipefail

family="$1"; mcu="$2"; device="$3"; module="$4"
wait_ms="${5:-2000}"; mode="${6:-uart}"; extra_mdb="${7:-}"
eeprom_writes="${8:-0}"
# PIC18 EECON1/EECON2 Access-Bank addresses (PIC18F4550); override for
# other families if a future gate needs them.
eeprom_econ1_addr="${EECON1_ADDR:-0xFA6}"
eeprom_econ2_addr="${EECON2_ADDR:-0xFA7}"

if [ "$mode" != "uart" ] && [ "$mode" != "gpio" ] && [ "$mode" != "toggle" ]; then
  echo "::error::mode must be 'uart', 'gpio' or 'toggle', got '$mode'" >&2
  exit 1
fi

# toggle mode: sample one pin across fixed instruction counts and require it to
# change, the "does this firmware actually blink" gate for a build with no UART
# harness. `stepi` rather than run+wait because it is reproducible: MPLAB SIM
# advances Timer0 across stepi on the PIC16F88X, so twelve samples of 200k
# instructions give a bit-identical sequence run to run, where wall-clock `wait`
# varies with machine speed.
toggle_reg="${TOGGLE_REG:-PORTB}"
toggle_bit="${TOGGLE_BIT:-0}"
toggle_samples="${TOGGLE_SAMPLES:-12}"
toggle_stepi="${TOGGLE_STEPI:-200000}"

build_dir="build-sim/${module}"
objdir="${build_dir}/${mcu}"
script="${objdir}/build.sh"
# SIM_MDB_SKIP_BUILD: the hex is already built and this container cannot rebuild
# it. That is the epic-cc path, whose compiler and clang live in the epic-cc
# image, not this one; the caller builds there and only the mdb run happens here.
if [ "${SIM_MDB_SKIP_BUILD:-0}" = "1" ]; then
  echo "SIM_MDB_SKIP_BUILD=1: using the existing hex under ${build_dir}"
else
  if [ ! -f "$script" ]; then
    echo "::error::no emitted build script at ${script}." \
         "Run 'python3 scripts/epic_build.py build --module ${module}" \
         "--mcu ${mcu} --variant sim --build-dir ${build_dir}'" \
         "first (needs python3, so outside this container)." >&2
    exit 1
  fi
  rm -f "$objdir"/*.p1
  EPIC_REPO_ROOT="$PWD" sh "$script"
fi

hexes=("$build_dir"/"$mcu"-*.hex)
if [ ! -e "${hexes[0]}" ] || [ "${#hexes[@]}" -ne 1 ]; then
  echo "::error::expected exactly one ${build_dir}/${mcu}-*.hex, found: ${hexes[*]}" >&2
  exit 1
fi
hex="${hexes[0]}"

# Each cycle: let the target run until it stalls on an EEPROM write,
# halt, and complete the write through the debugger (see the
# eeprom_writes argument's comment). Emitted before the final
# run/wait/halt so a gate that blocks on EEIF gets every write done.
eeprom_cycles=""
i=0
while [ "$i" -lt "$eeprom_writes" ]; do
  eeprom_cycles="${eeprom_cycles}
run
wait 500
halt
write /r ${eeprom_econ1_addr} 0x04
write /r ${eeprom_econ2_addr} 0x55
write /r ${eeprom_econ2_addr} 0xAA
write /r ${eeprom_econ1_addr} 0x06"
  i=$((i + 1))
done

if [ "$mode" = "toggle" ]; then
  out="/tmp/${family}-toggle-$$.txt"
  rm -f "$out"
  mdb_script="/tmp/${family}-mdb-$$.txt"
  {
    echo "device ${device}"
    echo "hwtool SIM"
    echo "program ${hex}"
    i=0
    while [ "$i" -lt "$toggle_samples" ]; do
      echo "stepi ${toggle_stepi}"
      echo "print ${toggle_reg}"
      i=$((i + 1))
    done
    [ -n "$extra_mdb" ] && echo "${extra_mdb}"
    echo "quit"
  } > "$mdb_script"
elif [ "$mode" = "gpio" ]; then
  out="/tmp/${family}-gpio-$$.txt"
  rm -f "$out"
  mdb_script="/tmp/${family}-mdb-$$.txt"
  cat > "$mdb_script" <<SCRIPT
device ${device}
hwtool SIM
program ${hex}
${eeprom_cycles}
run
wait ${wait_ms}
halt
print PORTA
${extra_mdb}
quit
SCRIPT
else
  out="/tmp/${family}-uart-$$.txt"
  rm -f "$out"
  mdb_script="/tmp/${family}-mdb-$$.txt"
  cat > "$mdb_script" <<SCRIPT
device ${device}
set uart1io.uartioenabled true
set uart1io.output file
set uart1io.outputfile ${out}
hwtool SIM
program ${hex}
${eeprom_cycles}
run
wait ${wait_ms}
halt
${extra_mdb}
quit
SCRIPT
fi

mdb_ok=1
if [ "$mode" = "gpio" ] || [ "$mode" = "toggle" ]; then
  mdb.sh "$mdb_script" > "$out" 2>&1 || mdb_ok=0
else
  mdb.sh "$mdb_script" || mdb_ok=0
fi

if [ "$mode" = "toggle" ]; then
  echo "---- captured ${toggle_reg} samples ----"
  cat "$out" 2>/dev/null || echo "(no output file produced)"
  echo "----------------------------------"

  if [ "$mdb_ok" -ne 1 ]; then
    echo "::error::mdb.sh itself exited non-zero"
    exit 1
  fi
  bits=""
  transitions=0
  prev=""
  while read -r raw; do
    byte=$((raw))
    bit=$(((byte >> toggle_bit) & 1))
    bits="${bits}${bit}"
    if [ -n "$prev" ] && [ "$bit" -ne "$prev" ]; then
      transitions=$((transitions + 1))
    fi
    prev="$bit"
  done < <(grep -E "^${toggle_reg}[[:space:]]*=" "$out" |
           sed -E 's/.*=[[:space:]]*(0x)?([0-9A-Fa-f]+).*/\2/')

  echo "${toggle_reg} bit ${toggle_bit} across ${#bits} samples: ${bits}"
  if [ "${#bits}" -lt 2 ]; then
    echo "::error::expected ${toggle_samples} ${toggle_reg} samples, got ${#bits}" \
         "(simulator may have errored before the first sample)"
    exit 1
  fi
  if [ "$transitions" -lt 1 ]; then
    echo "::error::${toggle_reg} bit ${toggle_bit} never changed across" \
         "${#bits} samples; the firmware is not toggling it"
    exit 1
  fi
  echo "PASS (${transitions} transitions)"
  exit 0
fi

if [ "$mode" = "gpio" ]; then
  echo "---- captured PORTA readback ----"
  cat "$out" 2>/dev/null || echo "(no output file produced)"
  echo "----------------------------------"

  if [ "$mdb_ok" -ne 1 ]; then
    echo "::error::mdb.sh itself exited non-zero"
    exit 1
  fi
  if [ ! -s "$out" ]; then
    echo "::error::mdb produced no PORTA output at all (simulator" \
         "may have errored, or the harness never reached report())"
    exit 1
  fi
  # Read the byte from the captured output. mdb's `print <REG>` outputs
  # "<REG>=NN" (decimal) by default, or "=0xNN" if hex mode is set;
  # accept either form. Strip any 0x prefix, parse as decimal (mdb's
  # default form), and check bit 0.
  por_line=$(grep -E 'PORTA\s*=' "$out" | tail -n 1 | sed -E 's/.*=\s*(0x)?([0-9A-Fa-f]+).*/\2/')
  if [ -z "$por_line" ]; then
    echo "::error::no PORTA value found in mdb output"
    exit 1
  fi
  # Decimal parse (mdb's default).
  por_byte=$((por_line))
  if [ $((por_byte & 0x01)) -ne 0 ]; then
    echo "PASS marker found (PORTA bit 0 set, byte=${por_byte})"
    exit 0
  fi
  echo "::error::PORTA bit 0 was 0 after halt (FAIL marker)"
  exit 1
fi

# uart mode (existing behavior preserved exactly)
echo "---- captured UART output ----"
cat "$out" 2>/dev/null || echo "(no output file produced)"
echo "-------------------------------"

if [ "$mdb_ok" -ne 1 ]; then
  echo "::error::mdb.sh itself exited non-zero"
  exit 1
fi
if [ ! -s "$out" ]; then
  echo "::error::mdb produced no UART output at all (simulator" \
       "may have errored, or the harness never reached report())"
  exit 1
fi
if grep -q "EPIC_HARNESS_RESULT: FAIL" "$out"; then
  echo "::error::sim-target harness reported FAIL"
  exit 1
fi
if ! grep -q "EPIC_HARNESS_RESULT: PASS" "$out"; then
  echo "::error::no PASS/FAIL marker found in captured UART output"
  exit 1
fi
echo "PASS marker found"
