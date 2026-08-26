# Experiment: epic-math C path under epic-cc, cycles per op

Status: 2026-08-26. Records the cycle benchmark of epic-math's portable C
path (`src/host/` + `src/common/`) compiled by epic-cc on the PIC14
devices, the measurement epic-hal#90 exists to produce. Same probe shape,
same operands, same TMR1 method as the hand-asm benchmark in
`docs/experiments/math-cycle-benchmark/`, so the numbers line up directly
against that directory's tables.

## Question

epic-math ships fixed-cycle hand-asm primitives for PIC16F87XA and
PIC18Fxx5x, and a portable C implementation of the same four modules in
`src/host/`. Doc 31 says the right first move for HAL-3 is to build the C
path under epic-cc and measure it, not to port the assembly. How does the
C path under epic-cc compare to the hand asm, and does anything depend on
the fixed-cycle property the asm advertises?

## Method

- Same firmware shape as the hand-asm bench: TMR1 counts instruction
  cycles, each bench_* runs N iterations with TMR1 snapshotted before and
  after, printing "<op> <hexdelta>" over UART. Per-op cycles =
  (delta - loop_empty) / N.
- MPLAB SIM (mdb) runs the firmware and captures the UART stream to a
  file. The simulator is deterministic: identical binaries reproduce
  identical raw deltas.
- Operands identical to the hand-asm bench: add/sub a=seed.., b=0x1357;
  mul8 b=0xCD; mul16 b=0xCDEF (bit 15 set); div16 den=0x0013.
- Build: epic-cc (current master) compiles `bench_epiccc.c` plus the C
  path sources (`src/host/epic_math_{mul,div,addsub}.c`) in one
  whole-program invocation, `--target 16F877A` / `16F887`.
- N=50, not 100: the 16-bit TMR1 wraps at 65536, and the C-path mul16
  and div16 loops at N=100 exceed it (the N=100 mul16_epic delta 0x5018
  is 86924 mod 65536, a wrap artifact, not a fast multiply). N=50 keeps
  every delta under the wrap.
- The bench writes its seed at runtime: epic-cc has no RAM-initializer
  copy, so a `volatile uint16_t g_seed = 0x1357u` reads as 0 (the first
  probe run measured 0 * 0xCDEF = 0 and looked like a miscompile; it was
  the missing initializer, not the multiply).
- Op labels are numeric IDs, not strings: epic-cc does not yet lower
  const string tables reliably in multi-table layouts, and the label is
  not what is being measured. ID to op: 0 loop_empty, 1 add_native,
  2 add_epic, 3 sub_native, 4 sub_epic, 5 mul8_native, 6 mul8_epic,
  7 mul16_native, 8 mul16_epic, 9 div16_native, A div16_epic.

## Results: cycles per op (N=50, loop baseline subtracted)

### PIC16F877A and PIC16F887 (identical: same PIC14 core, same codegen)

| op        | epic asm (877A) | C path under epic-cc | native under epic-cc |
|-----------|-----------------|----------------------|----------------------|
| add_u16   | 80              | 153                  | 18                   |
| sub_u16   | 75              | 90                   | 18                   |
| mul_u8    | 145             | 423                  | 410                  |
| mul_u16   | 302             | 797                  | 497                  |
| divmod_u16| 441             | 1000                 | 485                  |

The hand-asm column is the 877A `-O2` column of the hand-asm benchmark's
table (the shipped build level). "native" is the same C operators
(`a + b`, `a * b`, `a / b`) compiled by epic-cc, the inlined-arithmetic
baseline.

## What the numbers say

1. The C path under epic-cc is correct: every op matches the host oracle
   on both devices (probe: add 0x1357+0xFFFF = 0x1356 carry 1, sub
   0x1357-0x1357 = 0 borrow 0, mul8 0x57*0xCD = 0x46CB, div
   0x1357/0x0013 = 0x00F9 rem 0x0004, mul16 0x1357*0xCDEF =
   0x0F8EB939).
