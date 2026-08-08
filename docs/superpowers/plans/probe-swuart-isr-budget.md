# Probe: does a 4x-oversample tick fit the ISR cycle budget on PIC16F87XA?

Verdict, **array-of-structs probe (probe.c)**: **NO**. 299 cycles,
2.3x over the N=4 budget and 1.7x over N=3.

Verdict, **straight-line two-named-channel probe (probe2.c, follow-up)**:
**YES for N=4**, with a slim margin. Measured worst case 122 cycles
against a 130-cycle budget, roughly 6% headroom. See "Follow-up probe"
below for the full measurement; this is the number Task 2 should treat
as authoritative, since it's the shape closer to what a real
compile-time-bounded 2-channel implementation looks like. The
array-of-structs probe's failure was real but was driven by two
avoidable properties (a non-unrolled loop and a runtime multiply for
array indexing) that a hand-written 2-channel module has no reason to
have. Task 2 can use N=4, but the margin is tight enough that it
should not add materially more per-tick work than this probe's shape
without re-measuring.

Run on 2026-08-06 against XC8 v3.10 (native,
`/opt/microchip/xc8/v3.10/bin`), `-mcpu=16f877a -O2 -std=c99`. Codegen
cross-checked byte-for-byte against XC8 v4.00 (Docker
`pic8-hal-toolchain:local`, the version CI actually pins): identical
226-word program, identical opcodes. Cycle count independently verified
two ways: (1) hand-trace of the `.lst` control flow, (2) an actual
MPLAB SIM run under `mdb` reading the hardware stopwatch. Both agree
to within a few cycles (data-dependent branch taken differed slightly
between the two, expected and immaterial to the verdict).

## What was run

### Step 1: probe code (one deviation from the brief)

`/tmp/swuart-probe/probe.c`, copied from the brief verbatim except for
one line: `LATB` does not exist on classic PIC16F87XA (no LATx
registers, that peripheral first shows up on PIC18/Enhanced Mid-range,
per this repo's own banking notes). Replaced both `LATB` references
with `PORTB`, which is what a real PIC16F87XA driver uses for output
on this family. No other change; the rest is the brief's file exactly:
two-channel `chan_t` array, TMR1 ISR, worst-case branch shape (TX
shift+drive and RX sample+shift each on tick expiry, both channels).

### Step 2: compile and get a listing with real addresses

```sh
mkdir -p /tmp/swuart-probe && cd /tmp/swuart-probe
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
xc8-cc -mcpu=16f877a \
  -mdfp=/opt/microchip/mplabx/v6.35/packs/Microchip/PIC16Fxxx_DFP/1.8.167/xc8 \
  -O2 -std=c99 -fasmfile -Wa,-a probe.c -o probe.hex -ginhx32
```

Two adjustments from the brief's suggested command were needed:
- `-mdfp=<dir>` is mandatory (XC8 v3.10/v4.00 both refuse to compile
  without a device family pack); the path ends in `/xc8`, confirmed by
  reading how this repo's own generated example project
  (`examples/epicurus-demo-pic16f87xa.X/nbproject/Makefile-default.mk`)
  invokes it.
- XC8 v3.10 does not emit a `probe.c.lst`; the real address-annotated
  listing is `probe.lst`, produced only with `-fasmfile -Wa,-a` (same
  flags the repo's Makefile uses). Without `-Wa,-a` you only get
  `probe.s`, the pre-assembly output, whose macros (`ljmp`, `fcall`,
  `clrc`, `skipc`) don't show their real per-instruction word counts.

Output (trimmed to the memory summary):

```
16F877A Memory Summary:
    Program space        used    E2h (   226) of  2000h words   (  2.8%)
    Data space           used    25h (    37) of   170h bytes   ( 10.1%)
    EEPROM space         used     0h (     0) of   100h bytes   (  0.0%)
    Configuration bits   used     1h (     1) of     1h word    (100.0%)
    ID Location space    used     0h (     0) of     4h bytes   (  0.0%)
```

Cross-check against XC8 v4.00 (Docker):

```sh
docker run --rm -v /tmp/swuart-probe-v4:/work -w /work pic8-hal-toolchain:local bash -c '
  export PATH=$PATH:/opt/microchip/xc8/v4.00/bin
  xc8-cc -mcpu=16f877a \
    -mdfp=/opt/microchip/xc8/v4.00/pic/packs/Microchip.PIC16Fxxx_DFP/xc8 \
    -O2 -std=c99 -fasmfile -Wa,-a probe.c -o probe.hex -ginhx32
'
diff <(grep -oP '^\s*\d+\s+[0-9A-F]{4}\s+\K[0-9A-F ]+' /tmp/swuart-probe/probe.lst) \
     <(grep -oP '^\s*\d+\s+[0-9A-F]{4}\s+\K[0-9A-F ]+' /tmp/swuart-probe-v4/probe.lst)
```

Output: same `226`-word memory summary; `diff` of the opcode columns
produced no output (exit 0, byte-identical). The result below is not a
v3.10 quirk.

### Step 3: why a naive static line count from the .lst is wrong here

The brief's suggested check (`grep -c` addressed lines between `_isr:`
and `retfie`) gives the *static* size of the ISR's own code block:
address `0x0004` (interrupt vector trampoline) through `0x00C8`
(`retfie`) is `0x00C8 - 0x0004 + 1 = 197` words. That number is
**not** the answer to the question asked, for two reasons visible in
`probe.lst`:

1. The `for (uint8_t i = 0; i < 2; i++)` loop is not unrolled at `-O2`;
   the compiler emits one copy of the loop body and a real
   `goto`-back-to-top, so the static 197 words execute the loop body
   only once in the listing but **twice** at runtime (once per
   channel).
2. `&g_chan[i]` (a 2-element array of a 7-byte struct) is not
   strength-reduced to a compile-time offset; XC8 emits a call to the
   generic 8-bit multiply routine `___bmul` (`probe.lst:647`), which
   lives in a separate `psect` outside the `0x0004`-`0x00C8` range and
   is **called twice** (once per loop iteration), each call costing
   real cycles the static isr-block count never sees.

So the real per-invocation instruction/cycle count has to be a
*dynamic* trace (which branches actually execute, how many times),
not a listing address diff. Ran both a manual trace and a simulator
measurement to cross-check.

### Step 4: manual dynamic trace (worst case: both channels' TX and RX
mid-tick-expiry)

