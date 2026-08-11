#!/usr/bin/env bash
# Run a module's pre-emitted HARNESS=sim build under MPLAB SIM (mdb) and check
# the EPIC_HARNESS_RESULT marker. Shared by ci.yml's target job, family-check.yml,
# and scripts/sim-test-local.sh. Container-only (xc8-cc + mdb.sh, no python3): the
# pre-emitted build-sim/<module>/<mcu>/build.sh from `epic_build.py build --variant sim`.
#
# Usage: sim-mdb-run.sh <family> <mcu> <device> <module> [wait_ms] [mode] [extra_mdb] [eeprom_writes]
#   wait_ms=2000 wall-clock (SIM slower than real time); mode=uart|gpio (gpio:
#   PORTA bit 0, pic16f193x only); extra_mdb: commands before `quit`;
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

if [ "$mode" != "uart" ] && [ "$mode" != "gpio" ]; then
  echo "::error::mode must be 'uart' or 'gpio', got '$mode'" >&2
  exit 1
fi

build_dir="build-sim/${module}"
objdir="${build_dir}/${mcu}"
script="${objdir}/build.sh"
if [ ! -f "$script" ]; then
  echo "::error::no emitted build script at ${script}." \
       "Run 'python3 scripts/epic_build.py build --module ${module}" \
       "--mcu ${mcu} --variant sim --build-dir ${build_dir}'" \
       "first (needs python3, so outside this container)." >&2
  exit 1
fi
rm -f "$objdir"/*.p1
EPIC_REPO_ROOT="$PWD" sh "$script"

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

if [ "$mode" = "gpio" ]; then
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
if [ "$mode" = "gpio" ]; then
  mdb.sh "$mdb_script" > "$out" 2>&1 || mdb_ok=0
else
  mdb.sh "$mdb_script" || mdb_ok=0
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
