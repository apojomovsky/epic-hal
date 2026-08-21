# Experiment: epic-math hand asm vs XC8 native arithmetic

Status: 2026-08-12. Records a cycle benchmark comparing epic-math's
hand-written PIC16/PIC18 inline-asm math routines against XC8's native
arithmetic (compiler-generated code and runtime library routines) across
-O0..-O3, plus the follow-up study of whether the faster -O3 library
output could be learned from and adopted. Probe-based, reproducible via
the tools in this directory (see "Reproducing"). No product code changed;
the two algorithm transfers attempted in the follow-up were reverted
after deterministic measurements showed they did not help.

## Question

epic-math ships fixed-cycle hand-asm primitives (16-bit add/sub/mul/div,
32-bit mul/negate, BCD) for the PIC16F87XA and PIC18Fxx5x families. The
compiler's own arithmetic output varies with -O; the library routines
(__wmul, __lwdiv, __bmul) also differ between -O2 and -O3. How does the
hand asm compare at each level, and can the techniques behind the fastest
native output be adopted once, permanently, so the library is never
behind regardless of -O?

## Method

- Cycle counting via TMR1, which counts instruction cycles on the
  87XA/18Fxx5x. Each bench_* function runs N=100 iterations with TMR1
  snapshotted before and after, printing "<op> <hexdelta>" over UART.
  Per-op cycles = (delta - loop_empty) / N.
- MPLAB SIM (mdb) runs the firmware and captures the UART stream to a
  file (`set uart1io.uartioenabled true`, `set uart1io.output file`,
  `set uart1io.outputfile <path>`). mdb's stopwatch is broken
  (`stopwatch clear` always reports 0), so TMR1 is the counter.
- Two families: PIC16F877A (no hardware multiplier; both sides use
  software math) and PIC18F4550 (single-cycle MULWF).
- The simulator is deterministic: identical binaries reproduce identical
  raw deltas, so small differences are real, not noise.
- Operands: add/sub a=seed.., b=0x1357; mul8 b=0xCD; mul16 b=0xCDEF
  (bit 15 set, so the library's early-exit multiply cannot trigger);
  div16 den=0x0013 (so the library's normalized division runs 12 of 16
  iterations). Full sources: bench.c in this directory.
- Builds: "full" links the epic-math sources (the asm leaves are
  __at-pinned and only link at -O2/-O3, see the pin constraint below);
  "native" is the same firmware with BENCH_NATIVE_ONLY (no epic sources),
  which links at every -O.

## Results: cycles per op (N=100, loop baseline subtracted)

### PIC16F877A (no hardware multiplier)

| op        | epic asm | native -O0 | native -O1 | native -O2 | native -O3 |
|-----------|----------|------------|------------|------------|------------|
| add_u16   | 80       | 8          | 8          | 8          | 8          |
| sub_u16   | 75       | 8          | 8          | 8          | 8          |
| mul_u8    | 145      | 228        | 199        | 197        | 131        |
| mul_u16   | 302      | 394        | 342        | 340        | 234        |
| divmod_u16| 441      | 588        | 524        | 522        | 336        |

### PIC18F4550 (hardware multiplier)

| op        | epic asm | native -O0 | native -O1 | native -O2 | native -O3 |
|-----------|----------|------------|------------|------------|------------|
| add_u16   | 30       | 34         | 37         | 37         | 41         |
| sub_u16   | 29       | 32         | 32         | 32*        | 34*        |
| mul_u8    | 48       | 27         | 30         | 2          | ~0*        |
| mul_u16   | 55       | 49         | 46         | 13         | 15         |
| divmod_u16| 59       | 54         | 57         | 29         | 27         |

\* The op loop measured faster than the empty loop: the compiler folded
the constant operand into the loop's existing arithmetic, so the native
op is effectively free inlined.

## What the numbers say

- 87XA: the hand asm wins mul and div at the shipped -O2 (mul8 145 vs
  197, mul16 302 vs 340, div 441 vs 522). At -O3 the library routines
  pass it (131/234/336); the hand asm is -O-independent by design.
- 87XA add/sub: native inlined C is ~10x faster than calling the asm
  routine (8 vs 75-80). The cost is the function call plus the full
  carry-out contract, not the arithmetic.
- 18F4550: the hardware multiplier is decisive. Native inlined `mulwf`
  is 2 cycles for 8x8 and 13 for 16x16->16; the hand asm (MULWF-based,
  but through a call and computing 32-bit products) is 48-55. The asm
  add/sub win (30/29 vs 37/32) because the native add is not free there.
- The epic-math mul16/div numbers are the same at -O2 and -O3 (the asm
  is fixed); the apparent 87XA -O3 mul16_epic 489 reading is the caller
  loop restructuring around the call at -O3, not the routine.

## Follow-up: can the -O3 techniques be adopted?

XC8 v4 ships the runtime C source (pic/sources/c99/common/), so the
library routines were studied directly rather than reverse-engineered:

- __wmul (Umul16.c): early-exit shift-add, 16-bit result. The -O2 build
  already has the early exit; the -O3 win is scheduling (W-folding,
  carry fusion, tighter branches), not the algorithm.
- __lwdiv (lwdiv.c): normalized long division, 1+clz(den) iterations
  instead of a fixed 16.
- __bmul (Umul8.c): the __OPTIMIZE_SPEED__ (-O3) variant unrolls all 8
  steps.

Two transfers were implemented on the 87XA, verified correct
(instruction-level models: 0/500k div structure, 0/1M carry-idiom
combinations, 0/300k mul; plus the golden-vector mdb selftest, PASS=94),
then measured on the simulator and reverted:

- Normalized divmod_u16 (12 iterations instead of 16 for the bench
  divisor): +11 cycles/op. The extra control flow (quotient shift,
  divisor down-shift, restore path) costs more per iteration than the 4
  saved iterations.
- Early-exit mul_u16: +57 cycles/op. The exit test adds 5 instructions
  to every iteration, and the benchmark's multiplier (0xCDEF, bit 15
  set) never exits early; it pays the tax for nothing.