Traced `probe.lst` address-by-address for the path where
`PIR1bits.TMR1IF` is set and every `*_ticks_left` is 0 (forces the
heavier "shift a bit / sample a bit" branch on both TX and RX, for
both `i=0` and `i=1`), applying standard PIC16 timing (`GOTO`/`CALL`/
`RETURN`/`RETFIE` = 2 cycles; a conditional skip = 2 cycles when it
skips, 1 when it doesn't; everything else = 1 cycle):

| segment | words | cycles |
|---|---|---|
| interrupt vector trampoline + ISR header (flag check, TMR1 reload, `i=0`) | 21 | 23 |
| loop iteration `i=0` (incl. `___bmul` call site + its body) | 121 | 133 |
| loop iteration `i=1` (incl. `___bmul` call site + its body) | 121 | 135 |
| epilogue (context restore + `retfie`) | 11 | 12 |
| **total** | **274** | **303** |

The `___bmul` cost buried inside each loop-iteration row above, itemized
for traceability: the call site itself (2 `PCLATH` setup instructions +
the `CALL` + 2 `PCLATH` restore instructions) is 5 words / 6 cycles per
call; the `___bmul` body (Step 3's disassembly) is 12 words / 16 cycles
for the `i=0` call (multiplier 0) and 13 words / 16 cycles for the
`i=1` call (multiplier 1). Total attributable to the multiply across
both calls: `(6 + 16) x 2 = 44 cycles`, `(5 + 12) + (5 + 13) = 35`
words. This is the source of the "~44 cycles" figure used below.

### Step 5: simulator measurement (authoritative)

Let the chip run from reset under MPLAB SIM (`mdb`) so TMR1 free-runs
and overflows naturally, which fires the real interrupt with the
real, zero-initialized worst case (every `chan_t` field starts at 0,
which is exactly "every `*_ticks_left` expired" on the very first
tick, no memory pokes needed):

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
/opt/microchip/mplabx/v6.35/mplab_platform/bin/mdb.sh mdb_cmds.txt
```

`mdb_cmds.txt`:

```
Device PIC16F877A
Hwtool sim
Program "probe.hex"
break *0x0004
break *0x00C8
Run
Wait 5000
Stopwatch clear
Continue
Wait 5000
Stopwatch
quit
```

Relevant output:

```
break *0x0004
Breakpoint 0 at 0x4.
break *0x00C8
Breakpoint 1 at 0xc8.
Run
Running
Single breakpoint: @0x4
Simulator halted
Stopwatch clear
Stopwatch cleared. Stopwatch cycle count = 0 (0 ns)
Continue
Running
Single breakpoint: @0xC8
Simulator halted
Stopwatch
Stopwatch cycle count = 297 (297 us)
```

297 cycles measured from the interrupt vector (`0x0004`, before its
first instruction executes) to `0x00C8` (before `retfie` executes).
Add `retfie`'s own 2 cycles: **299 cycles total**, matching the manual
trace (303) to within 4 cycles, the small gap explained by which
data-dependent branch (`tx_shift`'s bit, the RX pin sample) the manual
trace assumed versus what the simulator's actual reset-state values
took, not a methodology error.

## What this means (probe.c, array-of-structs)

Budget at 4x/9600 baud, 20 MHz: **130 cycles/tick** (`26.0417 us x 5
cycles/us`). Budget at 3x: **174 cycles/tick** (`34.7222 us x 5
cycles/us`).

Measured worst-case ISR (2 channels, TX+RX both mid-byte-tick-expiry,
interrupt entry through `retfie`): **299 cycles**.

- vs N=4 (130): 299/130 = **2.30x over budget**, margin **-130%**.
- vs N=3 (174): 299/174 = **1.72x over budget**, margin **-72%**.

**Neither oversample factor fits** with this probe's code shape. The
brief's own probe code (array-of-structs, `for` loop, array indexing
by a runtime loop variable) is a reasonable stand-in for "worst-case
arithmetic shape" but two of its properties are not free: the
non-unrolled loop and the `___bmul` call together account for a large,
avoidable chunk of the 299 cycles (removing the multiply and its call
overhead entirely would save roughly 44 cycles, i.e. still ~255,
*still over N=3's 174*). The rest of the overage is the branchy,
non-inlined shape of each `if`/`else` and the ternary-style mask
selects, each compiling to several `goto`s (2 cycles each) that a
hand-unrolled, straight-line per-channel implementation would not
need.

**Consequence:** this is not a "change one constant from 4 to 3"
situation for the *array-of-structs* shape. But the failure here
turned out to be dominated by two properties of the probe code, not
an inherent property of the workload: looping over an array of
channel structs with a runtime-computed index (forces a multiply or
at least non-constant addressing), and deeply nested `if`/`else`/
ternary chains that don't collapse to straight-line code under `-O2`.
A manually-unrolled, two-named-channel rewrite (no loop, no array, no
`___bmul`) was approved as follow-up work and is measured next.

## Follow-up probe: straight-line two-named-channel version (probe2.c)

Same worst-case shape as `probe.c` (both channels' TX bit-clock and RX
sampler on tick expiry), rewritten as straight-line code: two named
channel variable sets (`a_*`/`b_*`, separate `static volatile uint8_t`
globals, not a struct or array), no loop, each channel's TX/RX
handling written out once, sequentially. This is closer to what the
real Task 2 module will look like for a compile-time-bounded 2-channel
count than the array-of-structs version was.

### Compile

```sh
cd /tmp/swuart-probe
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
xc8-cc -mcpu=16f877a \
  -mdfp=/opt/microchip/mplabx/v6.35/packs/Microchip/PIC16Fxxx_DFP/1.8.167/xc8 \
  -O2 -std=c99 -fasmfile -Wa,-a probe2.c -o probe2.hex -ginhx32
```

Output:

```
16F877A Memory Summary:
    Program space        used    ABh (   171) of  2000h words   (  2.1%)
    Data space           used    1Bh (    27) of   170h bytes   (  7.3%)
    EEPROM space         used     0h (     0) of   100h bytes   (  0.0%)
    Configuration bits   used     1h (     1) of     1h word    (100.0%)
    ID Location space    used     0h (     0) of     4h bytes   (  0.0%)
```

171 words total (vs 226 for the array-of-structs version) even before
looking at the ISR in isolation, one signal this shape is cheaper.

Cross-checked against XC8 v4.00 (Docker) the same way as `probe.c`:
same 171-word memory summary, `diff` of the opcode columns produced no
output (byte-identical).

### Simulator measurement

`probe2.lst` puts the interrupt vector trampoline at `0x0004` and
`retfie` at `0x00A0`. Same method as `probe.c`: let TMR1 free-run from
reset so it overflows naturally, firing the interrupt with the real
zero-initialized worst-case state (every `*_ticks_left` starts at 0,
i.e. already "expired" on the first tick, for both channels' TX and
RX).

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
/opt/microchip/mplabx/v6.35/mplab_platform/bin/mdb.sh mdb_cmds2.txt
```

`mdb_cmds2.txt`:

```
Device PIC16F877A
Hwtool sim
Program "probe2.hex"
break *0x0004
break *0x00A0
Run
Wait 5000
Stopwatch clear
Continue
Wait 5000
Stopwatch
quit
```

Relevant output:

```
break *0x0004
Breakpoint 0 at 0x4.
break *0x00A0
Breakpoint 1 at 0xa0.
Run
Running
Single breakpoint: @0x4
Simulator halted
Stopwatch clear
Stopwatch cleared. Stopwatch cycle count = 0 (0 ns)
Continue
Running
Single breakpoint: @0xA0
Simulator halted
Stopwatch
Stopwatch cycle count = 118 (118 us)
```

118 cycles from the vector to just before `retfie`; +2 cycles for
`retfie` itself = **120 cycles** for the reset-state path actually
exercised.

### Checking this is really the worst case, not just the reset-state path

The reset-state run above took specific data-dependent branches (TX
shift register's LSB was 0; both RX input pins read as 0 in the
simulator). Inspected `probe2.lst` for whether the untaken alternative
of each data-dependent branch costs more:

- TX bit-select (`if (bit) PORTB |= mask; else PORTB &= ~mask;`,
  `probe2.lst` around `0x0036`-`0x003D` for channel A, same shape for
  channel B): compiles to a direct `bsf`/`bcf`-style pair either way.
  Traced both arms by hand: **6 cycles regardless of which way `bit`
  goes.** No asymmetry, because this version indexes `PORTB` directly
  (a compile-time-constant address) instead of through the
  array-of-structs version's FSR-indirect `INDF` access.
- RX sample ternary (`rx_shift = (rx_shift>>1) | (sample?0x80:0)`,
  `probe2.lst` `0x004D`-`0x0056` for channel A, `0x0084`-`0x008D` for
  channel B): **asymmetric**, traced by hand: the `sample == 0` arm is
  7 cycles (`movf`+`btfss`(skip taken, 2)+`clrf`+`clrf`+`goto`), the
  `sample != 0` arm is 8 cycles (`movf`+`btfss`(no skip, 1)+`goto`(2)+
  `movlw`+`movwf`+`movlw`+`movwf`). One extra cycle when the sampled
  bit is a 1. This pattern is identical for both channels (confirmed
  by reading both call sites in the listing).

So the true worst case (both RX samples read as 1, not 0 as the reset
state happened to produce) adds 1 cycle per channel: **120 + 2 = 122
cycles** total, worst case, entry through `retfie` inclusive.

### Verdict

Budget at N=4: 130 cycles. Budget at N=3: 174 cycles. Worst-case
measured: **122 cycles**.

- vs N=4 (130): 122/130 = 0.94x, **margin +6.2%** (8 cycles headroom).
- vs N=3 (174): 122/174 = 0.70x, **margin +29.9%** (52 cycles headroom).

**YES for N=4.** The straight-line, two-named-channel shape fits the
4x-oversample budget on the slowest supported chip (PIC16F87XA at 20
MHz), with real headroom, though only about 8 cycles (roughly 6%) of
it at N=4. That is enough to call N=4 viable, but not enough to add
much more per-tick work without re-measuring: the real Task 2 state
machine will have more states than "mid-byte, ticks expired" (start-bit
detection, stop-bit/framing checks, idle detection), and if any of
those add meaningfully more than ~8 cycles to the worst-case tick,
N=4's margin is gone. If Task 2's real implementation grows
noticeably heavier than this probe's body, re-run this same
measurement against the real code before trusting N=4; N=3's ~30%
margin is the fallback with far more slack.

**Consequence for Task 2:** use N=4, with the shape demonstrated here,
two named per-channel variable sets (or two channel structs accessed
through compile-time-constant pointers, not an array indexed by a
loop variable), each channel's TX/RX logic written out directly
rather than through a shared loop body. Avoid re-introducing a
runtime-indexed loop over the channels or a helper function called
per-channel in a way that isn't inlined, either would reintroduce the
overhead this follow-up probe eliminated.

Reload-value formula (unchanged by either measurement, still needed by
Task 2): `reload = 65536 - round(FOSC_HZ / 4 / (baud * N))`. Sanity
check against the probe's own `TMR1H=0xFF, TMR1L=0x7E` (`0xFF7E` =
65406 = `65536 - 130`): matches N=4 exactly, confirming the brief's
example reload value was computed with this same formula. For N=3:
`65536 - round(5000000 / 28800) = 65536 - 174 = 65362` (`0xFF52`).