2. The C path loses to the hand asm on every op: add 153 vs 80, sub 90
   vs 75, mul8 423 vs 145, mul16 797 vs 302, div 1000 vs 441. The gap is
   the function call plus the richer contract (carry-out, remainder,
   32-bit product), the same overhead the hand-asm bench measured for
   XC8 native calls; epic-cc's codegen does not close it.
3. The C path also loses to epic-cc's own inlined arithmetic (native):
   add 153 vs 18, sub 90 vs 18, mul8 423 vs 410, mul16 797 vs 497, div
   1000 vs 485. Callers who need only the bare result should use C
   operators, not the library, exactly as the hand-asm benchmark
   concluded for XC8.
4. The hand asm's fixed-cycle property is not load-bearing: no consumer
   in the shelf, the combo firmwares, or the demos depends on math-call
   cycle counts. The only epic-math consumers outside the module are
   epic-pid (mul_s16 in the control loop, speed not cycle-determinism)
   and the demos' example_math.c. Nothing times a math call.

## The fixed-cycle question, answered with evidence

The hand asm advertises fixed cycles per op. The C path is not
cycle-deterministic. The grep over the shelf, the combo firmwares, and
the demos found no timing assumption on any math call: no ISR calls a
math primitive, no consumer measures a math call's duration, and the
cycle benchmark itself is the only place math-call cycles are read. The
fixed-cycle property is a property of the asm implementation, not a
contract any consumer relies on.

## Build gaps found on the full C path

The four leaf modules (mul, div, addsub, bcd) build and run correctly
under epic-cc. Two of the derived common modules do not compile yet:

- `epic_math_sqrt.c` passes `NULL` as the `ok` out-param of
  `epic_math_divmod_u16`; irparse panics on a `ptr null` call argument
  ("call arg must carry a value").
- `epic_math_rand.c`'s LFSR rotate lowers to `llvm.fshl.i16`, which
  legalize does not know ("unknown intrinsic").

Both are epic-cc gaps, not C-path defects: the same sources compile and
pass their exhaustive host tests under gcc. They do not affect the
measured ops (mul/div/addsub are the leaf primitives the benchmark
covers).

## Reproducing

From the repo root, with the epic-cc dev image and the hal toolchain
image available:

    # build (epic-cc dev image, clang env as in the Makefile's epiccc-build)
    docker run --rm -v "$PWD":/repo -w /repo \
      -e PIC8_CLANG_UNWRAPPED=/opt/clang/bin/clang \
      -e PIC8_CLANG_RESOURCE_DIR=/opt/clang/lib/clang/20 \
      epic-cc-dev:local /tmp/cargo-target/release/epic-cc --target 16F877A \
      -I epic-math/include -I docs/experiments/math-cycle-benchmark-epiccc \
      -D PIC16F877A -D FOSC_HZ=20000000 -D __EPIC_CC__ \
      -o build-sim/bench-epiccc/87-cpath.hex \
      docs/experiments/math-cycle-benchmark-epiccc/bench_epiccc.c \
      epic-math/src/host/epic_math_mul.c epic-math/src/host/epic_math_div.c \
      epic-math/src/host/epic_math_addsub.c

    # measure (hal toolchain image, mdb UART capture)
    docker run --rm -v "$PWD":/repo -w /repo epic-hal-toolchain:local \
      mdb.sh /repo/docs/experiments/math-cycle-benchmark-epiccc/cap.mdb

`cap.mdb` programs `build-sim/bench-epiccc/87-cpath.hex` under MPLAB SIM
and captures the UART stream to `build-sim/bench-epiccc/out-87.txt`.
Per-op cycles = (delta - loop_empty) / 50.

## Tools in this directory

- bench_epiccc.c: the firmware probe (TMR1 timing, UART reporting, N=50,
  numeric op IDs).
- cap.mdb: the mdb capture script (device, UART to file, run, halt).