Both are deterministic regressions (identical raw deltas across runs).

## Conclusions

1. The hand asm beats the -O2 library routines everywhere it competes
   (mul/div on the 87XA). The -O2 library already uses the same
   algorithms (early exit, normalization) and is slower.
2. The remaining -O3 gap is compiler scheduling, which does not transfer
   to hand-written fixed asm: adopting the algorithms without the
   scheduling costs more than it saves. The library is not a source of
   adoptable techniques beyond what the asm already does.
3. The other native wins (8-cycle add, 2-cycle 18F mul) are inlined C
   with no routine to mimic. Bare arithmetic is already optimal in C;
   the hand-asm functions' cost is their richer contract (carry-out,
   remainder, 32-bit product) and the call. Callers who need only the
   bare result should use C operators, not the library.
4. The benchmark surfaced one real defect: the __at pins on the 87XA
   asm leaves were sized for -O2 codegen only, so full builds failed to
   link at -O0/-O1 (segment overlap error 596). Fixed by re-pinning on
   worst-case (-O0) sizes (PR #40), with no asm change.

## Reproducing

Inside the XC8 toolchain container, from the repo root:

    docker run --rm -v "$PWD":/work -w /work epic-hal-toolchain:local \
      sh docs/experiments/math-cycle-benchmark/matrix.sh

The matrix builds all 12 variants (2 families x full/native x -O levels)
and prints per-build "<op> <hexdelta>" lines captured from the simulator
UART. Per-op cycles = (delta - loop_empty) / 100. Functional
verification of the asm on the real target uses the epic-math
golden-vector selftest through the repo's mdb gate (scripts/
sim-mdb-run.sh), which must report PASS=94 FAIL=0.

## Tools in this directory

- bench.c: the firmware probe (TMR1 timing, UART reporting, N=100).
- matrix.sh: builds all variants in the container and runs the mdb
  captures.
- pic16dis.py: minimal PIC16 (14-bit) disassembler, used to decode the
  __wmul/__lwdiv instruction streams from the linked images when
  studying the -O2 vs -O3 library codegen.
